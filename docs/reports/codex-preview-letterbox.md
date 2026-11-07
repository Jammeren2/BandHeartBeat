# Preview letterbox fix

The native preview now uses an explicit contain calculation for the fixed 640x480 transport. It centers the image, paints any aspect-ratio difference as black bars, and maps tracking rectangles into the same contained destination. This prevents future widescreen sources from being silently cropped or stretching overlay coordinates. Horizontal mirror remains applied to the final preview frame and is swapped back after drawing, so virtual output semantics are unchanged.

Validation: `directshow-camera\\build-msvc.cmd` from repository root completed successfully (MSVC source, producer, desktop, probes). No camera, registry, Discord, or user process was touched. Virtual camera transport remains 640x480 BGRA. Commit: `48f031f`.

Live Discord and physical-camera aspect-ratio acceptance were not run.
