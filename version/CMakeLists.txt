# 自动获取 Git 信息（兼容非Git仓库）
execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_SHORT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

execute_process(
    COMMAND git rev-parse HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

execute_process(
    COMMAND git rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

# 无 Git 时使用默认值
if(NOT GIT_SHORT_HASH)
    set(GIT_SHORT_HASH "00000000")
endif()

if(NOT GIT_HASH)
    set(GIT_HASH "00000000000000000000")
endif()

if(NOT GIT_BRANCH)
    set(GIT_BRANCH "no-git")
endif()

# 编译时间
string(TIMESTAMP BUILD_DATE "%Y-%m-%d")
string(TIMESTAMP BUILD_TIME "%H:%M:%S")

# 版本号计算
math(EXPR VERSION_CODE
    "${SDK_VERSION_MAJOR} << 16 | ${SDK_VERSION_MINOR} << 8 | ${SDK_VERSION_PATCH}"
)

# 生成版本头文件
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/version_auto.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/version_auto.h
    @ONLY
)

# 头文件路径
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# 编译静态库
add_library(sdk_version STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/sdk_version.c
)
