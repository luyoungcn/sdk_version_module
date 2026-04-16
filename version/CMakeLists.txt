# =============================================================================
#  version/CMakeLists.txt
#  SDK 版本信息自动注入模块
#
#  职责：
#    1. 从 Git 采集所有构建元数据
#    2. 校验 MP Release 洁净度（脏工作区直接 FATAL_ERROR）
#    3. 按通道规则组装完整版本字符串
#    4. 生成 version_auto.h（构建产物，禁止手动编辑）
#    5. 编译 sdk_version 静态库并导出公共头文件路径
# =============================================================================

# ── 1. 采集 Git 元数据 ────────────────────────────────────────────────────────

execute_process(
    COMMAND git rev-parse HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE   GIT_HASH_FULL
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

execute_process(
    COMMAND git rev-parse --short=8 HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE   GIT_HASH_SHORT
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

execute_process(
    COMMAND git rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE   GIT_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

execute_process(
    COMMAND git status --porcelain
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE   _GIT_STATUS_RAW
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

execute_process(
    COMMAND git describe --tags --abbrev=0
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE   GIT_LAST_TAG
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

execute_process(
    COMMAND git rev-list "${GIT_LAST_TAG}..HEAD" --count
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE   GIT_COMMITS_SINCE_TAG
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

# 洁净度判断
if("${_GIT_STATUS_RAW}" STREQUAL "")
    set(GIT_DIRTY_FLAG "CLEAN")
else()
    set(GIT_DIRTY_FLAG "DIRTY")
endif()

# 兜底：非 Git 仓库环境（离线构建、源码压缩包解压等）
if("${GIT_HASH_FULL}" STREQUAL "")
    set(GIT_HASH_FULL          "0000000000000000000000000000000000000000")
    set(GIT_HASH_SHORT         "00000000")
    set(GIT_BRANCH             "UNKNOWN")
    set(GIT_DIRTY_FLAG         "UNKNOWN")
    set(GIT_LAST_TAG           "v0.0.0")
    set(GIT_COMMITS_SINCE_TAG  "0")
    message(WARNING
        "[sdk_version] 未检测到 Git 仓库，所有 Git 字段将使用占位符。"
    )
endif()

if("${GIT_COMMITS_SINCE_TAG}" STREQUAL "")
    set(GIT_COMMITS_SINCE_TAG "0")
endif()

# ── 2. MP Release 洁净度强制校验 ──────────────────────────────────────────────
#
#  仅当 SDK_CHANNEL=mp 时触发。
#  dev / rc 通道不受限制，允许随时在脏工作区编译验证。

if("${SDK_CHANNEL}" STREQUAL "mp" AND "${GIT_DIRTY_FLAG}" STREQUAL "DIRTY")
    message(FATAL_ERROR
        "\n"
        "  [sdk_version] *** MP Release 构建被阻断 ***\n"
        "\n"
        "  当前工作区存在未提交的修改（GIT_DIRTY_FLAG = DIRTY）。\n"
        "  量产版本必须从洁净工作区构建，以确保版本可精确回溯。\n"
        "\n"
        "  解决方法：\n"
        "    git add . && git commit -m \"your message\"   # 提交修改\n"
        "    git stash                                     # 或暂存修改\n"
        "\n"
        "  若仅需本地调试，请改用 dev 通道：\n"
        "    cmake -B build -DSDK_CHANNEL=dev\n"
    )
endif()

# ── 3. 构建时间戳 ─────────────────────────────────────────────────────────────

string(TIMESTAMP BUILD_DATE_STR      "%Y%m%d"          UTC)
string(TIMESTAMP BUILD_TIMESTAMP_UTC "%Y%m%dT%H%M%SZ"  UTC)

# ── 4. 组装版本字符串（严格遵循 semver 2.0） ──────────────────────────────────
#
#  mp  通道：1.0.0
#  rc  通道：1.0.0-rc.{commits_since_tag}
#  dev 通道：1.0.0-dev.{YYYYMMDD}
#  脏构建附加 +dirty（仅 dev/rc）

set(_BASE "${SDK_VERSION_MAJOR}.${SDK_VERSION_MINOR}.${SDK_VERSION_PATCH}")

if("${SDK_CHANNEL}" STREQUAL "mp")
    set(SDK_VERSION_STRING "${_BASE}")

elseif("${SDK_CHANNEL}" STREQUAL "rc")
    set(SDK_VERSION_STRING "${_BASE}-rc.${GIT_COMMITS_SINCE_TAG}")
    if("${GIT_DIRTY_FLAG}" STREQUAL "DIRTY")
        set(SDK_VERSION_STRING "${SDK_VERSION_STRING}+dirty")
    endif()

else()
    # dev（以及任何未知通道，保守处理）
    set(SDK_VERSION_STRING "${_BASE}-dev.${BUILD_DATE_STR}")
    if("${GIT_DIRTY_FLAG}" STREQUAL "DIRTY")
        set(SDK_VERSION_STRING "${SDK_VERSION_STRING}+dirty")
    endif()
endif()

# ── 5. 组装完整 Banner（含平台兼容性范围，可选） ──────────────────────────────

if(NOT "${SDK_HW_COMPAT_MIN}" STREQUAL "" AND NOT "${SDK_HW_COMPAT_MAX}" STREQUAL "")
    set(_HW_PART " HW:${SDK_HW_COMPAT_MIN}-${SDK_HW_COMPAT_MAX}")
else()
    set(_HW_PART "")
endif()

set(SDK_VERSION_BANNER
    "${SDK_PRODUCT_NAME}/${SDK_VERSION_STRING}${_HW_PART} GIT:${GIT_HASH_SHORT}")

# ── 6. 数字版本码（0xMMIIPP） ─────────────────────────────────────────────────

math(EXPR SDK_VERSION_CODE_DEC
    "(${SDK_VERSION_MAJOR} << 16) | (${SDK_VERSION_MINOR} << 8) | ${SDK_VERSION_PATCH}"
)

# ── 7. 编译器标识 ─────────────────────────────────────────────────────────────

if(NOT "${CMAKE_C_COMPILER_VERSION}" STREQUAL "")
    set(SDK_TOOLCHAIN_VER "${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")
else()
    set(SDK_TOOLCHAIN_VER "UNKNOWN")
endif()

# ── 8. 生成 version_auto.h ────────────────────────────────────────────────────

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/version_auto.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/version_auto.h
    @ONLY
)

# ── 9. 编译静态库并导出头文件路径 ────────────────────────────────────────────

add_library(sdk_version STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/sdk_version.c
)

target_include_directories(sdk_version
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}   # sdk_version.h 对外可见
        ${CMAKE_CURRENT_BINARY_DIR}   # version_auto.h（生成文件）
)

# ── 10. 构建摘要（CI 日志核查用） ────────────────────────────────────────────

message(STATUS "┌─────────────────────────────────────────────")
message(STATUS "│ [sdk_version] PRODUCT      : ${SDK_PRODUCT_NAME}")
message(STATUS "│ [sdk_version] VERSION      : ${SDK_VERSION_STRING}")
message(STATUS "│ [sdk_version] BANNER       : ${SDK_VERSION_BANNER}")
message(STATUS "│ [sdk_version] GIT_HASH     : ${GIT_HASH_SHORT}")
message(STATUS "│ [sdk_version] GIT_BRANCH   : ${GIT_BRANCH}")
message(STATUS "│ [sdk_version] GIT_DIRTY    : ${GIT_DIRTY_FLAG}")
message(STATUS "│ [sdk_version] CHANNEL      : ${SDK_CHANNEL}")
message(STATUS "│ [sdk_version] VERSION_CODE : 0x${SDK_VERSION_CODE_DEC}")
message(STATUS "│ [sdk_version] TOOLCHAIN    : ${SDK_TOOLCHAIN_VER}")
message(STATUS "└─────────────────────────────────────────────")
