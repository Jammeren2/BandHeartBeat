# Background-mode controls checkpoint

## Summary
Added native «Без фона» toggle and CameraControls.transparent_background with sanitization,56-byte schemaV2 and distinct production/isolatedV2 mapping names. Renderer notified field ready. This checkpoint is not a final packaged/validated transparent-mode release.

Settings files accept version2 only; old version1 files reset safely to defaults. Producer already reads the same CameraControls and passes it to Compose, so no parallel UI-only settings path. Desktop slider rows shifted down to make room; full tracking/connection information stays in native UI, including a BPM/DEMO/disconnected pill. Renderer retains180×86 footprint and will draw compact180×60 panel or foreground only, with icon/number and minimal DEMO label.

Before scope changed, integrated tracking+first renderer build passed: tracking boundary/lowFPS tests,20render cases, control-to-published-pixel probe, producer selftest, DirectShow real allocator/Receive with restart and invalidIPC fallback, extracted production-namespace mock sequence progression, extracted backend demo72 0 1, desktop startup without capture and owned child cleanup. No production mappings or related processes were present on passive baseline. No physical camera, user process termination, registration or privacy settings touched. These earlier checks do NOT validate the new background-free schema/UI; compilation/package paused while renderer edits proceeded.

The existing desktop-v3 ZIP is an intermediate previous-renderer checkpoint, not deliverable for the updated request. Final archive/hash and updated project status will be produced in the next integration stage after48case renderer PASS. No final GUI/Discord/live-tracking acceptance claimed.
