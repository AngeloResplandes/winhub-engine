/*
 * Vulkan feature probe for the Direct3D 12 spike.
 *
 * Loads vulkan-1.dll (winevulkan -> MoltenVK) dynamically, creates an instance,
 * takes the first physical device and checks what vkd3d-proton v3.0.1 requires
 * to create a D3D12 device. The requirement list was read from the project's
 * README.md ("Drivers") and libs/vkd3d/device.c / state.c at that tag:
 *   - VKD3D_MIN_API_VERSION = Vulkan 1.3 (instance and physical device)
 *   - vkd3d_init_device_caps(): samplerMirrorClampToEdge, robustness2
 *     (robustBufferAccess2, robustImageAccess2, nullDescriptor), shaderDrawParameters,
 *     VK_KHR_push_descriptor, VK_EXT_vertex_attribute_divisor (instance rate +
 *     zero divisor), transformFeedbackQueries (VK_EXT_transform_feedback),
 *     single texel alignment for uniform and storage texel buffers
 *   - vkd3d_bindless_state_init(): descriptorIndexing, shaderStorageTexelBufferArray-
 *     NonUniformIndexing, shaderStorageImageArrayNonUniformIndexing,
 *     descriptorBindingVariableDescriptorCount, and (without descriptor buffers)
 *     maxPerStageDescriptorUpdateAfterBind{SampledImages,StorageImages,StorageBuffers}
 *     >= 1000000
 *   - README: every feature of VkPhysicalDeviceDescriptorIndexingFeatures, and
 *     1000000 UpdateAfterBind descriptors for all types except uniform buffers
 * Each line is tagged [code] (the library refuses to create a device) or [readme]
 * (stated in the README only).
 *
 * Build:
 *   x86_64-w64-mingw32-gcc -O2 -Wall -idirafter work/wine-11.0-cx26.3.0/include \
 *       -o vulkan-features.exe tests/vulkan-features.c
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wine/vulkan.h>

static PFN_vkGetInstanceProcAddr gipa;
static VkInstance inst;

#define IPROC(name) PFN_##name p_##name = (PFN_##name)gipa(inst, #name)

static VkExtensionProperties *exts;
static uint32_t n_exts;

static const VkExtensionProperties *find_ext(const char *name)
{
    uint32_t i;
    for (i = 0; i < n_exts; ++i)
        if (!strcmp(exts[i].extensionName, name))
            return &exts[i];
    return NULL;
}

static int n_missing, n_missing_code;

static void check(const char *tag, const char *what, int ok, const char *detail)
{
    printf("%s %-7s %s%s%s\n", ok ? "have   " : "MISSING", tag, what,
           detail && *detail ? "  " : "", detail ? detail : "");
    if (!ok)
    {
        ++n_missing;
        if (!strcmp(tag, "[code]"))
            ++n_missing_code;
    }
}

static void check_ext(const char *tag, const char *name, uint32_t min_spec)
{
    const VkExtensionProperties *e = find_ext(name);
    char d[64] = "";
    if (e)
        snprintf(d, sizeof(d), "(spec %u)", (unsigned)e->specVersion);
    check(tag, name, e && e->specVersion >= min_spec, d);
}

static void info_ext(const char *name)
{
    const VkExtensionProperties *e = find_ext(name);
    printf("  %-8s %s\n", e ? "present" : "absent", name);
}

#define F(tag, s, m) check(tag, #m, (s).m != 0, "")
#define FI(s, m) printf("  %-8s %s\n", (s).m ? "true" : "false", #m)

int main(void)
{
    HMODULE vk = LoadLibraryA("vulkan-1.dll");
    VkApplicationInfo app = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    VkInstanceCreateInfo ici = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    VkPhysicalDevice pdevs[8];
    uint32_t npd = 8;
    VkPhysicalDeviceProperties2 props2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
    VkPhysicalDeviceVulkan11Properties p11 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES};
    VkPhysicalDeviceVulkan12Properties p12 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES};
    VkPhysicalDeviceVulkan13Properties p13 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES};
    VkPhysicalDeviceTransformFeedbackPropertiesEXT xfbp = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_PROPERTIES_EXT};
    VkPhysicalDeviceFeatures2 f2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
    VkPhysicalDeviceVulkan11Features f11 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};
    VkPhysicalDeviceVulkan12Features f12 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    VkPhysicalDeviceVulkan13Features f13 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    VkPhysicalDeviceRobustness2FeaturesEXT rob2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT};
    VkPhysicalDeviceVertexAttributeDivisorFeaturesEXT vad = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_FEATURES_EXT};
    VkPhysicalDeviceTransformFeedbackFeaturesEXT xfb = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_FEATURES_EXT};
    VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT mut = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MUTABLE_DESCRIPTOR_TYPE_FEATURES_EXT};
    VkPhysicalDeviceDescriptorBufferFeaturesEXT dbuf = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT};
    VkPhysicalDeviceExtendedDynamicState3FeaturesEXT eds3 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT};
    VkPhysicalDeviceDepthClipEnableFeaturesEXT dce = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_ENABLE_FEATURES_EXT};
    VkPhysicalDeviceImageViewMinLodFeaturesEXT minlod = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_VIEW_MIN_LOD_FEATURES_EXT};
    VkResult vr;
    uint32_t i, apiver;
    const VkPhysicalDeviceFeatures *f;
    void *chain_f = NULL, *chain_p = NULL;
    char tmp[128];
    int single_storage, single_uniform;

    if (!vk)
    {
        printf("FAIL LoadLibrary vulkan-1.dll (%lu)\n", GetLastError());
        return 2;
    }
    gipa = (PFN_vkGetInstanceProcAddr)GetProcAddress(vk, "vkGetInstanceProcAddr");
    if (!gipa)
    {
        printf("FAIL vkGetInstanceProcAddr\n");
        return 2;
    }
    {
        PFN_vkCreateInstance create = (PFN_vkCreateInstance)gipa(NULL, "vkCreateInstance");
        PFN_vkEnumerateInstanceVersion enum_ver =
            (PFN_vkEnumerateInstanceVersion)gipa(NULL, "vkEnumerateInstanceVersion");
        uint32_t loader = VK_API_VERSION_1_0;

        if (enum_ver)
            enum_ver(&loader);
        printf("instance version: %u.%u.%u\n", VK_API_VERSION_MAJOR(loader),
               VK_API_VERSION_MINOR(loader), VK_API_VERSION_PATCH(loader));
        app.pApplicationName = "vulkan-features";
        app.apiVersion = VK_API_VERSION_1_3;
        ici.pApplicationInfo = &app;
        vr = create(&ici, NULL, &inst);
        if (vr)
        {
            printf("FAIL vkCreateInstance(apiVersion 1.3) %d\n", vr);
            return 2;
        }
    }

    {
        IPROC(vkEnumeratePhysicalDevices);
        IPROC(vkGetPhysicalDeviceProperties);
        IPROC(vkGetPhysicalDeviceProperties2);
        IPROC(vkGetPhysicalDeviceFeatures2);
        IPROC(vkEnumerateDeviceExtensionProperties);

        vr = p_vkEnumeratePhysicalDevices(inst, &npd, pdevs);
        if (vr < 0 || !npd)
        {
            printf("FAIL vkEnumeratePhysicalDevices %d (count %u)\n", vr, npd);
            return 2;
        }

        {
            VkPhysicalDeviceProperties pr;
            p_vkGetPhysicalDeviceProperties(pdevs[0], &pr);
            apiver = pr.apiVersion;
            printf("device: %s\n", pr.deviceName);
            printf("device apiVersion: %u.%u.%u\n", VK_API_VERSION_MAJOR(apiver),
                   VK_API_VERSION_MINOR(apiver), VK_API_VERSION_PATCH(apiver));
        }

        n_exts = 0;
        p_vkEnumerateDeviceExtensionProperties(pdevs[0], NULL, &n_exts, NULL);
        exts = calloc(n_exts, sizeof(*exts));
        p_vkEnumerateDeviceExtensionProperties(pdevs[0], NULL, &n_exts, exts);
        printf("device extensions: %u\n", n_exts);
        for (i = 0; i < n_exts; ++i)
            printf("  ext %s %u\n", exts[i].extensionName, (unsigned)exts[i].specVersion);

        /* Property chain. Extension structs only when the extension is present. */
        p11.pNext = &p12;
        p12.pNext = &p13;
        chain_p = &p11;
        if (find_ext("VK_EXT_transform_feedback"))
        {
            p13.pNext = &xfbp;
        }
        props2.pNext = chain_p;
        p_vkGetPhysicalDeviceProperties2(pdevs[0], &props2);
        printf("driver: %s (%s) driverID %d\n", p12.driverName, p12.driverInfo, p12.driverID);

        /* Feature chain. */
        f11.pNext = &f12;
        f12.pNext = &f13;
        {
            void **tail = &f13.pNext;
            if (find_ext("VK_EXT_robustness2")) { *tail = &rob2; tail = &rob2.pNext; }
            if (find_ext("VK_EXT_vertex_attribute_divisor")) { *tail = &vad; tail = &vad.pNext; }
            if (find_ext("VK_EXT_transform_feedback")) { *tail = &xfb; tail = &xfb.pNext; }
            if (find_ext("VK_EXT_mutable_descriptor_type")) { *tail = &mut; tail = &mut.pNext; }
            if (find_ext("VK_EXT_descriptor_buffer")) { *tail = &dbuf; tail = &dbuf.pNext; }
            if (find_ext("VK_EXT_extended_dynamic_state3")) { *tail = &eds3; tail = &eds3.pNext; }
            if (find_ext("VK_EXT_depth_clip_enable")) { *tail = &dce; tail = &dce.pNext; }
            if (find_ext("VK_EXT_image_view_min_lod")) { *tail = &minlod; tail = &minlod.pNext; }
        }
        chain_f = &f11;
        f2.pNext = chain_f;
        p_vkGetPhysicalDeviceFeatures2(pdevs[0], &f2);
        f = &f2.features;
    }

    printf("\n== vkd3d-proton v3.0.1 hard requirements ==\n");
    snprintf(tmp, sizeof(tmp), "(device %u.%u.%u)", VK_API_VERSION_MAJOR(apiver),
             VK_API_VERSION_MINOR(apiver), VK_API_VERSION_PATCH(apiver));
    check("[code]", "Vulkan 1.3 (VKD3D_MIN_API_VERSION)", apiver >= VK_API_VERSION_1_3, tmp);

    check_ext("[code]", "VK_EXT_robustness2", 0);
    F("[code]", rob2, robustBufferAccess2);
    F("[code]", rob2, robustImageAccess2);
    F("[code]", rob2, nullDescriptor);
    check_ext("[code]", "VK_KHR_push_descriptor", 0);
    F("[code]", f12, samplerMirrorClampToEdge);
    F("[code]", f11, shaderDrawParameters);
    check_ext("[code]", "VK_EXT_vertex_attribute_divisor", 3);
    F("[code]", vad, vertexAttributeInstanceRateDivisor);
    F("[code]", vad, vertexAttributeInstanceRateZeroDivisor);
    check_ext("[code]", "VK_EXT_transform_feedback", 0);
    check("[code]", "transformFeedbackQueries", xfbp.transformFeedbackQueries != 0, "");
    single_storage = p13.storageTexelBufferOffsetSingleTexelAlignment ||
                     p13.storageTexelBufferOffsetAlignmentBytes == 1;
    single_uniform = p13.uniformTexelBufferOffsetSingleTexelAlignment ||
                     p13.uniformTexelBufferOffsetAlignmentBytes == 1;
    snprintf(tmp, sizeof(tmp), "(single %u, bytes %llu)", (unsigned)p13.storageTexelBufferOffsetSingleTexelAlignment,
             (unsigned long long)p13.storageTexelBufferOffsetAlignmentBytes);
    check("[code]", "storage texel buffer single-texel alignment", single_storage, tmp);
    snprintf(tmp, sizeof(tmp), "(single %u, bytes %llu)", (unsigned)p13.uniformTexelBufferOffsetSingleTexelAlignment,
             (unsigned long long)p13.uniformTexelBufferOffsetAlignmentBytes);
    check("[code]", "uniform texel buffer single-texel alignment", single_uniform, tmp);

    printf("-- descriptor indexing (Vulkan 1.2 features) --\n");
    F("[code]", f12, descriptorIndexing);
    F("[code]", f12, shaderStorageTexelBufferArrayNonUniformIndexing);
    F("[code]", f12, shaderStorageImageArrayNonUniformIndexing);
    F("[code]", f12, descriptorBindingVariableDescriptorCount);
    F("[readme]", f12, shaderInputAttachmentArrayDynamicIndexing);
    F("[readme]", f12, shaderUniformTexelBufferArrayDynamicIndexing);
    F("[readme]", f12, shaderStorageTexelBufferArrayDynamicIndexing);
    F("[readme]", f12, shaderUniformBufferArrayNonUniformIndexing);
    F("[readme]", f12, shaderSampledImageArrayNonUniformIndexing);
    F("[readme]", f12, shaderStorageBufferArrayNonUniformIndexing);
    F("[readme]", f12, shaderInputAttachmentArrayNonUniformIndexing);
    F("[readme]", f12, shaderUniformTexelBufferArrayNonUniformIndexing);
    F("[readme]", f12, descriptorBindingUniformBufferUpdateAfterBind);
    F("[readme]", f12, descriptorBindingSampledImageUpdateAfterBind);
    F("[readme]", f12, descriptorBindingStorageImageUpdateAfterBind);
    F("[readme]", f12, descriptorBindingStorageBufferUpdateAfterBind);
    F("[readme]", f12, descriptorBindingUniformTexelBufferUpdateAfterBind);
    F("[readme]", f12, descriptorBindingStorageTexelBufferUpdateAfterBind);
    F("[readme]", f12, descriptorBindingUpdateUnusedWhilePending);
    F("[readme]", f12, descriptorBindingPartiallyBound);
    F("[readme]", f12, runtimeDescriptorArray);

    printf("-- UpdateAfterBind descriptor limits (need >= 1000000) --\n");
#define LIM(tag, m) do { snprintf(tmp, sizeof(tmp), "(= %u)", (unsigned)p12.m); \
        check(tag, #m, p12.m >= 1000000, tmp); } while (0)
    LIM("[code]", maxPerStageDescriptorUpdateAfterBindSampledImages);
    LIM("[code]", maxPerStageDescriptorUpdateAfterBindStorageImages);
    LIM("[code]", maxPerStageDescriptorUpdateAfterBindStorageBuffers);
    LIM("[readme]", maxPerStageDescriptorUpdateAfterBindSamplers);

    printf("\nvkd3d-proton: %d missing (%d of them enforced by the library's own device checks)\n",
           n_missing, n_missing_code);

    printf("\n== Not counted: recommended / optional (informational) ==\n");
    info_ext("VK_EXT_image_view_min_lod");
    info_ext("VK_EXT_mutable_descriptor_type");
    info_ext("VK_VALVE_mutable_descriptor_type");
    info_ext("VK_EXT_descriptor_buffer");
    info_ext("VK_EXT_depth_clip_enable");
    info_ext("VK_EXT_extended_dynamic_state3");
    info_ext("VK_KHR_robustness2");
    info_ext("VK_KHR_calibrated_timestamps");
    info_ext("VK_EXT_conservative_rasterization");
    info_ext("VK_EXT_custom_border_color");
    info_ext("VK_EXT_shader_stencil_export");
    info_ext("VK_KHR_maintenance5");
    info_ext("VK_EXT_graphics_pipeline_library");
    info_ext("VK_EXT_mesh_shader");
    info_ext("VK_KHR_fragment_shading_rate");
    info_ext("VK_KHR_acceleration_structure");
    info_ext("VK_KHR_ray_tracing_pipeline");
    FI(mut, mutableDescriptorType);
    FI(dbuf, descriptorBuffer);
    FI(dce, depthClipEnable);
    FI(minlod, minLod);
    FI(f13, dynamicRendering);
    FI(f13, synchronization2);
    FI(f13, maintenance4);
    FI(f12, timelineSemaphore);
    FI(f12, bufferDeviceAddress);
    FI(f12, shaderFloat16);
    FI(f12, scalarBlockLayout);
    printf("-- core features D3D12 feature levels lean on --\n");
    FI(*f, geometryShader);
    FI(*f, tessellationShader);
    FI(*f, shaderCullDistance);
    FI(*f, shaderClipDistance);
    FI(*f, depthClamp);
    FI(*f, depthBiasClamp);
    FI(*f, fillModeNonSolid);
    FI(*f, independentBlend);
    FI(*f, dualSrcBlend);
    FI(*f, logicOp);
    FI(*f, multiDrawIndirect);
    FI(*f, drawIndirectFirstInstance);
    FI(*f, shaderInt64);
    FI(*f, shaderFloat64);
    FI(*f, shaderInt16);
    FI(*f, imageCubeArray);
    FI(*f, sampleRateShading);
    FI(*f, pipelineStatisticsQuery);
    FI(*f, fragmentStoresAndAtomics);
    FI(*f, vertexPipelineStoresAndAtomics);
    FI(*f, shaderStorageImageWriteWithoutFormat);
    FI(*f, shaderStorageImageReadWithoutFormat);
    FI(*f, sparseBinding);
    FI(*f, shaderResourceResidency);
    FI(*f, shaderResourceMinLod);
    FI(*f, robustBufferAccess);

    return n_missing ? 1 : 0;
}
