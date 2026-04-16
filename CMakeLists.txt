cmake_minimum_required(VERSION 3.10)
project(sdk_version_module C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# =============================================================================
#  用户配置区（可通过 cmake -DXXX=yyy 命令行覆盖）
# =============================================================================

# ── 版本三元组 ────────────────────────────────────────────────────────────────
set(SDK_VERSION_MAJOR "0" CACHE STRING "SDK major version")
set(SDK_VERSION_MINOR "0" CACHE STRING "SDK minor version")
set(SDK_VERSION_PATCH "2" CACHE STRING "SDK patch version")

# ── 产品标识（对外打印用，替换为你的产品名，如 MY-SDK / FOO-LIB） ──────────
set(SDK_PRODUCT_NAME  "MY-SDK" CACHE STRING "Product name shown in version banner")

# ── 发布通道：dev | rc | mp ──────────────────────────────────────────────────
#    dev : 日常开发，允许脏工作区，版本号附加 +dirty
#    rc  : 集成测试候选，允许脏工作区，版本号附加 +dirty
#    mp  : 量产发布，脏工作区直接 FATAL_ERROR 阻断构建
set(SDK_CHANNEL "dev" CACHE STRING "Release channel: dev | rc | mp")

# ── 可选：硬件 / 平台兼容性标识（嵌入式场景专用，桌面项目留空即可） ────────
set(SDK_HW_COMPAT_MIN "" CACHE STRING "Min compatible HW/platform version (optional)")
set(SDK_HW_COMPAT_MAX "" CACHE STRING "Max compatible HW/platform version (optional)")

# =============================================================================
#  子模块与测试目标
# =============================================================================

add_subdirectory(version)

add_executable(version_test test/test_main.c)
target_link_libraries(version_test PRIVATE sdk_version)

if(MSVC)
    target_compile_options(version_test PRIVATE /W4)
else()
    target_compile_options(version_test PRIVATE
        -Wall -Wextra -Wpedantic -Wno-unused-parameter
    )
endif()
