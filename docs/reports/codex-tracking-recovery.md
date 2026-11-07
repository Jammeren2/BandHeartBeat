# Timed face-loss recovery

## Summary
Implemented a2000ms hold from last actual detection timestamp, followed by a time-based smooth transition to bottom-right; reacquisition follows smoothly. Deterministic tracking probe and isolated desktop compilation passed; renderer integration/package validation is a separate pending stage.

`tracking.h` now owns FaceBox/Anchor, independent of renderer. API retains x/y/tracking/seen and adds visible, AnchorMode {Searching,Tracking,Holding,Fallback,Manual}; footprint180×86 scaled by size. During hold tracking=false but visible=true, including with corner fallback disabled. At timeout fallback-disabled hides; manual placement retains offsets/clamp. First detection initializes anchor; subsequent detections and reacquisition use elapsed-time exponential smoothing. Corner transition tau350ms; no position jump at exactly2000ms. Low-frame-rate stepping accounts only time beyond timeout, producing the same fallback position as frequent updates.

FaceWorker returns actual completed-detection timestamp, cached results expire after500ms (previous1500). Producer passes timestamp to Anchor, so repeated cached bbox reads do not extend hold. Producer publishes mode in existing state tracking integer; desktop only labels mode1 as face found, mode2 as recovery/hold. Hold anchor remains draggable; dragging without an active detected face intentionally selects manual placement. Renderer owns compositor.h and consumes Anchor.visible; file not edited/staged in this task.

Checks: VS2022 x64 cl /std:c++17 /EHsc /MT directshow-camera\tracking_probe.cpp /Fodirectshow-camera\out\tracking_probe.obj /Fedirectshow-camera\out\tracking-probe.exe, then tracking-probe.exe PASS. Cases: loss0/1900/2000/2100ms, exact boundary no movement, convergence bottom-right, reacquisition before timeout, restarted grace, hide, manual/clamp, cached timestamp not refreshed, no initial detection,2500ms low-FPS step equals100ms steps. Separate BH_ISOLATED_TEST Windows-subsystem desktop compilation passed. Build script includes tracking probe target.

No full producer/renderer build or new archive claim yet: renderer agent is concurrently implementing compositor, with shared API agreed. No physical camera, user app inputs/process control, registry or privacy changes. Existing package remains previous UX checkpoint, not final integrated release.
