/* Direct3D 12 smoke test: creates a device, clears an offscreen texture and reads it back, then presents frames
 * through a swap chain. Prints "OK cleared B G R A" and "OK presented N frames", or FAIL lines.
 * Usage: d3d12-smoke.exe [frames] */
#define COBJMACROS
#define WIDL_C_INLINE_WRAPPERS
#include <windows.h>
#include <initguid.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <stdio.h>
#include <stdlib.h>

#define W 256
#define H 256

static ID3D12Device *device;
static ID3D12CommandQueue *queue;
static ID3D12CommandAllocator *allocator;
static ID3D12GraphicsCommandList *list;
static ID3D12Fence *fence;
static UINT64 fence_value;

static int fail(const char *what, HRESULT hr)
{
    printf("FAIL %s %#lx\n", what, hr);
    return 1;
}

static void barrier(ID3D12Resource *resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
    D3D12_RESOURCE_BARRIER b = {0};
    b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    b.Transition.pResource = resource;
    b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    b.Transition.StateBefore = before;
    b.Transition.StateAfter = after;
    ID3D12GraphicsCommandList_ResourceBarrier(list, 1, &b);
}

static HRESULT run_and_wait(void)
{
    HANDLE event = CreateEventW(NULL, FALSE, FALSE, NULL);
    HRESULT hr;
    if (FAILED(hr = ID3D12GraphicsCommandList_Close(list)))
    {
        CloseHandle(event);
        return hr;
    }
    ID3D12CommandQueue_ExecuteCommandLists(queue, 1, (ID3D12CommandList **)&list);
    if (FAILED(hr = ID3D12CommandQueue_Signal(queue, fence, ++fence_value)))
    {
        CloseHandle(event);
        return hr;
    }
    if (ID3D12Fence_GetCompletedValue(fence) < fence_value)
    {
        ID3D12Fence_SetEventOnCompletion(fence, fence_value, event);
        if (WaitForSingleObject(event, 5000) != WAIT_OBJECT_0) hr = E_FAIL;
    }
    CloseHandle(event);
    ID3D12CommandAllocator_Reset(allocator);
    ID3D12GraphicsCommandList_Reset(list, allocator, NULL);
    return hr;
}

static int clear_offscreen(void)
{
    static const float color[4] = {0.25f, 0.5f, 0.75f, 1.0f};
    D3D12_HEAP_PROPERTIES heap = {D3D12_HEAP_TYPE_DEFAULT}, readback_heap = {D3D12_HEAP_TYPE_READBACK};
    D3D12_RESOURCE_DESC desc = {D3D12_RESOURCE_DIMENSION_TEXTURE2D, 0, W, H, 1, 1, DXGI_FORMAT_B8G8R8A8_UNORM, {1, 0},
                                D3D12_TEXTURE_LAYOUT_UNKNOWN, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET};
    D3D12_RESOURCE_DESC buffer_desc = {D3D12_RESOURCE_DIMENSION_BUFFER, 0, W * H * 4, 1, 1, 1, DXGI_FORMAT_UNKNOWN,
                                       {1, 0}, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE};
    D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1};
    D3D12_TEXTURE_COPY_LOCATION dst = {0}, src = {0};
    D3D12_CPU_DESCRIPTOR_HANDLE rtv;
    ID3D12DescriptorHeap *rtv_heap;
    ID3D12Resource *texture, *readback;
    BYTE *data;
    HRESULT hr;
    int ok;

    if (FAILED(hr = ID3D12Device_CreateCommittedResource(device, &heap, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_RENDER_TARGET, NULL, &IID_ID3D12Resource, (void **)&texture)))
        return fail("CreateCommittedResource(texture)", hr);
    if (FAILED(hr = ID3D12Device_CreateCommittedResource(device, &readback_heap, D3D12_HEAP_FLAG_NONE, &buffer_desc,
            D3D12_RESOURCE_STATE_COPY_DEST, NULL, &IID_ID3D12Resource, (void **)&readback)))
        return fail("CreateCommittedResource(readback)", hr);
    if (FAILED(hr = ID3D12Device_CreateDescriptorHeap(device, &heap_desc, &IID_ID3D12DescriptorHeap, (void **)&rtv_heap)))
        return fail("CreateDescriptorHeap", hr);
    rtv = ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(rtv_heap);
    ID3D12Device_CreateRenderTargetView(device, texture, NULL, rtv);
    ID3D12GraphicsCommandList_ClearRenderTargetView(list, rtv, color, 0, NULL);
    barrier(texture, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
    dst.pResource = readback;
    dst.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    dst.PlacedFootprint.Footprint.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    dst.PlacedFootprint.Footprint.Width = W;
    dst.PlacedFootprint.Footprint.Height = H;
    dst.PlacedFootprint.Footprint.Depth = 1;
    dst.PlacedFootprint.Footprint.RowPitch = W * 4;
    src.pResource = texture;
    src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    ID3D12GraphicsCommandList_CopyTextureRegion(list, &dst, 0, 0, 0, &src, NULL);
    if (FAILED(hr = run_and_wait())) return fail("clear: wait", hr);
    if (FAILED(hr = ID3D12Resource_Map(readback, 0, NULL, (void **)&data))) return fail("Map(readback)", hr);
    data += (H / 2) * W * 4 + (W / 2) * 4;
    ok = abs(data[0] - 191) <= 2 && abs(data[1] - 128) <= 2 && abs(data[2] - 64) <= 2 && data[3] == 255;
    printf("%s cleared %u %u %u %u\n", ok ? "OK" : "FAIL", data[0], data[1], data[2], data[3]);
    ID3D12Resource_Unmap(readback, 0, NULL);
    return ok ? 0 : 1;
}

static int present(int frames)
{
    WNDCLASSW wc = {0, DefWindowProcW, 0, 0, GetModuleHandleW(NULL), NULL, NULL, NULL, NULL, L"d3d12smoke"};
    DXGI_SWAP_CHAIN_DESC1 sc_desc = {W, H, DXGI_FORMAT_B8G8R8A8_UNORM, FALSE, {1, 0},
                                     DXGI_USAGE_RENDER_TARGET_OUTPUT, 2, DXGI_SCALING_STRETCH,
                                     DXGI_SWAP_EFFECT_FLIP_DISCARD, DXGI_ALPHA_MODE_UNSPECIFIED, 0};
    D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2};
    IDXGIFactory4 *factory;
    IDXGISwapChain1 *swapchain1;
    IDXGISwapChain3 *swapchain;
    ID3D12DescriptorHeap *rtv_heap;
    D3D12_CPU_DESCRIPTOR_HANDLE rtv_start;
    UINT rtv_size;
    HWND window;
    HRESULT hr;
    int i;

    RegisterClassW(&wc);
    window = CreateWindowW(L"d3d12smoke", L"d3d12smoke", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, 0, W, H,
                           NULL, NULL, NULL, NULL);
    if (!window) return fail("CreateWindowW", HRESULT_FROM_WIN32(GetLastError()));
    if (FAILED(hr = CreateDXGIFactory2(0, &IID_IDXGIFactory4, (void **)&factory))) return fail("CreateDXGIFactory2", hr);
    if (FAILED(hr = IDXGIFactory4_CreateSwapChainForHwnd(factory, (IUnknown *)queue, window, &sc_desc, NULL, NULL,
            &swapchain1)))
        return fail("CreateSwapChainForHwnd", hr);
    if (FAILED(hr = IDXGISwapChain1_QueryInterface(swapchain1, &IID_IDXGISwapChain3, (void **)&swapchain)))
        return fail("IDXGISwapChain1_QueryInterface", hr);
    if (FAILED(hr = ID3D12Device_CreateDescriptorHeap(device, &heap_desc, &IID_ID3D12DescriptorHeap, (void **)&rtv_heap)))
        return fail("ID3D12Device_CreateDescriptorHeap", hr);
    rtv_start = ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(rtv_heap);
    rtv_size = ID3D12Device_GetDescriptorHandleIncrementSize(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    for (i = 0; i < frames; i++)
    {
        UINT index = IDXGISwapChain3_GetCurrentBackBufferIndex(swapchain);
        float color[4] = {0.0f, (float)i / frames, 1.0f - (float)i / frames, 1.0f};
        D3D12_CPU_DESCRIPTOR_HANDLE rtv = {rtv_start.ptr + index * rtv_size};
        ID3D12Resource *back;
        MSG msg;

        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessageW(&msg);
        if (FAILED(hr = IDXGISwapChain3_GetBuffer(swapchain, index, &IID_ID3D12Resource, (void **)&back)))
            return fail("IDXGISwapChain3_GetBuffer", hr);
        ID3D12Device_CreateRenderTargetView(device, back, NULL, rtv);
        barrier(back, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        ID3D12GraphicsCommandList_ClearRenderTargetView(list, rtv, color, 0, NULL);
        barrier(back, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        if (FAILED(hr = run_and_wait())) return fail("present: wait", hr);
        ID3D12Resource_Release(back);
        if (FAILED(hr = IDXGISwapChain3_Present(swapchain, 1, 0))) return fail("Present", hr);
    }
    printf("OK presented %d frames\n", frames);
    return 0;
}

int main(int argc, char **argv)
{
    D3D12_COMMAND_QUEUE_DESC queue_desc = {D3D12_COMMAND_LIST_TYPE_DIRECT};
    D3D12_FEATURE_DATA_FEATURE_LEVELS levels = {0};
    static const D3D_FEATURE_LEVEL requested[] = {D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0,
                                                  D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0};
    IDXGIFactory4 *factory;
    IDXGIAdapter1 *adapter;
    DXGI_ADAPTER_DESC1 adapter_desc;
    HRESULT hr;
    int frames = argc > 1 ? atoi(argv[1]) : 30;

    if (frames < 1) return fail("frames", E_INVALIDARG);
    if (SUCCEEDED(CreateDXGIFactory2(0, &IID_IDXGIFactory4, (void **)&factory))
        && SUCCEEDED(IDXGIFactory4_EnumAdapters1(factory, 0, &adapter))
        && SUCCEEDED(IDXGIAdapter1_GetDesc1(adapter, &adapter_desc)))
        printf("adapter: %ls\n", adapter_desc.Description);
    if (FAILED(hr = D3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_11_0, &IID_ID3D12Device, (void **)&device)))
        return fail("D3D12CreateDevice", hr);
    levels.NumFeatureLevels = ARRAYSIZE(requested);
    levels.pFeatureLevelsRequested = requested;
    if (SUCCEEDED(ID3D12Device_CheckFeatureSupport(device, D3D12_FEATURE_FEATURE_LEVELS, &levels, sizeof(levels))))
        printf("feature level: %#x\n", levels.MaxSupportedFeatureLevel);
    if (FAILED(hr = ID3D12Device_CreateCommandQueue(device, &queue_desc, &IID_ID3D12CommandQueue, (void **)&queue)))
        return fail("CreateCommandQueue", hr);
    if (FAILED(hr = ID3D12Device_CreateCommandAllocator(device, D3D12_COMMAND_LIST_TYPE_DIRECT,
            &IID_ID3D12CommandAllocator, (void **)&allocator)))
        return fail("ID3D12Device_CreateCommandAllocator", hr);
    if (FAILED(hr = ID3D12Device_CreateCommandList(device, 0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator, NULL,
            &IID_ID3D12GraphicsCommandList, (void **)&list)))
        return fail("ID3D12Device_CreateCommandList", hr);
    if (FAILED(hr = ID3D12Device_CreateFence(device, 0, D3D12_FENCE_FLAG_NONE, &IID_ID3D12Fence, (void **)&fence)))
        return fail("ID3D12Device_CreateFence", hr);
    if (clear_offscreen()) return 1;
    return present(frames);
}
