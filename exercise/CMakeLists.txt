# 设置CMake最低版本要求
cmake_minimum_required(VERSION 3.8)

# 定义项目名称
project("exercise")

# 包含用于检测头文件的CMake模块
include(CheckIncludeFile)
include(CheckIncludeFileCXX)
include(CheckLibraryExists)
include(CheckCXXCompilerFlag)

# 查找LLVM库，并且是必需的（REQUIRED）
find_package(LLVM REQUIRED CONFIG)
# 打印找到的LLVM版本和构建类型
message("Found LLVM ${LLVM_PACKAGE_VERSION}, build type ${LLVM_BUILD_TYPE}")

# 添加LLVM目录到CMake模块路径
list(APPEND CMAKE_MODULE_PATH ${LLVM_DIR})
# 包含检测GCC兼容性的CMake模块
include(DetermineGCCCompatible)
# 如果需要，可以包含选择MSVC CRT的CMake模块
# include(ChooseMSVCCRT)

# 添加LLVM定义到编译器选项
add_definitions(${LLVM_DEFINITIONS})
# 将LLVM的包含目录设为系统目录（SYSTEM），避免产生警告
include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})
# 将LLVM组件映射到库名称
llvm_map_components_to_libnames(llvm_libs Core)

# 如果编译器是GCC兼容的
if(LLVM_COMPILER_IS_GCC_COMPATIBLE)
  # 如果LLVM没有启用RTTI（运行时类型识别）
  if(NOT LLVM_ENABLE_RTTI)
    # 添加编译选项以禁用RTTI
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
  endif()
  # 如果LLVM没有启用异常处理
  if(NOT LLVM_ENABLE_EH)
    # 添加编译选项以禁用异常处理
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")
  endif()
endif()

# 添加子目录“src”，其中包含项目的源代码
add_subdirectory("src")
