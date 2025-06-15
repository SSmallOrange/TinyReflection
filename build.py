import os
import shutil
import subprocess
from pathlib import Path
def needs_reconfigure(build_dir: Path) -> bool:
    cmake_cache = build_dir / "CMakeCache.txt"
    cmakelists = Path(__file__).parent / "CMakeLists.txt"
    
    if not build_dir.exists() or not cmake_cache.exists():
        return True
    
    return cmakelists.stat().st_mtime > cmake_cache.stat().st_mtime

def main():
    project_root = Path(__file__).parent
    build_dir = project_root / "build"
    config = "Release"

    if needs_reconfigure(build_dir):
        print("Generating CMake project...")
        build_dir.mkdir(exist_ok=True)
        subprocess.run([
            "cmake",
            "-G", "Visual Studio 17 2022",
            "-A", "x64",
            "-S", str(project_root),
            "-B", str(build_dir),
        ], check=True)
    else:
        print("CMake is up to date, skipping generation.")

    print("Building with MSBuild...")
    subprocess.run([
        "msbuild",
        str(build_dir / "tinyrefl.sln"),
        f"/p:Configuration={config}",
        "/p:Platform=x64",
        "/m"
    ], check=True)

    print("Success!")

if __name__ == "__main__":
    main()