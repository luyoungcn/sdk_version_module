/**
 * @file    test/test_main.c
 * @brief   SDK 版本模块独立测试程序
 *
 * @details
 *   完全独立运行，无任何硬件、OS 或第三方依赖（仅需 C99 标准库）。
 *
 *   测试用例：
 *     T01  上电规范打印（人类可读表格 + JSON 单行摘要）
 *     T02  所有 API 返回值非空校验
 *     T03  版本字符串 semver 格式合法性（无 'V' 前缀、Banner 前缀）
 *     T04  数字版本码与三元组一致性 + VERSION_AT_LEAST 宏验证
 *     T05  发布通道合法枚举值校验
 *     T06  sdk_version_is_clean() 返回值合法性
 *     T07  Git Hash 长度与字符集校验（短 8 位 / 完整 40 位）
 *     T08  构建时间戳与日期格式校验
 *     T09  HW_COMPAT 字段合法性（嵌入式：长度≥2；桌面：允许为空）
 *     T10  所有 API 与结构体字段交叉一致性
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "sdk_version.h"

/* ═══════════════════════════════════════════════════════════════════════════
 *  零依赖轻量测试框架
 * ═══════════════════════════════════════════════════════════════════════════ */

static int s_pass  = 0;
static int s_fail  = 0;
static int s_skip  = 0;

#define TEST_PASS(desc) \
    do { printf("  [PASS] %s\n", (desc)); s_pass++; } while (0)

#define TEST_FAIL(desc) \
    do { printf("  [FAIL] %s  (line %d)\n", (desc), __LINE__); s_fail++; } while (0)

#define TEST_SKIP(desc, reason) \
    do { printf("  [SKIP] %s  (%s)\n", (desc), (reason)); s_skip++; } while (0)

#define TEST_ASSERT(desc, cond) \
    do { if (cond) { TEST_PASS(desc); } else { TEST_FAIL(desc); } } while (0)

#define TEST_ASSERT_NONEMPTY(desc, str) \
    TEST_ASSERT(desc, (str) != NULL && (str)[0] != '\0')

#define TEST_SECTION(name) \
    printf("\n── %s ──\n", (name))

/* ── 辅助函数 ────────────────────────────────────────────────────────────── */

static int str_starts_with(const char *s, const char *prefix)
{
    if (s == NULL || prefix == NULL) { return 0; }
    return strncmp(s, prefix, strlen(prefix)) == 0;
}

static int is_hex_string(const char *s, size_t expected_len)
{
    size_t i;
    if (s == NULL || strlen(s) != expected_len) { return 0; }
    for (i = 0; i < expected_len; i++) {
        char c = s[i];
        int ok = ((c >= '0' && c <= '9') ||
                  (c >= 'a' && c <= 'f') ||
                  (c >= 'A' && c <= 'F'));
        if (!ok) { return 0; }
    }
    return 1;
}

static int is_digit_string(const char *s, size_t expected_len)
{
    size_t i;
    if (s == NULL || strlen(s) != expected_len) { return 0; }
    for (i = 0; i < expected_len; i++) {
        if (s[i] < '0' || s[i] > '9') { return 0; }
    }
    return 1;
}

static int is_valid_channel(const char *ch)
{
    if (ch == NULL) { return 0; }
    return (strcmp(ch, "dev") == 0 ||
            strcmp(ch, "rc")  == 0 ||
            strcmp(ch, "mp")  == 0);
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  main
 * ═══════════════════════════════════════════════════════════════════════════ */

int main(void)
{
    const SdkVersionInfo_t *ver;
    uint32_t code;
    uint32_t expected_code;
    int has_hw_compat;

    printf("╔══════════════════════════════════════════════╗\n");
    printf("║      SDK 版本模块独立测试程序                ║\n");
    printf("╚══════════════════════════════════════════════╝\n");

    /* ── T01 上电规范打印 ────────────────────────────────────────────────── */
    TEST_SECTION("T01  上电规范打印");
    printf("\n");
    sdk_version_print();

    /* ── T02 API 返回值非空 ──────────────────────────────────────────────── */
    TEST_SECTION("T02  API 返回值非空");

    ver = sdk_version_get_info();
    TEST_ASSERT("sdk_version_get_info() 非 NULL",    ver != NULL);
    TEST_ASSERT_NONEMPTY("sdk_version_get_str()    非空", sdk_version_get_str());
    TEST_ASSERT_NONEMPTY("sdk_version_get_banner() 非空", sdk_version_get_banner());
    TEST_ASSERT_NONEMPTY("sdk_version_get_git_hash() 非空", sdk_version_get_git_hash());
    TEST_ASSERT_NONEMPTY("sdk_version_get_channel() 非空",  sdk_version_get_channel());

    if (ver != NULL) {
        TEST_ASSERT_NONEMPTY("ver->product_name    非空",  ver->product_name);
        TEST_ASSERT_NONEMPTY("ver->git_hash_full   非空",  ver->git_hash_full);
        TEST_ASSERT_NONEMPTY("ver->git_branch      非空",  ver->git_branch);
        TEST_ASSERT_NONEMPTY("ver->git_dirty       非空",  ver->git_dirty);
        TEST_ASSERT_NONEMPTY("ver->build_timestamp 非空",  ver->build_timestamp);
        TEST_ASSERT_NONEMPTY("ver->build_date      非空",  ver->build_date);
        TEST_ASSERT_NONEMPTY("ver->toolchain_ver   非空",  ver->toolchain_ver);
    }

    /* ── T03 版本字符串格式 ──────────────────────────────────────────────── */
    TEST_SECTION("T03  版本字符串 semver 格式");
    {
        const char *vs = sdk_version_get_str();
        const char *bn = sdk_version_get_banner();

        /* semver：首字符必须是数字（无 'V' 前缀） */
        TEST_ASSERT("版本号首字符为数字（无 'V' 前缀）",
                    vs != NULL && vs[0] >= '0' && vs[0] <= '9');

        /* Banner 须包含产品名前缀 */
        if (ver != NULL && bn != NULL) {
            TEST_ASSERT("banner 以产品名开头",
                        str_starts_with(bn, ver->product_name));
        }

        /* Banner 须包含 '/' 分隔符 */
        TEST_ASSERT("banner 含 '/' 分隔符",
                    bn != NULL && strchr(bn, '/') != NULL);

        /* Banner 须包含 " GIT:" 字段 */
        TEST_ASSERT("banner 含 ' GIT:' 字段",
                    bn != NULL && strstr(bn, " GIT:") != NULL);

        /* mp 通道：版本号不含预发布标识符 */
        if (ver != NULL && strcmp(ver->channel, "mp") == 0) {
            TEST_ASSERT("mp 通道版本号不含 '-'（无预发布后缀）",
                        vs != NULL && strchr(vs, '-') == NULL);
        }

        /* dev 通道脏构建：版本号含 '+dirty' */
        if (ver != NULL &&
            strcmp(ver->channel, "dev") == 0 &&
            strcmp(ver->git_dirty, "DIRTY") == 0) {
            TEST_ASSERT("dev 脏构建版本号含 '+dirty'",
                        vs != NULL && strstr(vs, "+dirty") != NULL);
        }
    }

    /* ── T04 版本码一致性 ────────────────────────────────────────────────── */
    TEST_SECTION("T04  版本码与三元组一致性");
    {
        code = sdk_version_get_code();
        expected_code =
            ((uint32_t)SDK_VERSION_MAJOR << 16U) |
            ((uint32_t)SDK_VERSION_MINOR <<  8U) |
            ((uint32_t)SDK_VERSION_PATCH);

        TEST_ASSERT("sdk_version_get_code() == 宏计算值", code == expected_code);
        printf("  版本码: 0x%06X  (期望: 0x%06X)\n",
               (unsigned int)code, (unsigned int)expected_code);

        TEST_ASSERT("版本码高 8 位为 0（防溢出）",
                    (code & 0xFF000000U) == 0U);

        /* SDK_VERSION_AT_LEAST 宏运行期验证 */
        TEST_ASSERT("VERSION_AT_LEAST(0,0,0) 恒为真",
                    SDK_VERSION_AT_LEAST(0, 0, 0));
        TEST_ASSERT("VERSION_AT_LEAST(255,255,255) 恒为假",
                    !SDK_VERSION_AT_LEAST(255, 255, 255));
        TEST_ASSERT("VERSION_AT_LEAST(MAJOR,MINOR,PATCH) 为真（等于自身）",
                    SDK_VERSION_AT_LEAST(
                        SDK_VERSION_MAJOR,
                        SDK_VERSION_MINOR,
                        SDK_VERSION_PATCH));
    }

    /* ── T05 发布通道合法枚举 ────────────────────────────────────────────── */
    TEST_SECTION("T05  发布通道合法枚举值");
    TEST_ASSERT("channel 为 dev | rc | mp",
                is_valid_channel(sdk_version_get_channel()));
    printf("  当前通道: %s\n", sdk_version_get_channel());

    /* ── T06 sdk_version_is_clean() 返回值 ──────────────────────────────── */
    TEST_SECTION("T06  sdk_version_is_clean() 返回值");
    {
        int clean = sdk_version_is_clean();
        TEST_ASSERT("is_clean() 返回 0 或 1",  clean == 0 || clean == 1);
        if (ver != NULL) {
            printf("  GIT_DIRTY_FLAG: %s  is_clean=%d\n",
                   ver->git_dirty, clean);
            /* mp 通道由 CMake 保障必为 CLEAN，此处做二次确认 */
            if (strcmp(ver->channel, "mp") == 0) {
                TEST_ASSERT("mp 通道 is_clean() 必须为 1", clean == 1);
            }
        }
    }

    /* ── T07 Git Hash 格式 ───────────────────────────────────────────────── */
    TEST_SECTION("T07  Git Hash 格式与长度");
    {
        const char *sh = sdk_version_get_git_hash();
        TEST_ASSERT("短 Hash 长度恰好 8 位",
                    sh != NULL && strlen(sh) == 8U);
        TEST_ASSERT("短 Hash 全为十六进制字符",
                    is_hex_string(sh, 8U));

        if (ver != NULL) {
            TEST_ASSERT("完整 Hash 长度为 40 位",
                        strlen(ver->git_hash_full) == 40U);
            TEST_ASSERT("完整 Hash 全为十六进制字符",
                        is_hex_string(ver->git_hash_full, 40U));
            /* 短 Hash 须是完整 Hash 的前缀 */
            TEST_ASSERT("短 Hash 是完整 Hash 的前8字符",
                        strncmp(ver->git_hash_short,
                                ver->git_hash_full, 8U) == 0);
        }
    }

    /* ── T08 构建时间戳与日期格式 ────────────────────────────────────────── */
    TEST_SECTION("T08  构建时间戳与日期格式");
    if (ver != NULL) {
        /* build_date 须为 8 位纯数字 YYYYMMDD */
        TEST_ASSERT("build_date 长度为 8 位",
                    strlen(ver->build_date) == 8U);
        TEST_ASSERT("build_date 全为数字字符",
                    is_digit_string(ver->build_date, 8U));

        /* build_timestamp 须含 'T'（ISO 8601 日期时间分隔符） */
        TEST_ASSERT("build_timestamp 含 'T' 分隔符",
                    strchr(ver->build_timestamp, 'T') != NULL);

        /* build_timestamp 须含 'Z'（UTC 标记） */
        TEST_ASSERT("build_timestamp 含 'Z' UTC 标记",
                    strchr(ver->build_timestamp, 'Z') != NULL);

        /* 长度合理性：YYYYMMDDTHHMMSSz 共 16 字符 */
        TEST_ASSERT("build_timestamp 长度 >= 16",
                    strlen(ver->build_timestamp) >= 16U);
    }

    /* ── T09 HW_COMPAT 字段 ──────────────────────────────────────────────── */
    TEST_SECTION("T09  HW_COMPAT 字段");
    if (ver != NULL) {
        has_hw_compat = (ver->hw_compat_min != NULL &&
                         ver->hw_compat_min[0] != '\0');
        if (has_hw_compat) {
            TEST_ASSERT("hw_compat_min 长度 >= 2",
                        strlen(ver->hw_compat_min) >= 2U);
            TEST_ASSERT("hw_compat_max 长度 >= 2",
                        strlen(ver->hw_compat_max) >= 2U);
            /* Banner 须含 " HW:" 字段 */
            TEST_ASSERT("banner 含 ' HW:' 字段（已配置 HW_COMPAT）",
                        strstr(ver->version_banner, " HW:") != NULL);
            printf("  HW_COMPAT: %s ~ %s\n",
                   ver->hw_compat_min, ver->hw_compat_max);
        } else {
            TEST_SKIP("hw_compat_min / max 长度校验",
                      "SDK_HW_COMPAT_MIN 未配置（桌面 SDK 场景）");
        }
    }

    /* ── T10 API 与结构体字段交叉一致性 ─────────────────────────────────── */
    TEST_SECTION("T10  API 与结构体字段交叉一致性");
    if (ver != NULL) {
        TEST_ASSERT("get_code()     == ver->version_code",
                    sdk_version_get_code() == ver->version_code);
        TEST_ASSERT("get_str()      == ver->version_str",
                    strcmp(sdk_version_get_str(), ver->version_str) == 0);
        TEST_ASSERT("get_banner()   == ver->version_banner",
                    strcmp(sdk_version_get_banner(), ver->version_banner) == 0);
        TEST_ASSERT("get_git_hash() == ver->git_hash_short",
                    strcmp(sdk_version_get_git_hash(), ver->git_hash_short) == 0);
        TEST_ASSERT("get_channel()  == ver->channel",
                    strcmp(sdk_version_get_channel(), ver->channel) == 0);
        TEST_ASSERT("is_clean()     与 git_dirty 首字符一致",
                    sdk_version_is_clean() == (ver->git_dirty[0] == 'C' ? 1 : 0));
    }

    /* ── 汇总 ────────────────────────────────────────────────────────────── */
    printf("\n╔══════════════════════════════════════════════╗\n");
    printf("║  PASS %-4d   FAIL %-4d   SKIP %-4d   TOTAL %-4d║\n",
           s_pass, s_fail, s_skip, s_pass + s_fail + s_skip);
    printf("╚══════════════════════════════════════════════╝\n");

    if (s_fail > 0) {
        printf("结论: *** 存在失败用例，请检查上方 [FAIL] 条目 ***\n\n");
        return 1;
    }
    printf("结论: 全部断言通过 ✓\n\n");
    return 0;
}
