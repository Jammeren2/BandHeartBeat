# Compact overlay geometry integration

## Summary
Integrated shared126×86 geometry into tracking bounds,corner fallback,producer state and desktop state-driven preview/drag. Timedtracking and real isolated producer control/pixel/edge-state tests passed. Runtime source436a11a remains frozen for separate review; no ZIP in this stage.

Renderer b446373 owns geometry.h,CardWidth(size)/CardHeight(size),actual252×1722x surface (not horizontal bitmap squeeze). Width126 is70% of prior180,footprintheight86 unchanged. tracking now derives width/height from helpers for face-side choice,clamp,manual placement and bottom-right target (x502,y382 at100%). Producer publishes same helper-derived w/h,default CameraState matches. Controls ABI68bytes/schema3 unchanged. Desktop already derives preview bbox,drag offsets and hit anchor from CameraState; no hardcoded180width existed there,so producer/defaultstate changes propagate without duplicate UI geometry constants. Preview4:3 transform remains704/640and528/480.

Tests: installedVS2022SDK fullnative buildpassed; tracking-probe hold/reacquire/clamp/lowFPSequivalencePASS with compact fallback; controls-probe styles/offset/size/fallback/no-bg exactpixelPASS. Added real isolated producer maxsize140 manualfar-edge test reportsw176,h120andclampedbounds. renderer probe and contact sheet are renderer-author tests; accessible copied image docs/reports/screenshots/desktop-compact-width.png. Producer selftestFaceDetector1Gray8supported1syntheticbbox/compositorPASS. Testprocess stopped cleanly. No physicalcamera,userprocess,productionmapping,registry or Discord input changes.

No packageduserrelease or actualDiscordsuccessclaim. Parentcompatibilityfix remains pending independentreview and consumerretry; report docs/reports/codex-chromium-negotiation-fix.md is separate runtimeevidence. Olduserappstill uses previous geometry until explicit newrelease launch.
