"""Generate GET_MEMBER_TUPLE_HELPER macro expansions.

This is a one-off code generator: its output
(`tinyrefl/utils/reflection_get_member_tuple_helper.hpp`) is committed to the
repository and consumed directly by the normal build. You only need to
re-run this script if you want to raise `max_count` (the maximum supported
struct member count) or change the macro expansion logic itself.

Usage (from anywhere, path resolution is independent of cwd):
    python3 scripts/generate_macro.py
"""
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent
OUTPUT_FILE = (
    PROJECT_ROOT / "tinyrefl" / "utils" / "reflection_get_member_tuple_helper.hpp"
)


def generate_macro_line(n: int) -> str:
    members = [f"m{i + 1}" for i in range(n)]
    return f"GET_MEMBER_TUPLE_HELPER({n}, {', '.join(members)})"


def generate_header_file(output_file: Path = OUTPUT_FILE, max_count: int = 255):
    output_file.parent.mkdir(parents=True, exist_ok=True)
    with output_file.open("w", encoding="utf-8") as f:
        f.write("// Auto-generated macro expansion for GET_MEMBER_TUPLE_HELPER\n")
        f.write("#pragma once\n\n")
        for i in range(1, max_count + 1):
            f.write(generate_macro_line(i) + "\n")


if __name__ == "__main__":
    generate_header_file()
    print(f"Generated {OUTPUT_FILE}")
