// Compile-only Windows 11 virtual-camera API boundary spike.
// Deliberately does not call MFCreateVirtualCamera or touch camera devices.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mfapi.h>
#include <mfvirtualcamera.h>
#include <mfreadwrite.h>
#include <iostream>

#pragma comment(lib, "mfsensorgroup.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")

int main() {
    // Taking the address proves the SDK declaration and link surface are visible.
    using CreateVirtualCameraFn = decltype(&MFCreateVirtualCamera);
    CreateVirtualCameraFn api = &MFCreateVirtualCamera;
    (void)api;
    std::cout << "MFCreateVirtualCamera declaration available; activation intentionally skipped.\n";
    std::cout << "Required sourceId form: {CLSID}; target lifetime: Session; access: CurrentUser.\n";
    return 0;
}
