# IMFActivate wrapper generation attempt

Added deterministic `virtual-camera/generate_activate_wrapper.py` and generated `imf_activate_wrapper.generated.h/.cpp`. The script reads the installed SDK `mfobjects.h`, extracts the IMFActivate/IMFAttributes declaration region, and emits a compile-time ABI count. Current extraction found **16** method declarations with the simple header pattern; the SDK's multiline annotations prevent this regex from safely capturing the complete IMFAttributes vtable. The generated translation unit compiles in the isolated MSVC build and asserts the nonzero extraction count.

The helper build was corrected so the generated translation unit is compiled before the batch script exits. VS2022/SDK build succeeds, producing `virtual-camera/out/imf_activate_wrapper.generated.obj`. This is an ABI extraction sanity artifact, not yet a usable IMFActivate object: it intentionally contains no partial forwarding class, no unsafe cast, and does not alter the production factory. Attribute roundtrip, `QI(IID_IMFActivate)`, and `ActivateObject` were therefore not claimed.

No registry, activation, physical camera, privacy, HKLM or Discord operation occurred. Existing Rust/OBS behavior remains unchanged.

The exact remaining implementation blocker is a robust parser/generator for all SDK IMFAttributes methods (including multiline signatures) followed by a composition wrapper that forwards every method to `MFCreateAttributes` and implements the three IMFActivate methods. The current generated count is evidence of the parser's partial result and must not be treated as complete.

## Summary

Committed a deterministic SDK extraction script and compilable generated ABI sanity translation unit. It reports 16 parsed methods but correctly stops short of a partial IMFActivate wrapper; complete multiline signature generation remains required before activation.
