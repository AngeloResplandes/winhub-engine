/* Smoke test for DXMT's NVIDIA extensions: loads nvapi64.dll, calls NvAPI_Initialize and opens
 * system32's nvngx.dll (Wine finds DXMT's builtin through the DLL path, with no file in the bottle).
 * Usage: nvapi-smoke.exe on|off ("on": DXMT_ENABLE_NVEXT=1, NvAPI_Initialize must succeed and nvngx.dll
 * be there; "off": NvAPI_Initialize must refuse)
 * Exit code 0 on success. Built with mingw-w64 (64-bit only); run under Wine. */

#include <stdio.h>
#include <string.h>
#include <windows.h>

typedef void *(__cdecl *query_interface_fn)(unsigned int id);
typedef int (__cdecl *initialize_fn)(void);

#define NVAPI_INITIALIZE_ID 0x0150e828

int main(int argc, char **argv)
{
    int on = argc > 1 && !strcmp(argv[1], "on"), status = -1, nvngx;
    HMODULE nvapi = LoadLibraryA("nvapi64.dll");
    HANDLE file;

    if (nvapi)
    {
        query_interface_fn query = (query_interface_fn)GetProcAddress(nvapi, "nvapi_QueryInterface");
        initialize_fn initialize = query ? (initialize_fn)query(NVAPI_INITIALIZE_ID) : NULL;
        if (initialize) status = initialize();
    }
    file = CreateFileA("C:\\windows\\system32\\nvngx.dll", GENERIC_READ, FILE_SHARE_READ, NULL,
                       OPEN_EXISTING, 0, NULL);
    nvngx = file != INVALID_HANDLE_VALUE;
    if (nvngx) CloseHandle(file);

    printf("nvapi64 %s, NvAPI_Initialize %d, nvngx %s\n", nvapi ? "loaded" : "missing", status,
           nvngx ? "found" : "missing");
    if (on ? (status == 0 && nvngx) : status != 0)
    {
        printf("OK nvidia extensions %s\n", on ? "on" : "off");
        return 0;
    }
    printf("FAIL nvidia extensions %s\n", on ? "on" : "off");
    return 1;
}
