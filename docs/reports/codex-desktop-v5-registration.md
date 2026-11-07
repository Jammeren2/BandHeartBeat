# V5 explicit camera installation/removal

## Summary
Replaced opaque Update with separate Install/Remove buttons and persistent async HRESULT/system-error results. Registration removal supports verified own legacy/stale/missing-DLL/partial entries without loading old DLL; foreign entries are preserved. Isolated registry tests and V5 package checks passed; Discord consumer acceptance remains unresolved.

## Failure located
V4 Registration worker put helper stdout into generic status, but WM_TIMER replaced it every33ms during capture. Draw also truncated output at first newline, while --upgrade printed installed/current paths before eventual error/HRESULT. Thus update errors were effectively hidden. Old upgrade refused a DLL detected in use and required LoadLibrary(oldpath), so missing old files prevented cleanup. The exact user's old exit/HRESULT was not retained, and no production update was reproduced; these are confirmed source defects/possible refusal paths, not a fabricated exact user trace.

## Fix
Two full-width always-visible native buttons «Установить виртуальную камеру» and «Удалить виртуальную камеру». Install enabled only without existing own/partial/foreign key pair; Remove enabled when either key exists; both disabled during owned helper work. Foreign removal returns explicit failure rather than overwrite. Source paths remain visible. New registration_status is separate from capture status, drawn multiline below controls. Working/success/failure persists; exitcode and UTF8 helper HRESULT/system reason shown. Empty output/nonzero exit/launch failure have explicit error paths. Helper polling is off UI thread and bounded10seconds; timeout stops only owned helper, never consumers. Win32 launch error preserved across handle cleanup. No automatic startup action, no --upgrade or old-DLL dependency.

`RegistrationStore` shares exact install/remove code between DLL exports and admin. Current install validates current DLL file existence and never overwrites any prior registration. Ownership checks unique expected CLSID/category plus Product marker or legacy filename/category evidence, rejects contradictory marker/friendlyname/GUID/threading/foreign DLL basename. Partial entries without proof are refused; proven own partial class/category removed. Missing or different old path is not grounds to refuse own cleanup. It deletes only two explicit scoped keys. Explicit user Remove then Install; no hidden migration.

Production default HKCU Software/Classes. `BH_REGISTRY_TEST` helper uses HKCU Software/BandHeartBeat/RegistrationQa instead; isolated desktop uses test helper, isolated frame/control maps/events/class. Test binaries excluded from ZIP.

## Actual checks
- VS2022/SDK package-msvc.cmd build/link success; native static CRT, locked offline Rust MSVC. Unrelated overlay.html preserved and excluded from artifact via committed fallback bytes during backend build.
- registration-probe actual isolated HKCU: install/remove/reinstall; stale path Z:/missing/BandHeartBeatDirectShow.dll; partial category only, Product-only, legacy path-only; foreign friendlyname refusal with key preserved; idempotent absent removal. Action command dispatch exact --register/--unregister; launcherror, empty helper output, HRESULT failure formatting assertions. Probe cleans complete RegistrationQa branch; Test-Path False.
- Real camera-admin-test subprocess sequence register/remove/register/remove produced HRESULT0 success. Duplicate register produced0x800700B7 and explicit Remove-then-Install text. Copied helper without DLL returned0x80070002 and Windows file-not-found reason. Final regression probe rerun and isolated test branch absent.
- controls-probe published-pixel and tracking-probe passed. Compositor/filter/producer behavior untouched in this change, so prior Quartz30fps/temporal renderer evidence remains applicable; no invented new full pipeline test.
- Extracted V5 producer --self-test PASS FaceDetector1/Gray8supported1. Extracted admin --status and --dry-run pass read-only. Installed path remains old standalone path, not altered.
- Isolated native mock desktop start and --quit: after2second startup and2second cleanup, no testdesktop/testproducer left. Final test build dispatches registration to isolated helper. No GUI clicking/install action or visual acceptance claimed; source layout rectangles checked: buttons240–308, switches330/366/402, sliders445–605,Start630, dedicated result812–892 in1120×900client. High-DPI layout unverified.
- Manifest8files only; UTF16 UxTest/RegistrationQa absent in EXE/DLL. fmtcheckPASS. No physical camera,Discord operation,production registry changes or user process closure. Existing production mappings not touched by our testproducer.

## Package
`dist/BandHeartBeat-desktop-v5.zip`:1,048,240bytes; SHA256 `8F3F5FFA94EDA170074D04CB0DFC1659871205FBC5DC9E583190BEF2AE728CAE`.
Folder dist/desktop-v5; extracted target/desktop-v5-package-qa. BandHeartBeat.exe276992,band-backend.exe1127424,DirectShowDLL172544,camera-admin312320,camera-producer416256,README9784,install62,rollback92bytes. Previous versions preserved. RUreadme: Exit old app,closeDiscord/consumers,launchV5,Remove(waitHRESULT0),Install(waitHRESULT0),reopenDiscord/selectcamera; rollback via newremove/oldinstall. Source registration marker/migration affects no frame ABI or animations.

## Remaining limits
Exact V4 user action failure could not be recovered from lost UI output. New production registry actions intentionally not executed here. Discord actual loading/live-face quality/HuaweiBand10/uniquehardware30fps/resource utilization remain unverified. Parent Discord task stays in_progress; package is author-tested, not proof Discord fixed.
