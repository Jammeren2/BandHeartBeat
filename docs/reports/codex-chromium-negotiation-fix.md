# Chromium-style DirectShow negotiation fix

## Summary
Reproduced V5 rejection of Chromium-style requested15/20/24/25fps and fixed negotiated5–30fps interval handling while preserving30default. Added real Quartz capture-consumer sample/property tests and allocatorCommit negative test; all passed. Discord still had old standalone DLL loaded; actual Discord requested format/error is not captured, so Discord acceptance remains unconfirmed.

## Read-only environment evidence
At2026-11-07T10:20:27+05:00, DiscordPID27908 loaded `D:\py\BandHeartBeat\dist\BandHeartBeat-standalone\BandHeartBeatDirectShow.dll`, despite successful V5 registration/current path in supplied screenshot. Loaded file152064bytes,LastWriteTimeUtc2026-11-05T19:29:00,SHA25618D6F9CC2974046CA9902D8555EA34B1851F0F753C59088BC85055653A84ACC4. ReleasedV5 DLL hash32FBE1C4C874CA147FDD7DD5114A7015A375DD42C6AB32EDBCFF06EAE48316E6. This is an already-loaded-module lifetime issue; registration cannot unload it. No process was closed, no reinstall attempted.
Discord media logs available were dated09:16 and contained hardware enumeration only; no relevant capture failureHRESULT or actual requestFPS. No private messages/calls/UI inspected. Chromium main acceptsRGB32+RGB24; standalone WebRTC DS sink rejectsRGB32. These upstream routes differ and are not evidence of Discord's shipped binary exact contract. FriendlyName fallback means absentDevicePath alone is not established failure.

## Proven conditional blocker
Chromium `VideoCaptureDeviceWin` obtainsGetStreamCaps then unconditionally rewrites VIDEOINFOHEADER.AvgTimePerFrame to requested frame_rate before SetFormat. V5 ValidType required exactly333333; deterministic LoadLibrary/factory/config probe against releasedV5 rejected5/15/20/24fps with0x8004022A(VFW_E_TYPE_NOT_ACCEPTED), accepted30 only. This is a real compatibility defect even though user's actual Discord requestedFPS unknown.

Fix: supported interval333333..2000000(5–30fps) consistently advertised by caps. SetFormat stores subtype/interval; GetFormat,ConnectionMediaType,preferred media enumeration,ConnectDirectNULL,worker deadline and sample duration preserve negotiated values.30fps default and producer30Hz unchanged.60fps rejected. RGB24/RGB32 size/format validation retained.

Also fixed Filter::Run masking allocatorCommit failure: StartDelivery returnsHRESULT,Run propagates failedcommit and leaves sourceStopped. UnconnectedRun reportsVFW_E_NOT_CONNECTED. Thread creation failure decommits/returnsE_OUTOFMEMORY. AverageFrameSize now reflectsRGB24 vsRGB32.

## Consumer-faithful regression
`graph-check.cmd` builds isolated testDLL and real WindowsQuartz graph. Added test-only CaptureConsumer IBaseFilter/IPin/IMemInputPin with system allocator; not NullRenderer-only. Mirrors relevant publicChromium sequence: GetStreamCaps→requested interval overwrite→SetFormat→ConnectDirectNULL→ConnectionMediaType assertion→Pause→Run→sampleReceive/GetIMediaSample2properties→Stop/restart/disconnect/unload. Asserts negotiatedRGB24 subtype/interval,actualbytes,monotonic timestamps,exact sampleduration and sample2TIMEVALID/pointer/length. It is an author contract model, not actual Chromium/Discord binary execution; reference code was consulted, not copied.
Observed1200ms captureconsumer deliveries15fps18samples,20fps24,24fps29,25fps30,30fps36. All restart/unload pass. RealNullRenderer RGB32/RGB2430fps36samples each still pass.
Fault allocator returnsE_OUTOFMEMORY fromCommit: sourceRun returns same,StateStopped,zero samples within1000ms. Direct sourceRun used for preciseHRESULT assertion after graphPause; graph teardown safe. Constructor/factory/export/COM boundaries real.
`consumer-format-probe`A/B against releasedV5 vsfixedDLL verifies bothRGB24/32 supported lowerFPS nowS_OK and60fps stillrejected. Build source/probe with installedVS2022/SDK, no registration or physicalcamera. TestDLL LNK4070 name mismatch warning harmless and existing.

## References
- https://github.com/chromium/chromium/blob/main/media/capture/video/win/video_capture_device_win.cc (GetStreamCaps/AvgTimePerFrame/SetFormat/Pause/Run, snapshot read2026-11-07)
- https://github.com/chromium/chromium/blob/main/media/capture/video/win/sink_input_pin_win.cc (format/sample consumer)
- https://github.com/webrtc-mirror/webrtc/blob/main/modules/video_capture/windows/video_capture_ds.cc (distinct standaloneWebRTC flow)
References cached only under ignoredtarget/webrtc-reference; no third-party implementation vendored.

No new ZIP yet: freeze source for independent review first. ParentDiscord task remainsin_progress; actual consumer retry/moduleversion/request tracing still needed. Width126px renderer work concurrent and excluded from this runtime commit; no compositor changes by this author.
