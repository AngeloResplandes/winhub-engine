/* Smoke test for the engine: opens a window, creates a Direct3D 11 device and swap chain,
 * clears and presents a few frames, then prints the adapter and the feature level.
 * Usage: d3d11-smoke.exe [minimum feature level, hex, e.g. b000]
 * Exit code 0 on success. Built with mingw-w64; run under Wine. */
#define COBJMACROS
#include <windows.h>
#include <d3d11.h>
#include <stdio.h>
#include <stdlib.h>

static LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    return DefWindowProcW(hwnd, msg, wp, lp);
}

int main(int argc, char **argv)
{
    WNDCLASSW wc = {0};
    DXGI_SWAP_CHAIN_DESC desc = {0};
    ID3D11Device *device = NULL;
    ID3D11DeviceContext *context = NULL;
    IDXGISwapChain *swapchain = NULL;
    ID3D11Texture2D *backbuffer = NULL;
    ID3D11RenderTargetView *rtv = NULL;
    IDXGIDevice *dxgi_device = NULL;
    IDXGIAdapter *adapter = NULL;
    DXGI_ADAPTER_DESC adapter_desc;
    D3D_FEATURE_LEVEL level;
    const float color[4] = {0.1f, 0.4f, 0.8f, 1.0f};
    HWND hwnd;
    HRESULT hr;
    int i;

    wc.lpfnWndProc = wndproc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.lpszClassName = L"d3d11-smoke";
    RegisterClassW(&wc);
    hwnd = CreateWindowW(L"d3d11-smoke", L"d3d11-smoke", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                         100, 100, 640, 360, NULL, NULL, wc.hInstance, NULL);
    if (!hwnd) { printf("FAIL CreateWindow %lu\n", GetLastError()); return 1; }

    desc.BufferCount = 2;
    desc.BufferDesc.Width = 640;
    desc.BufferDesc.Height = 360;
    desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.OutputWindow = hwnd;
    desc.SampleDesc.Count = 1;
    desc.Windowed = TRUE;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION,
                                       &desc, &swapchain, &device, &level, &context);
    if (FAILED(hr)) { printf("FAIL D3D11CreateDeviceAndSwapChain 0x%08lx\n", hr); return 1; }

    hr = IDXGISwapChain_GetBuffer(swapchain, 0, &IID_ID3D11Texture2D, (void **)&backbuffer);
    if (FAILED(hr)) { printf("FAIL GetBuffer 0x%08lx\n", hr); return 1; }
    hr = ID3D11Device_CreateRenderTargetView(device, (ID3D11Resource *)backbuffer, NULL, &rtv);
    if (FAILED(hr)) { printf("FAIL CreateRenderTargetView 0x%08lx\n", hr); return 1; }

    for (i = 0; i < 30; i++)
    {
        MSG msg;
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessageW(&msg);
        ID3D11DeviceContext_ClearRenderTargetView(context, rtv, color);
        hr = IDXGISwapChain_Present(swapchain, 1, 0);
        if (FAILED(hr)) { printf("FAIL Present 0x%08lx (frame %d)\n", hr, i); return 1; }
    }

    if (SUCCEEDED(ID3D11Device_QueryInterface(device, &IID_IDXGIDevice, (void **)&dxgi_device)) &&
        SUCCEEDED(IDXGIDevice_GetAdapter(dxgi_device, &adapter)) &&
        SUCCEEDED(IDXGIAdapter_GetDesc(adapter, &adapter_desc)))
        printf("adapter: %ls\n", adapter_desc.Description);
    printf("feature level: 0x%x\n", level);
    if (argc > 1 && level < strtoul(argv[1], NULL, 16))
    {
        printf("FAIL feature level 0x%x is below 0x%s\n", level, argv[1]);
        return 1;
    }
    printf("OK presented %d frames\n", i);

    ID3D11RenderTargetView_Release(rtv);
    ID3D11Texture2D_Release(backbuffer);
    IDXGISwapChain_Release(swapchain);
    ID3D11DeviceContext_Release(context);
    ID3D11Device_Release(device);
    DestroyWindow(hwnd);
    return 0;
}
