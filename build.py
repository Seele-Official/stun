import argparse
import subprocess
import os

def run_cmake(project, build_dir, cmake_args, rebuild):
    source_dir = os.path.join(os.getcwd(), project)
    build_path = os.path.join(os.getcwd(), build_dir)
    
    if rebuild and os.path.exists(build_path):
        print(f"Removing existing build directory: {build_path}")
        subprocess.run(["rm", "-r", build_path], check=True)
    
    os.makedirs(build_path, exist_ok=True)
    
    # 运行 CMake 配置
    cmake_command = ["cmake", "-S", source_dir, "-B", build_path, "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"] + cmake_args
    print("Running:", " ".join(cmake_command))
    subprocess.run(cmake_command, check=True)
    
    # 运行 CMake 构建
    build_command = ["cmake", "--build", build_path]
    print("Running:", " ".join(build_command))
    subprocess.run(build_command, check=True)

def main():
    parser = argparse.ArgumentParser(description="CMake Project Build Manager")
    parser.add_argument("project", choices=["stun-client", "stun-server"], help="Project to build")
    parser.add_argument("--build-dir", default="build", help="Custom build directory")
    parser.add_argument("--cmake-args", nargs=argparse.REMAINDER, help="Additional CMake arguments")
    parser.add_argument("--rebuild", action="store_true", help="Clean and rebuild the project")
    
    args = parser.parse_args()
    run_cmake(args.project, args.build_dir, args.cmake_args or [], args.rebuild)
    
if __name__ == "__main__":
    main()