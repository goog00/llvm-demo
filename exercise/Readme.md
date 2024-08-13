
## 执行步骤：
### 方式1：通过cmake
1.修改src/CMakeLists.txt
add_executable(
    exercise
    # test.cpp
    # PhiCreate.cpp
    # Align.cpp
    CreateModule.cpp
)

2.进入到build目录，执行 
cmake -G Ninja  -DLT_LLVM_INSTALL_DIR=/usr/local/llvm17/ ../exercise

3.进入到build/src 目录，执行
./exercise

### 方式2： clang++
命令：clang++ -O3 CreateModule.cpp `llvm-config --cxxflags --ldflags --system-libs --libs core` -o toy


