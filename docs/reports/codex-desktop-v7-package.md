# Desktop V7 frame-cache package

## Summary
Packaged independently reviewed bounded frame-cache fix with prior compact cards/animations/no-bg/2secfacehold/negotiated5–30fps(default30)/InstallRemove. Final isolated cache/graph/control/tracker and extractedpackage checks passed. This addresses proven conditional placeholderflicker; actualuserartifacttype andDiscordsuccess remain unconfirmed.

## Artifact and freeze
`dist/BandHeartBeat-desktop-v7.zip`:1,051,524bytes;SHA256 `320404B4FE6C930FDC2FDC79D5F2B30A2EA4865369A6C69A0DF9A6EAD1C3601A`.
Folderdist/desktop-v7;extractedtarget/desktop-v7-package-qa.8releasefiles,no testhelpers/UTF16UxTest/RegistrationQa names. Previouspackages preserved. Runtimefilterblob `1b63f4201b4ac3385911debc180a3ee4e1010f61` equals frozen2d04dab throughout packaging,no runtimeedits.

Independentreview1fa3324 [report](codex-frame-cache-review.md) used own archivedold/fixedisolatedbuilds. Before18placeholderpixelmismatches/600ms,afterzeroRGB24/32. Additional reviewer checks newseqage reset,long2300msbusyexpiry,sameseqrelease staysplaceholder,newseqrecovery,Stop/restartclear. This is runtime-review scope,not independent archive/Discord acceptance. Age<=2000ms is checked when choosing sample; delivery/consumer scheduling quantizes displaytime,not exactscreen deadline.

## Author package tests
- VS2022SDK package-msvc.cmd fullnative+lockedofflineMSVCRustreleasePASS;newdesktop-v7output. Unrelated overlay.html workingchange preserved/excluded frombackendartifact via committedfallbackbytes duringcompile,thenrestored.
- frame-cache-check.cmd guardedisolatedDLLbothRGB: initialempty,patternvalid,600mscontention,recovery,invalidheader,unchangedseq2.2sexpiry,newseqrecovery,producer-mapclosure,4pixelpoints forchannels/rows/stridePASS. Invalid/missing clearscache;stale/longbusyexpires. NoindefiniteLIVEframe.
- graph-check.cmd negotiation5/15/20/24/25/30/reject60,A/BV5strict30difference,realQuartzconsumerIMediaSample2,length,pointer,timestamps/duration,restart/unload,allocatorCommitnegative,RGB24/32contentionPASS. Negativeasserts immediatezero+boundedcall,not1secobservation.
- controls-probe actualisolatedproducer style/position/size/fallback/no-bg/compactedgeclampPASS;trackingprobehold/reacquire/clampPASS.
- Extractedfinalproducer--self-testPASS,FaceDetector1Gray8supported1;admin--statusPASSreadonly(installeddesktop-v6 vsnewextractedV7). No mutation executed. Productionmock/GUI/backend not launched againstuserliveIPC/port/singleton.
-8filemanifest/noTestNamespacesPASS;fmtcheckPASS. Prior compact/render content unchanged; current [compactpreview](screenshots/desktop-compact-width.png) is generatedrendererreview,not livecamera screenshot. No newGUIvisualacceptance claim.

## User migration / limits
RUreadme: fullyQuitDiscordfromtray (oldDLLcanstayloaded),ExitoldBandHeartBeat,closeconsumers,launchV7permanentfolder,explicitRemoveowncamera thenInstall(waitHRESULT0),verifyinstalled/currentbothnewfolderBandHeartBeatDirectShow.dll,restartDiscord/selectcamera. Errorspersist withHRESULT/systemreason. No automatic appclosing/registrychange. Previouspackages retained forrollback.
No productionregistrywrites,physicalcameraactivation oruserprocessclosures inthispackagestage;allruntimeprobesisolated. Earlier accidental read-onlyproductionprobe attempt remains disclosed/excluded in [authorfixreport](codex-frame-cache-artifacts.md),not repeated here.
ActualDiscordartifacts not visuallyclassified; no solecause/finalfixclaim. Existingclock/Pausetimestampalignment andlate-workerrecovery limitations unchanged. Nohardwareunique30fps/resource/Band10/livefacetrackingclaim. ParentDiscordtask remainsin_progress.
