#!/usr/bin/env python3
"""
Pre-build check: verify that all CJK characters used in translation YAML files
are present in the built-in CJK UI font header (cjk_ui_font_20.h).

If missing characters are found, the build fails with an actionable error message.

Usage (standalone):
    python3 check_cjk_ui_font.py

Usage (PlatformIO pre-build):
    Added automatically via platformio.ini extra_scripts.
"""

import glob
import re
import sys
from pathlib import Path

try:
    import yaml
except ImportError:
    print("Warning: PyYAML not installed, skipping CJK UI font check")
    sys.exit(0)


def extract_cjk_from_translations(translations_dir):
    """Extract all CJK characters (U+3000+) from translation YAML files."""
    chars = set()
    for path in sorted(glob.glob(str(Path(translations_dir) / "*.yaml"))):
        with open(path, "r", encoding="utf-8") as f:
            data = yaml.safe_load(f)
        for key, value in data.items():
            if key.startswith("_"):
                continue
            for c in str(value):
                if ord(c) >= 0x3000:
                    chars.add(c)
    return chars


def extract_codepoints_from_header(header_path):
    """Extract codepoints from CJK_UI_CODEPOINTS array in the header file."""
    codepoints = set()
    with open(header_path, "r", encoding="utf-8") as f:
        content = f.read()
    for match in re.finditer(r"0x([0-9A-Fa-f]{4})", content):
        codepoints.add(int(match.group(1), 16))
    return codepoints


def check(project_root):
    translations_dir = project_root / "lib" / "I18n" / "translations"
    header_path = project_root / "lib" / "GfxRenderer" / "cjk_ui_font_20.h"

    if not translations_dir.is_dir():
        return True
    if not header_path.exists():
        return True

    translation_chars = extract_cjk_from_translations(translations_dir)
    header_codepoints = extract_codepoints_from_header(header_path)

    missing = []
    for c in sorted(translation_chars, key=ord):
        if ord(c) not in header_codepoints:
            missing.append(c)

    if missing:
        print(f"\n*** CJK UI Font Check Failed ***")
        print(f"{len(missing)} characters used in translations are missing from cjk_ui_font_20.h:\n")
        print("  " + "".join(missing))
        print(f"\nRun the following to regenerate:")
        print(f"  python3 scripts/generate_cjk_ui_font.py --size 20 --font <path-to-font.otf>")
        print()
        return False

    return True


def main():
    project_root = Path(__file__).parent.parent
    if check(project_root):
        print("CJK UI font check: OK")
    else:
        sys.exit(1)


if __name__ == "__main__":
    main()
else:
    try:
        Import("env")
        _project_root = Path(env.subst("$PROJECT_DIR"))
        if not check(_project_root):
            Import("env")
            env.Exit(1)
    except NameError:
        pass
