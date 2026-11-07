# Desktop V4 package

## Summary
Built versioned V4 with schema3 animation controls,30fps RGB24/RGB32 source and explicit stale-registration upgrade diagnostics. Real Quartz graphs, temporal/68render/control/tracking probes, extractedselftest and isolated30fps mock passed. Discord acceptance remains pending explicit user upgrade/retry; no user registration/processes changed.

## Frozen artifact
`dist/BandHeartBeat-desktop-v4.zip`1,031,881bytes, SHA256 `800A1FB78B23BB52E2525C54BF139C183EE29C020C651824395D19D05EAC41CF`. Folder `dist/desktop-v4`; extracted `target/desktop-v4-package-qa`.8files: BandHeartBeat.exe272896,band-backend.exe1127424,DirectShowDLL154112,camera-admin295936,camera-producer416256,RUreadme9185,install62,rollback92bytes. No test helper binaries or UTF16UxTest strings. Production frame namespace unchanged; controlsV3 namespace/schema3/68bytes. Old packages preserved. Native CRT static, system MF/WinRT/GDI+ dependencies, no browser runtime/models.

## Build/test
`cmd /c directshow-camera\package-msvc.cmd` builds VS2022/SDK C++ and locked offline MSVC Rust; output changed todesktop-v4. Unrelated src/overlay.html modification preserved; backend artifact built with committed fallback overlay bytes then original bytes restored, excluded from commit. Archive Compress-Archive/Expand-Archive success; SHA256 independently read after generation.
- `graph-check.cmd`: isolated DLL, real Windows Quartz FilterGraph→NullRenderer RGB32 and RGB24. Each ConnectDirectS_OK,RunS_FALSE async,36accepted samples/1200ms,Stop/restart/disconnect/remove/unloadS_OK. Harmless test-DLL LNK4070 name warning.
- `renderer-visual-check.cmd`: temporal/freeze/disconnected/fill/ECG/BPMinterval tests +48background and20style cases PASS. Current animation filmstrip/benchmark copied under docs/reports; not a webcam screenshot or full-pipelineCPU benchmark.
- controls-probe:68byte ABI; real isolated producer changes shared output style/position/size/fallback/background pixels; cleanstop. tracking-probe timedhold/reacquire/clamp/lowFPSequivalence PASS.
- Extracted final producer --self-test PASS,FaceDetector1/Gray8supported1. Extracted camera-admin --status correctly compares old installed DLL with extracted new DLL, no writes.
- Isolated same-source --mock --seconds5:150published/5015ms,150synthetic unique,0repeats,0physical callbacks. Production extracted mock was NOT run: user V3producer owns production mapping. No attempt to mutate/stop it.
- Isolated desktop mock launched (different class/events/IPC), then --quit; after3s passive inventory showed no testdesktop/testproducer. Production desktop startup was NOT run to avoid singleton focus/input against user'sV3. fmtcheckPASS.

## UI / user migration
Native window1120×810client; new toggles animation(y732),optionalECG(y732),heartfilled(y763). Existing no-bg toggle(y362),sliders(y405–565),start(y590),styles(y678–729) have separate hit ranges. Path diagnostics atx24,y754/777 are capped108characters so remain left of right controls atx774; coordinate/code review finds no overlaps at96DPI. Actual visual screenshot NOT accepted: computer-use selected isolated window but capture returned active TanksBlitz gameplay; did not activate/click/retry or save irrelevant image. No final GUI/DPI/accessibility acceptance claimed.

RUreadme includes exact sequence: Exitoldapp,closeDiscord/consumers yourself,launchV4,explicitUpdateVirtualCamera,reopenDiscord/selectcamera. --status read-only; --upgrade only explicit action with ownCLSID/category/threading checks, detected-module-in-use refusal and oldregistration restore attempt on failure. Full registry mutation/rollback not tested on user install. User retains old folder for rollback: closeconsumers,newrollback,oldinstall. Installedpath remains `dist/BandHeartBeat-standalone/BandHeartBeatDirectShow.dll`; new package is not automatically installed.

## Limits
Parent Discord issue remains in_progress. Old source enumerates/binds/delivers locally; no evidence that stale path alone explains Discord. No physicalcamera activation,privacychanges,HKLMwrites,Discordactions,userprocesstermination. No measured uniquehardware30fps,CPU/GPU/RAM or live-facequality acceptance. ECG/heart timing illustrative BPM animation, not measuredECG/beat timestamps. Final package has author validation only.
