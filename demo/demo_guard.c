/**
 * @file  demo_guard.c
 * @brief 场景3：编译期 API 版本守卫与运行时版本上报
 *
 * 演示三种版本守卫用法：
 *   A. SDK_VERSION_AT_LEAST 宏 —— 编译期条件编译，按版本选择 API 实现
 *   B. sdk_version_get_code()  —— 运行时版本比较，动态分支
 *   C. SDK_ASSERT_CLEAN_BUILD  —— 编译期洁净度断言（C11，量产敏感文件专用）
 *
 * 典型场景：你的 SDK 对外提供给第三方集成商，集成商在自己代码里用
 * SDK_VERSION_AT_LEAST 做新旧接口的兼容适配。
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "sdk_version.h"

/* ── C. 洁净度断言（可选，放在对版本回溯要求严格的文件顶部） ────────────── */
/* 仅在 C11 编译器 + MP 通道 CLEAN 构建时触发静态断言，其余情况无副作用    */
SDK_ASSERT_CLEAN_BUILD

/* ═══════════════════════════════════════════════════════════════════════════
 *  A. SDK_VERSION_AT_LEAST —— 编译期条件编译
 *
 *  场景：SDK 1.1.0 新增了 fast_process() 高性能接口，1.0.x 只有 slow_process()。
 *  集成商代码在编译时自动选择可用的最优实现，无需运行时判断。
 * ═══════════════════════════════════════════════════════════════════════════ */

/* 模拟 SDK 提供的两代接口 */
static void slow_process(const char *data)
{
    printf("  [兼容接口] slow_process: %s\n", data);
}

#if defined(__GNUC__) || defined(__clang__)
__attribute__((unused))
#endif
static void fast_process(const char *data)
{
    printf("  [新接口]   fast_process: %s\n", data);
}

static void demo_compile_time_guard(void)
{
    printf("\n── A. 编译期 API 版本守卫 ──────────────────────\n");
    printf("  当前 SDK 版本码: 0x%06X\n", (unsigned int)SDK_VERSION_CODE);

#if SDK_VERSION_IF(1, 1, 0)
    /* SDK >= 1.1.0：使用新的高性能接口 */
    printf("  编译分支: SDK >= 1.1.0，使用 fast_process()\n");
    fast_process("sensor_data_frame");
#else
    /* SDK < 1.1.0：回退到兼容接口 */
    printf("  编译分支: SDK < 1.1.0，使用 slow_process()\n");
    slow_process("sensor_data_frame");
#endif
    /* 注意：未被选中的分支在编译时直接被裁剪，零运行时开销 */
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  B. sdk_version_get_code() —— 运行时版本比较
 *
 *  场景：诊断工具连接到 ECU 后，检查 SDK 版本是否满足诊断协议要求，
 *  并针对不同版本区间采用不同的诊断命令集。
 * ═══════════════════════════════════════════════════════════════════════════ */

/* 诊断协议版本要求 */
#define DIAG_REQUIRE_MIN_VERSION    0x010000U   /* 最低要求 1.0.0 */
#define DIAG_ENHANCED_MIN_VERSION   0x010200U   /* 增强诊断需 1.2.0 */

static void demo_runtime_version_check(void)
{
    uint32_t code = sdk_version_get_code();

    printf("\n── B. 运行时版本比较（诊断协议适配）────────────\n");
    printf("  运行时版本码: 0x%06X\n", (unsigned int)code);

    /* 最低版本门槛检查 */
    if (code < DIAG_REQUIRE_MIN_VERSION) {
        printf("  ✗ 版本过低，最低要求 v1.0.0，当前 %s，拒绝诊断连接\n",
               sdk_version_get_str());
        return;
    }

    /* 按版本区间选择诊断命令集 */
    if (code >= DIAG_ENHANCED_MIN_VERSION) {
        printf("  ✓ SDK >= 1.2.0，启用增强诊断命令集\n");
        printf("  → 发送 UDS 0x22 0xF1A0（扩展版本DID）\n");
    } else {
        printf("  ✓ SDK 1.0.x ~ 1.1.x，使用基础诊断命令集\n");
        printf("  → 发送 UDS 0x22 0xF189（标准版本DID）\n");
    }

    /* 上报完整版本信息到诊断主机 */
    printf("  → 版本横幅上报: %s\n", sdk_version_get_banner());
    printf("  → Git Hash 上报: %s（用于精确回溯）\n", sdk_version_get_git_hash());
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  C. 发布通道守卫
 *
 *  场景：在生产烧录工具中，拒绝将非 MP 通道产物烧录到量产板。
 * ═══════════════════════════════════════════════════════════════════════════ */

static void demo_channel_guard(void)
{
    const char *channel = sdk_version_get_channel();

    printf("\n── C. 发布通道守卫（量产烧录保护）──────────────\n");
    printf("  当前通道: %s\n", channel);
    printf("  洁净状态: %s\n",
           sdk_version_is_clean() ? "CLEAN ✓" : "DIRTY ✗");

    if (strcmp(channel, "mp") != 0) {
        printf("  ✗ 非 MP 通道产物，量产烧录工具应拒绝此固件\n");
        printf("    （当前为 %s 通道，仅 mp 通道允许量产上车）\n", channel);
        return;
    }

    if (!sdk_version_is_clean()) {
        printf("  ✗ 工作区不洁净，此情况在 mp 通道下不应出现\n");
        printf("    （CMake 层已有 FATAL_ERROR 保护，此为运行时双重校验）\n");
        return;
    }

    printf("  ✓ MP 通道 + CLEAN 构建，允许量产烧录\n");
    printf("  → 写入版本绑定记录: VIN:LSVXXXXXXXX%s\n",
           sdk_version_get_git_hash());
}

/* ─────────────────────────────────────────────────────────────────────────── */

int main(void)
{
    printf("========================================\n");
    printf("  Demo: 编译期守卫 & 运行时版本检查\n");
    printf("========================================\n");

    sdk_version_print();

    demo_compile_time_guard();
    demo_runtime_version_check();
    demo_channel_guard();

    printf("\n========================================\n");
    return 0;
}
