# Virtual-camera DLL export fix

Root cause of the previous activation `0x800401F9 CO_E_ERRORINDLL` was confirmed: the built DLL exported only the test constructors (`CreatePlaceholderSource` and `CreatePlaceholderStream`), not the COM entrypoints required by the registered InProcServer32 class. The `STDAPI` declarations alone did not place those names in the DLL export table.

Added `virtual-camera/source.def` and passed `/DEF:virtual-camera\source.def` to the MSVC linker. The DLL now exports exact undecorated names:

```text
DllGetClassObject
DllCanUnloadNow
DllRegisterServer
DllUnregisterServer
```

`activation-probe.exe` default dry-run now loads `BandHeartBeatSource.dll` and checks all four with `GetProcAddress`; observed output: `Export check: PASS`. The build succeeds with VS2022/SDK and the probe performs no registry write, camera activation, privacy change, physical camera access or Discord operation.

The corrected DLL has not been registered or activated in this task. The next explicitly approved command sequence is: call `DllRegisterServer` for the built DLL, verify HKCU InprocServer32, then run `activation-probe.exe --activate`; on failure or completion call `--remove`, `DllUnregisterServer`, and verify no PnP/registry entry. A complete IMFMediaSource/stream contract remains necessary even with the corrected COM exports.

Official references: [COM in-process servers](https://learn.microsoft.com/en-us/windows/win32/com/in-process-server), [MFCreateVirtualCamera](https://learn.microsoft.com/en-us/windows/win32/api/mfvirtualcamera/nf-mfvirtualcamera-mfcreatevirtualcamera).

## Summary

Fixed the production DLL export table using a DEF file; dry-run `GetProcAddress` checks now pass for all required COM registration/class-factory exports. No registration or activation was performed; the source-media contract remains the next blocker.
