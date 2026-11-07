# Independent review: Chromium negotiation fix

**Verdict: PASS for the scoped negotiation/startup fix in frozen commit `436a11a2c0820a4369241b838f152cb47c98ccba`. Actual Discord capture remains unverified.** No blocking regression found in this scope.

Reviewed on2026-11-07. Extracted committed sources with git archive into ignored `target/overlay-frozen-review`, compiled in its own `out` directory using VS2022 x64 `/O2 /EHsc /MT /W4`. Runtime filter was unmodified: local blob hash `093eda1fc49fdeec5cfaccc2b093bb69bfd971ee` matches the frozen commit. Only build warning was existing harmless LNK4070 export output-name mismatch. Working runtime/geometry files were not modified. No registration writes, physical camera, production IPC, Discord or user-process operations.

## Independent execution

I augmented only disposable test copies, not runtime: added capture-builder FindInterface, boundary requests, GetFormat/EnumMediaTypes/caps assertions, and enabled placeholder pixel checking in CaptureConsumer. These temporary sources, binaries and logs are under `target/overlay-frozen-review`; the only tracked review deliverable is this report.

| Check | Observed result |
|---|---|
| Released `dist/desktop-v5/BandHeartBeatDirectShow.dll`, both RGB32/24 | SetFormat rejected5/15/20/24/25fps with `0x8004022A`;30 accepted; zero/negative/60 rejected. DLL was instantiated/configured only, never Run, so no production IPC reads. |
| Frozen fixed DLL, both RGB32/24 | Accepted5/15/20/24/25/30; rejected zero, negative interval,4/31/60fps. Every accepted request matched GetFormat and first EnumMediaTypes subtype/interval; caps range333333..2000000 matched. |
| Real Quartz graph, default RGB32/RGB24 NullRenderer | Each received36samples in1200ms; Stop/restart/disconnect/unload passed. |
| CaptureBuilder2 FindInterface(capture category, video, IAMStreamConfig) | S_OK in all graph cases; SetFormat -> ConnectDirect(NULL) -> ConnectionMediaType -> Pause -> Run succeeded. |
| RGB24 graph CaptureConsumer at5/15/20/24/25/30fps | Respectively6/18/24/29/30/36samples in1200ms; restart and unload passed. All child test processes exited0 and had12second timeout. |
| Sample validation | Actual RGB24 length921600, pointer, monotonic start time, positive/exact negotiated duration, first placeholder pixel, IMediaSample2 GetProperties actual length/pointer/TIMEVALID/start time checked. Sample failures latch `valid=false` and are asserted after Stop. |
| Allocator Commit fault | Run returned E_OUTOFMEMORY; filter reportedStopped; no samples; call completed within1000ms bound. |

Logs: `target/overlay-frozen-review/out/independent-review.log` and `boundary-review.log`. These are local ignored test evidence, not package output.

## Implementation assessment

- `filter.h:20,39,55`: validation, advertised range and stored interval now agree. GetStreamCaps still returns the default30fps media type with a5–30fps range, which is consistent; SetFormat updates active configuration.
- `filter.h:21–25,34,37,44,58–66`: GetFormat, preferred media enumeration, ConnectionMediaType and null-type Connect preserve selected RGB subtype and interval. RGB32->RGB24 fallback preserves interval.
- `filter.h:85,99`: deadline pacing uses `interval*100`nanoseconds and sample duration uses the same100ns interval. Observed wall-time counts substantiate pacing, rather than checking header metadata alone.
- `filter.h:51,77–81,106`: StartDelivery now returns HRESULT, propagates Commit failure and prevents false Running state; unconnected start returns VFW_E_NOT_CONNECTED; thread-creation catch decommits. The allocator negative probe exercises the real DLL COM boundary.

## Test limits and remaining work

The author's graph test is a meaningful custom consumer in a real Quartz graph, **not execution of Chromium's sink or Discord**. Its original path omitted CaptureBuilder FindInterface; my disposable augmentation covers that. It does not validate registered-moniker activation or browser sandbox/cache/architecture/backend policy. Reported old loadedDLL is a strong separate diagnostic clue, not proof it is the sole cause.

The negative test checks that Run returns within1000ms and checks count immediately; it does not observe a full1second no-sample interval. Since Commit fails before worker creation, source inspection supports no worker delivery; describe this as bounded startup failure, not a1second observation. CaptureConsumer verifies a pixel spot, not full-frame image correctness; IMediaSample2 checks TIMEVALID/start but not all flags or stop-time properties. Sample count bounds are broad, although independently observed counts closely match requested rates.

Pre-existing timing/recovery issues remain outside this fix: Run ignores clock start, pause/resume timestamps do not account for elapsed graph time, and a later delivery-worker failure leaves a joinable ended worker that StartDelivery can incorrectly treat as resumable (`filter.h:79,82,86,101`). These do not invalidate tested initial negotiation/Commit propagation, but should be addressed if consumers fail after startup or reconnection. Allocator alignment requirements and reentrant connection callbacks are also unchanged.

Recommended next consumer validation: fully exit/restart Discord to unload the old module, verify only this camera's loaded DLL version/path, then retry with bounded camera-specific subtype/interval/HRESULT/first-sample diagnostics if failure persists. No claim of Discord compatibility should be made solely from these tests.
