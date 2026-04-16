# =============================================================================
#  demo/CMakeLists.txt
#  演示程序构建
#
#  本目录不属于 sdk_version 模块本身，仅用于展示集成用法。
#  实际集成时只需复制 version/ 目录，demo/ 可以删除。
# =============================================================================

# ── 公共编译选项 ──────────────────────────────────────────────────────────────
if(MSVC)
    set(DEMO_COMPILE_OPTIONS /W4)
else()
    set(DEMO_COMPILE_OPTIONS -Wall -Wextra -Wpedantic -Wno-unused-parameter)
endif()

# ── 各 Demo 目标 ──────────────────────────────────────────────────────────────

# 场景1：最简集成（上电打印）
add_executable(demo_basic demo_basic.c)
target_link_libraries(demo_basic PRIVATE sdk_version)
target_compile_options(demo_basic PRIVATE ${DEMO_COMPILE_OPTIONS})

# 场景2：OTA 升级版本兼容性校验
add_executable(demo_ota demo_ota.c)
target_link_libraries(demo_ota PRIVATE sdk_version)
target_compile_options(demo_ota PRIVATE ${DEMO_COMPILE_OPTIONS})

# 场景3：编译期版本守卫 & 运行时检查
add_executable(demo_guard demo_guard.c)
target_link_libraries(demo_guard PRIVATE sdk_version)
target_compile_options(demo_guard PRIVATE ${DEMO_COMPILE_OPTIONS})

# 场景4：现场故障快速回溯
add_executable(demo_fault demo_fault.c)
target_link_libraries(demo_fault PRIVATE sdk_version)
target_compile_options(demo_fault PRIVATE ${DEMO_COMPILE_OPTIONS})

message(STATUS "[sdk_version] Demo 目标已注册：demo_basic / demo_ota / demo_guard / demo_fault")
