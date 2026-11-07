# Independent review: bounded frame cache

**Verdict: PASS for frozen `2d04dab6cf5c39404a7c24452a7dc81016cfba6c`.** Independent isolated A/B reproduces placeholder flashes in old source and verifies the scoped cache fix. This does not establish that the user's actual Discord artifacts are resolved.

Reviewed2026-11-07. Extracted `2d04dab` and pre-fix `41191cf` with git archive into ignored `target/cache-independent-review/{fixed,old}`. Built each DLL and every executed probe with `/DBH_ISOLATED_TEST /std:c++17 /O2 /EHsc /MT /W4`, separate outputs. Fixed runtime filter blob `1b63f4201b4ac3385911debc180a3ee4e1010f61` matches the frozen commit. No runtime source modifications. The only tracked deliverable is this report.

Before any class-factory instantiation, independently checked BOTH DLL files contain the complete NUL-terminated UTF16 `Local\BandHeartBeatBGRAUxTest` name and do not contain complete production `Local\BandHeartBeatBGRA`. Committed guarded probes also check the test-name marker before factory lookup. No released production DLL was loaded. No production IPC, registry, physical camera, Discord or user-process actions occurred.

## Execution results

| Test | Independent result |
|---|---|
| Isolated old41191cf + guarded current graph consumer, RGB24,600ms held writer mutex | Expected failure exit9:18pixel mismatches out of21samples during window. Demonstrates real output bytes become placeholder, not merely a metadata claim. |
| Same graph consumer against fixed, RGB24 and RGB32 | Exit0 each,21samples, zero placeholder mismatch; restart/disconnect/unload pass. |
| Committed frame-cache-probe, both formats | Exit0: initial empty placeholder, patterned valid frame,600ms contention, recovery, invalid version, stale unchanged sequence after2200ms, new sequence recovery, missing map and unload all pass. |
| Disposable extended probe, both formats | Exit0: new sequence after1050ms resets age and stays visible another1050ms; mutex held2300ms expires to placeholder; releasing mutex with SAME sequence stays placeholder; new sequence recovers. Stop/restart after map removal produces placeholder and clears delivery state. |
| Negotiated5/15/20/24/25/30fps real Quartz RGB24 consumer |6/18/24/29/30/36samples in1200ms respectively; IMediaSample2, bytes, pointers, monotonic timestamps/exact negotiated duration, restart and unload checks pass. |
| Commit failure injection | E_OUTOFMEMORY propagates from Run, filterStopped and zero immediate samples within bounded call; exit0. |

All new/fixed executions exited0, each subprocess had18second timeout (extended probe30seconds). Build has only existing LNK4070 DLL export-name warning. Old failure was expected and explicitly inspected. Logs: `target/cache-independent-review/independent-results.log` and `fixed/out/extra-results.log`. Disposable extended test copies changed no runtime implementation.

## Source and test assessment

- `filter.h:81–87`: placeholder/cached/work buffers allocated once before worker start; allocation exceptions propagate through existing catch. Cache data/sequence/age are private to that worker. Placeholder GDI drawing is performed once, not every sample.
- `filter.h:95–99`: exact magic/version/640x480/stride2560/byte count validated under writer mutex before full copy. Only first valid acquisition or differing sequence updates last_good; an expired unchanged sequence cannot renew TTL. Invalid header and missing mapping invalidate immediately. Busy/abandoned/unavailable synchronization retains only the bounded prior cache, never reads unprotected bytes. TTL decision is `age<=2000` at sample selection; transition is naturally quantized by delivery interval, not a promise of exact wall-clock presentation deadline.
- `filter.h:100–102`: RGB32 copies complete working frame; RGB24 packs B/G/R from the same cached BGRA frame. Four-point patterned checks at `(0,0),(639,0),(0,479),(37,221)` distinguish channel order, rows and stride in both output layouts. This checks shared-memory row order, not physical camera orientation. It is meaningful spot validation rather than full-frame comparison.
- `filter.h:71–75,81–83`: Stop joins worker; a subsequent worker has fresh cache. Control mutex serializes worker start/stop. Delivery and cache mutation are single-worker-owned; producer copy is protected by IPC mutex. No added shared-cache data race found.
- TestSink records bounded120sample histories under its own mutex, validates lengths/timestamps and latches failures.600ms test starts after valid samples and inspects subsequent samples; real graph A/B additionally latches every pixel mismatch. Extended long-contention test closes gaps in committed coverage for expiry while mutex remains busy and freshness after a new sequence.

## Limits / nonblocking observations

Committed dedicated probe tests invalid version, not every invalid header field; all fields are visibly checked in the same source branch. MapView/OpenMutex failures use bounded cache rather than immediate clearing, whereas missing mapping and observed invalid header clear immediately. Abandoned-writer behavior is source-reviewed, not separately fault-injected.

The guard scans a test string marker and is a practical test-build check, not cryptographic proof against a mixed-namespace binary. Independent compilation flags plus complete production-name absence checks strengthen this run's isolation. Old and fixed tests were run sequentially to avoid collisions in the shared test namespace.

No new blocker in the reviewed change. Previously noted clock/pause timing and ended-worker recovery behavior remain unchanged and outside this cache fix. Real Discord acceptance and the nature of the user's artifacts remain unverified; this is a proven contention-flicker fix only.
