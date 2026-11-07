# Virtual-camera IPC contention artifact fix

## Summary
Reproduced placeholder flashes when writer mutex is busy:18bad placeholder samples during600ms isolated contention. Fixed bounded last-good caching; RGB24/RGB32 tests pass contention/recovery/invalid/missing/stale and patterned pixel conversion. User's actual artifacts are not yet visually identified; this fixes a proven defect, not confirmed final Discord acceptance.

## Before / after
Before: every output sample regenerated checkerboard, then attempted WaitForSingleObject(gate,0). Mutex timeout skipped IPCcopy and delivered checkerboard instead of last camera frame. The real capture-consumer isolated probe loaded pre-fix isolatedDLL,received valid synthetic44frame,held writer gate600ms,and observed18pixel mismatches/FAIL. After source rebuild same testpasses bothRGB32/RGB24,with no placeholder during short contention.

Filter now creates placeholder once per deliveryworker,allocates bounded placeholder/cache/work buffers before starting thread (allocationfailure propagated through existingStartDeliveryHRESULT),and copies only validated complete frame under mutex. If mutexbusy,it reuses previous validated frame for maximum2000ms since a different sequence was last observed. Re-reading identical sequence does NOT refresh age. Invalidheader immediately invalidates cache. Missingmap immediately invalidates cache. Staleunchangedsequence/longcontention expires to explicit PLACEHOLDER-NOCAMERA rather than indefinite liveframe. Cache local to eachworker;Stop/restart clears it. OutputRGB24conversion uses same cachedBGRA bytes;RGB32copies correct samplelength. No image allocation/GDIplaceholderdraw on everyframe anymore.5–30fps negotiation/animation/geometry unchanged.

## Tests
`frame-cache-check.cmd` self-builds isolated-name DLL and isolated-only frameprobe;guard beforefactoryinstantiation scans loadedDLL for expected UTF16testIPCname. Compile withoutBH_ISOLATED_TEST rejected atpreprocessor; arbitrary productionDLL path removed fromframeprobe CLI.
For BOTH RGB32andRGB24: firstnoframe→honestplaceholder; publish patternedframe→fourpoint channel/row/stride checks; writer gateheld600ms→all new firstpixel samples remainactualframe; release→valid; newsequence/differentpattern→recovery; version99→placeholder; validrecovery; unchangedsequenceSleep2200ms→placeholder; newsequence→recovery; writer closesmap→placeholder. FinalStop/disconnect/unloadPASS. Points(0,0),(639,0),(0,479),(37,221) have distinctB/G/R values to catch channel order and top/bottom/stride errors,not uniformgrayonly. Pixelhistory bounded120samples andtestonly.
`graph-check.cmd` rerun PASS oldnegotiationmatrix15/20/24/25/30,captureIMediaSample2/bytes/timestamps,restart/unload,Commitfailure,and newcontentionconsumerRGB24/RGB32. Dedicated finalguardedRGB24contention smokePASS. TestDLL LNK4070 librarynamewarning unchanged/harmless.
No productionregistration/process closure or physicalcameraactivation. No newZIP: frozenfix forindependentreview first.

## Excluded attempt / test isolation correction
An additional A/B attempt briefly loaded releasedV6 productionnamespace DLL and failed the initial no-frame assertion because user'smapping alreadyexisted. It read samples only,no mappingwrites/cameraactivation/registrysystemchange/processclosure,saved noimage orpixelvalues. This attempt is excluded fromregressionevidence. It was reported tolead immediately and not repeated. Subsequent probes now enforce isolatedcompileflag AND verify isolatedDLLnamespace beforeclassfactoryinstantiation. Valid beforefix evidence is the originalisolatedDLL18mismatches,not this productionattempt.

## Limits
User said only artifacts,no screenshot/type/consumerformat provided. Flicker frommutexcontention is proven conditionalcause; color/stride/codec artifacts in actualDiscord not asserted. Brief bounded cachedcameraimage may appear duringproducerstall,thenplaceholder within2s; no telemetryLIVE status added tovideo. ActualDiscordretry andindependentreview pending. Runtimeparent taskremainsopen.
