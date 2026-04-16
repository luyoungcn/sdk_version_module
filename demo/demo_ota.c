/**
 * @file  demo_ota.c
 * @brief 场景2：OTA 升级 —— 版本兼容性校验与降级保护
 *
 * 演示在 OTA 升级流程中如何使用版本模块：
 *   - 拒绝版本降级（防止回滚到有已知漏洞的旧版本）
 *   - 校验目标版本与当前硬件平台的兼容性
 *   - 升级完成后验证实际运行版本与预期一致
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "sdk_version.h"

/* ── OTA 包描述符（实际场景中来自 OTA 服务器下发的 JSON / 二进制头） ──────── */
typedef struct {
    const char *target_version;     /* 目标版本号字符串，如 "1.2.0"      */
    uint32_t    target_version_code; /* 目标版本数字码，如 0x010200       */
    const char *target_git_hash;    /* 目标版本 Git Hash，用于完整性核对 */
    const char *hw_compat_min;      /* 目标版本要求的最低 HW 版本        */
    const char *hw_compat_max;      /* 目标版本要求的最高 HW 版本        */
    uint32_t    package_crc32;      /* OTA 包 CRC32 完整性校验值         */
} OtaPackageHeader_t;

/* ── 模拟当前硬件平台标识（实际从 Flash / OTP 读取） ─────────────────────── */
#define CURRENT_HW_VERSION  "R3B"

/* ── 最低允许版本（低于此版本存在已知安全漏洞，禁止降级至此版本以下） ─────── */
#define MIN_ALLOWED_VERSION_CODE   SDK_VERSION_AT_LEAST(1, 0, 0)

/* ─────────────────────────────────────────────────────────────────────────── */

/**
 * @brief 检查硬件版本是否在 OTA 包兼容范围内
 *
 * 简化实现：比较字符串字典序。实际工程中根据平台规则实现。
 */
static int hw_version_in_range(const char *hw_ver,
                                const char *compat_min,
                                const char *compat_max)
{
    if (hw_ver == NULL || compat_min == NULL || compat_max == NULL) {
        return 0;
    }
    /* hw_ver >= compat_min  且  hw_ver <= compat_max */
    return (strcmp(hw_ver, compat_min) >= 0 &&
            strcmp(hw_ver, compat_max) <= 0);
}

/**
 * @brief OTA 升级前置校验
 * @return 0 = 校验通过，可继续升级；非0 = 校验失败，拒绝升级
 */
static int ota_pre_check(const OtaPackageHeader_t *pkg)
{
    uint32_t current_code = sdk_version_get_code();

    printf("\n[OTA] ── 升级前置校验 ──────────────────────────\n");
    printf("[OTA] 当前版本 : %s (0x%06X)\n",
           sdk_version_get_str(), (unsigned int)current_code);
    printf("[OTA] 目标版本 : %s (0x%06X)\n",
           pkg->target_version, (unsigned int)pkg->target_version_code);
    printf("[OTA] 当前 HW  : %s\n", CURRENT_HW_VERSION);
    printf("[OTA] 兼容范围 : %s ~ %s\n", pkg->hw_compat_min, pkg->hw_compat_max);

    /* 校验1：目标版本不得低于安全下限 */
    if (pkg->target_version_code < 0x010000U) {   /* 示例：不允许降级到 1.0.0 以下 */
        printf("[OTA] ✗ 拒绝：目标版本低于最低安全版本 (0x010000)\n");
        return -1;
    }

    /* 校验2：禁止降级（目标版本必须 >= 当前版本） */
    if (pkg->target_version_code < current_code) {
        printf("[OTA] ✗ 拒绝：目标版本 (0x%06X) 低于当前版本 (0x%06X)，禁止降级\n",
               (unsigned int)pkg->target_version_code,
               (unsigned int)current_code);
        return -2;
    }

    /* 校验3：硬件兼容性 */
    if (!hw_version_in_range(CURRENT_HW_VERSION,
                              pkg->hw_compat_min,
                              pkg->hw_compat_max)) {
        printf("[OTA] ✗ 拒绝：当前硬件 %s 不在目标版本兼容范围 [%s, %s] 内\n",
               CURRENT_HW_VERSION, pkg->hw_compat_min, pkg->hw_compat_max);
        return -3;
    }

    /* 校验4：仅允许 MP 通道产物（非量产版本禁止推送） */
    /* 实际场景中检查 pkg->channel 字段，此处用 Git Hash 长度做简化演示 */
    if (strlen(pkg->target_git_hash) != 8U) {
        printf("[OTA] ✗ 拒绝：OTA 包版本信息异常（Git Hash 格式错误）\n");
        return -4;
    }

    printf("[OTA] ✓ 全部校验通过，允许执行升级\n");
    return 0;
}

/**
 * @brief OTA 升级完成后的版本验证（模拟重启后调用）
 */
static void ota_post_verify(const char *expected_version,
                             const char *expected_git_hash)
{
    printf("\n[OTA] ── 升级后版本验证 ─────────────────────────\n");

    /* 对比版本号 */
    if (strcmp(sdk_version_get_str(), expected_version) == 0) {
        printf("[OTA] ✓ 版本号匹配：%s\n", sdk_version_get_str());
    } else {
        printf("[OTA] ✗ 版本号不符！期望 %s，实际 %s\n",
               expected_version, sdk_version_get_str());
    }

    /* 对比 Git Hash（精确溯源） */
    if (strcmp(sdk_version_get_git_hash(), expected_git_hash) == 0) {
        printf("[OTA] ✓ Git Hash 匹配：%s\n", sdk_version_get_git_hash());
    } else {
        printf("[OTA] ✗ Git Hash 不符！期望 %s，实际 %s\n",
               expected_git_hash, sdk_version_get_git_hash());
    }

    /* 上报完整版本信息到 OTA 平台（实际场景通过网络上报） */
    printf("[OTA] 上报版本信息：%s\n", sdk_version_get_banner());
}

/* ─────────────────────────────────────────────────────────────────────────── */

int main(void)
{
    printf("========================================\n");
    printf("  Demo: OTA 升级版本兼容性校验\n");
    printf("========================================\n");

    /* 上电打印当前版本 */
    sdk_version_print();

    /* ── 场景A：正常升级（版本递增，硬件兼容） ─────────────────────────── */
    {
        OtaPackageHeader_t pkg_ok = {
            .target_version      = "1.1.0",
            .target_version_code = 0x010100U,
            .target_git_hash     = "deadbeef",
            .hw_compat_min       = "R3A",
            .hw_compat_max       = "R3C",
            .package_crc32       = 0xABCD1234U,
        };
        printf("\n[场景A] 正常升级包\n");
        ota_pre_check(&pkg_ok);
    }

    /* ── 场景B：降级请求（应被拒绝） ───────────────────────────────────── */
    {
        OtaPackageHeader_t pkg_downgrade = {
            .target_version      = "0.9.0",
            .target_version_code = 0x000900U,
            .target_git_hash     = "cafebabe",
            .hw_compat_min       = "R3A",
            .hw_compat_max       = "R3C",
            .package_crc32       = 0x12345678U,
        };
        printf("\n[场景B] 降级包（应被拒绝）\n");
        ota_pre_check(&pkg_downgrade);
    }

    /* ── 场景C：硬件不兼容（应被拒绝） ────────────────────────────────── */
    {
        OtaPackageHeader_t pkg_hw_mismatch = {
            .target_version      = "2.0.0",
            .target_version_code = 0x020000U,
            .target_git_hash     = "f00dcafe",
            .hw_compat_min       = "R4A",   /* 新硬件平台，当前 R3B 不兼容 */
            .hw_compat_max       = "R4C",
            .package_crc32       = 0x87654321U,
        };
        printf("\n[场景C] 硬件不兼容包（应被拒绝）\n");
        ota_pre_check(&pkg_hw_mismatch);
    }

    /* ── 场景D：升级成功后的版本验证（模拟重启后）───────────────────────── */
    printf("\n[场景D] 模拟升级完成后的版本验证\n");
    /* 注意：此处 expected_git_hash 与当前实际 Hash 相同是因为 Demo 中
     * "升级"前后用的是同一个二进制。实际场景中期望值来自 OTA 服务器。 */
    ota_post_verify(sdk_version_get_str(), sdk_version_get_git_hash());

    printf("\n========================================\n");
    return 0;
}
