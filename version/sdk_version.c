/**
 * @file    sdk_version.c
 * @brief   SDK 版本管理实现
 *
 * @details
 *   版本信息结构体通过 VERSION_SECTION 属性固定存储于二进制的
 *   .rodata.version 段，链接脚本须保留该段（KEEP），以确保：
 *     - JTAG / SWD 调试器可在不运行代码的情况下直接读取版本信息
 *     - OTA 工具可从 Flash 固定偏移地址解析版本
 *     - strings 命令可从裸二进制中提取版本字符串
 *
 *   段属性兼容性：
 *     GCC / Clang : __attribute__((section / used))
 *     MSVC        : __declspec(allocate)  + #pragma section
 *     其他编译器  : 退化为普通 const（功能完整，仅失去 Flash 定位能力）
 */

#include "sdk_version.h"
#include "version_auto.h"

#include <stdio.h>
#include <string.h>

/* ── 段属性定义 ──────────────────────────────────────────────────────────── */

#if defined(__GNUC__) || defined(__clang__)
#  define VERSION_SECTION \
     __attribute__((section(".rodata.version"))) \
     __attribute__((used))
#elif defined(_MSC_VER)
#  pragma section(".rodata.version", read)
#  define VERSION_SECTION  __declspec(allocate(".rodata.version"))
#else
#  define VERSION_SECTION  /* 退化为普通 const */
#endif

/* ── 全局只读版本实例 ────────────────────────────────────────────────────── */

VERSION_SECTION
static const SdkVersionInfo_t s_sdk_version = {
    .product_name    = SDK_PRODUCT_NAME,
    .version_str     = SDK_VERSION_STRING,
    .version_banner  = SDK_VERSION_BANNER,
    .git_hash_full   = SDK_GIT_HASH_FULL,
    .git_hash_short  = SDK_GIT_HASH_SHORT,
    .git_branch      = SDK_GIT_BRANCH,
    .git_dirty       = SDK_GIT_DIRTY_FLAG,
    .build_timestamp = SDK_BUILD_TIMESTAMP,
    .build_date      = SDK_BUILD_DATE,
    .toolchain_ver   = SDK_TOOLCHAIN_VER,
    .channel         = SDK_CHANNEL,
    .hw_compat_min   = SDK_HW_COMPAT_MIN,
    .hw_compat_max   = SDK_HW_COMPAT_MAX,
    .version_code    = SDK_VERSION_CODE,
};

/* ── API 实现 ────────────────────────────────────────────────────────────── */

const SdkVersionInfo_t *sdk_version_get_info(void)
{
    return &s_sdk_version;
}

const char *sdk_version_get_str(void)
{
    return s_sdk_version.version_str;
}

const char *sdk_version_get_banner(void)
{
    return s_sdk_version.version_banner;
}

const char *sdk_version_get_git_hash(void)
{
    return s_sdk_version.git_hash_short;
}

uint32_t sdk_version_get_code(void)
{
    return s_sdk_version.version_code;
}

int sdk_version_is_clean(void)
{
    /* 用首字符比较，避免 strcmp，符合 MISRA C 字符串比较约束 */
    return (s_sdk_version.git_dirty[0] == 'C') ? 1 : 0;
}

const char *sdk_version_get_channel(void)
{
    return s_sdk_version.channel;
}

void sdk_version_print(void)
{
    const SdkVersionInfo_t *v = &s_sdk_version;
    int has_hw_compat;

    /* ── 人类可读：固定列宽对齐表格 ─────────────────────────────────────── */
    printf("================================================\n");
    printf("           SDK Version Info\n");
    printf("================================================\n");
    printf("  %-16s: %s\n", "PRODUCT",       v->product_name);
    printf("  %-16s: %s\n", "VERSION",        v->version_str);
    printf("  %-16s: %s\n", "BANNER",         v->version_banner);
    printf("  %-16s: %s\n", "GIT_HASH",       v->git_hash_short);
    printf("  %-16s: %s\n", "GIT_BRANCH",     v->git_branch);
    printf("  %-16s: %s\n", "DIRTY",          v->git_dirty);
    printf("  %-16s: %s\n", "BUILD_TS",       v->build_timestamp);
    printf("  %-16s: %s\n", "TOOLCHAIN",      v->toolchain_ver);
    printf("  %-16s: %s\n", "CHANNEL",        v->channel);

    has_hw_compat = (v->hw_compat_min != NULL && v->hw_compat_min[0] != '\0');
    if (has_hw_compat) {
        printf("  %-16s: %s ~ %s\n", "HW_COMPAT",
               v->hw_compat_min, v->hw_compat_max);
    }

    printf("  %-16s: 0x%06X\n", "VERSION_CODE", (unsigned int)v->version_code);
    printf("================================================\n");

    /* ── 机器可读：单行 JSON（字段顺序固定，便于正则 / jq 解析） ──────────
     *
     *  hw_compat 字段：仅在配置了 HW_COMPAT 时输出，桌面 SDK 不产生该字段。
     */
    if (has_hw_compat) {
        printf(
            "{\"event\":\"sdk_init\","
            "\"product\":\"%s\","
            "\"version\":\"%s\","
            "\"git_hash\":\"%s\","
            "\"git_branch\":\"%s\","
            "\"dirty\":\"%s\","
            "\"build_ts\":\"%s\","
            "\"channel\":\"%s\","
            "\"hw_compat\":\"%s-%s\","
            "\"version_code\":\"0x%06X\"}\n",
            v->product_name,
            v->version_str,
            v->git_hash_short,
            v->git_branch,
            v->git_dirty,
            v->build_timestamp,
            v->channel,
            v->hw_compat_min,
            v->hw_compat_max,
            (unsigned int)v->version_code
        );
    } else {
        printf(
            "{\"event\":\"sdk_init\","
            "\"product\":\"%s\","
            "\"version\":\"%s\","
            "\"git_hash\":\"%s\","
            "\"git_branch\":\"%s\","
            "\"dirty\":\"%s\","
            "\"build_ts\":\"%s\","
            "\"channel\":\"%s\","
            "\"version_code\":\"0x%06X\"}\n",
            v->product_name,
            v->version_str,
            v->git_hash_short,
            v->git_branch,
            v->git_dirty,
            v->build_timestamp,
            v->channel,
            (unsigned int)v->version_code
        );
    }
}
