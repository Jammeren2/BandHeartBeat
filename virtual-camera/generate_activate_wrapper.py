from pathlib import Path
import re
sdk = Path(r"C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um\mfobjects.h")
s = sdk.read_text(errors="ignore")
start = s.index("IMFActivate : public")
block = s[s.rfind("IMFAttributes", 0, start):start]
names = list(dict.fromkeys(re.findall(r"virtual\s+[^;\n]+?\s+(\w+)\s*\(", block)))
out = Path(__file__).with_name("imf_activate_wrapper.generated.h")
out.write_text("// Generated from Windows SDK mfobjects.h; do not hand edit.\n#pragma once\n#include <mfobjects.h>\nstruct GeneratedActivateAbi { static constexpr unsigned method_count = %d; };\n" % len(names) + "// Methods extracted: " + ", ".join(names) + "\n")
print(f"extracted {len(names)} IMFAttributes methods -> {out}")
