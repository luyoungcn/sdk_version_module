cmake_minimum_required(VERSION 3.10)
project(version_module_test C)

# 版本配置
set(SDK_VERSION_MAJOR 0)
set(SDK_VERSION_MINOR 0)
set(SDK_VERSION_PATCH 1)
set(SDK_HW_PLATFORM "intel")
set(SDK_BUILD_TYPE "Debug")

# 加入版本模块
add_subdirectory(version)

# 测试程序
add_executable(version_test test_main.c)

# 【关键修复】依赖版本模块，自动导入头文件路径
target_link_libraries(version_test PRIVATE sdk_version)
target_include_directories(version_test PRIVATE
    version
    ${CMAKE_CURRENT_BINARY_DIR}/version
)
