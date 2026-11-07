# Desktop V6 reviewed compatibility package

## Summary
Packaged independently reviewed negotiated5–30fps/default30 source with126px compact overlays and shared tracking geometry. Final native/pixel/graph/renderer/extractedpackage checks passed. Independent review covers scoped runtime contract, not this archive or Discord acceptance; parent consumer issue remains open.

## Artifact
`dist/BandHeartBeat-desktop-v6.zip`:1,050,183bytes; SHA256 `66F92EB43A6BA2AD448837CB382795FD305C04D3B8B86C5CFF78C8862D88E2E9`.
Folder dist/desktop-v6; extractedtarget/desktop-v6-package-qa.8files: nativeGUI276992,backend1127424,DirectShowDLL173056,camera-admin313344,producer416768,README11185,install62,rollback92bytes. No test helpers,UTF16UxTest orRegistrationQa namespace inEXE/DLL. Native staticCRT, systemMF/WinRT/GDI+,no browser/models. Previous packages preserved.

## Reviewed source consistency
Runtime436a11a and independentreport26f8531; compactrendererb446373 and geometry2801838. `git hash-object filter.h` equals `git rev-parse436a11a:...`:093eda1fc49fdeec5cfaccc2b093bb69bfd971ee, unchanged throughout packaging. No compatibility-source edits here. Independent reviewer used own archivedsnapshot/build and augmented capability/discovery/sample tests; see [review](codex-chromium-fix-review.md). ActualDiscord requestFPS/error remains unknown. Reviewer clarified allocatornegative verifies immediatezero samples and call<1sec, NOT a1second observation window; package report makes same precise claim.

## Author build/tests
- `cmd /c directshow-camera\package-msvc.cmd`:VS2022/SDK fullnative + lockedofflineMSVCRustrelease success. Unrelated overlay.html workingchange preserved and excluded from backendartifact using committedfallbackbytes during compile,then restored. Newfolderdesktop-v6 only.
- controls-probe:68byteABI,realisolatedproducer style/position/size/fallback/no-bg pixels and140%edgeclampPASS. tracking-probe timedhold/reacquire/lowFPS/clampPASS.
- graph-check.cmd repeat:default30RGB32/RGB24NullRenderer36samples/1200ms; Chromiumsequence captureconsumerIMediaSample2 gets18/24/29/30/36samples at15/20/24/25/30fps respectively,negotiatedsubtype/interval,timestamps/length/props verified; restart/disconnect/unloadPASS. Consumer is authored contractmodel,not shippedChromium/Discord.
- A/B formatprobe:releasedV5 rejects lowerFPS,fixed bothRGB accepts5/15/20/24/25/30,reject60. Startupnegative allocatorCommitE_OUTOFMEMORY propagated,StateStopped,immediatecount0,call<1000ms.
- renderer-visual-check.cmd temporal/freeze/disconnected/fill/ECG,BPMphase,no-bg/alpha/residue/scales+compactcasesPASS. Current compact reference [image](screenshots/desktop-compact-width.png). It is generated rendererreview,not livewebcam.
- Isolated mock90published/3015ms,90syntheticunique,0repeated,0physicalcallbacks. Not hardware30uniqueFPS.
- Extracted final producer--self-testPASS(FaceDetector1/Gray8supported1);admin--statusPASS read-only,installedpathcurrentlydesktop-v5,currentpathQAextractedV6. No mutation invoked.
- Manifest8files/namespacesPASS;isolatedsame-sourceGUI mockstart/--quit leftno testdesktop/testproducer;fmtcheckPASS. Productionextractedmock/GUI/backendstartup not attempted against user'sproductionIPC/port/singleton. No newGUIvisual/drag/DPIacceptance claim.

## User migration / errors
Updated RUreadme: fully QuitDiscord fromtray (cross/minimizeinsufficient),ExitpreviousBandHeartBeat,closeotherconsumers;extractV6permanentfolder;newGUI Removeowncamera(waitHRESULT0),Install(waitHRESULT0);verifyinstalled/currentpathbothnewfolderBandHeartBeatDirectShow.dll withUI/--status;reopenDiscord/selectcamera. Source5–30fps negotiation keeps30default,producer30. FailureHR/systemreason remains persistent;foreignownership refused. No silent startupregistrychange. Rollback:newRemove/oldInstall afterclosingconsumers. No productionregistry,userprocess,camera,DiscordGUI/call/message actions by agent.

## Limits
Independent runtime review is scopedPASS,package itself authorvalidation. Existingclock/Pause timestampalignment and late-worker failure recovery limitations remain outsidefixscope,documentedinRUreadme. No fullDiscordacceptance,actualrequestedFPS trace,livefacetrackingquality,Band10compatibility ormeasuredCPU/GPU/RAM guarantee. OldloadedDLL evidence is separate actionablefact,not solecauseproof. ParentDiscordtask staysin_progress until user'sactualsuccess.
