import os
import shutil
import subprocess
import platform
from pathlib import Path

def needs_reconfigure(build_dir: Path) -> bool:
    cmake_cache = build_dir / "CMakeCache.txt"
    cmakelists = Path(__file__).parent / "CMakeLists.txt"
    if not build_dir.exists() or not cmake_cache.exists():
        return True
    return cmakelists.stat().st_mtime > cmake_cache.stat().st_mtime

def run_command(cmd, cwd=None):
    print("Running:", " ".join(cmd))
    subprocess.run(cmd, check=True, cwd=cwd)

def main():
    project_root = Path(__file__).parent.resolve()
    build_dir = project_root / "build"
    config = "Release"
    is_windows = platform.system() == "Windows"

    if needs_reconfigure(build_dir):
        print("Generating CMake project...")
        build_dir.mkdir(exist_ok=True)
        cmake_cmd = [
            "cmake",
            "-DCMAKE_BUILD_TYPE={}".format(config) if not is_windows else "",
            "-S", str(project_root),
            "-B", str(build_dir)
        ]

        if is_windows:
            cmake_cmd = [
                "cmake",
                "-G", "Visual Studio 17 2022",
                "-A", "x64",
                "-S", str(project_root),
                "-B", str(build_dir)
            ]
        
        run_command([arg for arg in cmake_cmd if arg])
    else:
        print("CMake is up to date, skipping generation.")

    if is_windows:
        print("Building with MSBuild...")
        run_command([
            "msbuild",
            str(build_dir / "tinyrefl.sln"),
            f"/p:Configuration={config}",
            "/p:Platform=x64",
            "/m"
        ])
    else:
        print("Building with Make...")
        run_command([
            "cmake", "--build", str(build_dir), "--config", config, "--parallel"
        ])

    print("✅ Build succeeded!")

if __name__ == "__main__":
    main()
