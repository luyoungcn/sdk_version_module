/**
 * @file    sdk_version.h
 * @brief   SDK 版本管理公共接口
 *
 * @details
 *   通用版本管理模块，可集成进任意嵌入式 / 桌面 SDK 工程。
 *   集成步骤：
 *     1. 将 version/ 目录复制到目标工程
 *     2. 在顶层 CMakeLists.txt 中 add_subdirectory(version)
 *     3. target_link_libraries(your_target PRIVATE sdk_version)
 *     4. 在 SDK 初始化最早阶段调用 sdk_version_print()
 *
 *   所有接口满足以下约束（适用于车规 / 安全关键嵌入式场景）：
 *     - 无动态内存分配
 *     - 无递归调用
 *     - 无运行时字符串格式化
 *     - 所有返回指针指向静态常量，调用者不得释放或修改
 *     - 可通过 MISRA C:2012 合规检查
 */

#ifndef SDK_VERSION_H
#define SDK_VERSION_H

#include <stdint.h>

/* 引入自动生成的宏定义（版本字符串、Hash、实用宏等） */
#include "version_auto.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ═══════════════════════════════════════════════════════════════════════════
 *  数据结构
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * @brief SDK 完整版本信息结构体
 *
 * 所有 const char* 字段均指向静态只读字符串，生命周期与程序相同。
 */
typedef struct {
    const char *product_name;   /**< 产品名，如 "MY-SDK"                        */
    const char *version_str;    /**< semver 版本号，如 "1.2.3" / "1.2.3-rc.2"  */
    const char *version_banner; /**< 完整上电横幅，含产品名、HW范围、Git Hash    */
    const char *git_hash_full;  /**< 40 位完整 SHA-1 Hash                       */
    const char *git_hash_short; /**< 8 位缩写 Hash                              */
    const char *git_branch;     /**< 构建时所在 Git 分支名                      */
    const char *git_dirty;      /**< "CLEAN" | "DIRTY" | "UNKNOWN"              */
    const char *build_timestamp;/**< UTC 构建时间戳，格式 YYYYMMDDTHHMMSSz      */
    const char *build_date;     /**< 构建日期，格式 YYYYMMDD                    */
    const char *toolchain_ver;  /**< 编译器标识，如 "GNU 12.3.1"                */
    const char *channel;        /**< 发布通道："dev" | "rc" | "mp"              */
    const char *hw_compat_min;  /**< 兼容最低 HW/平台版本（嵌入式场景，可为空） */
    const char *hw_compat_max;  /**< 兼容最高 HW/平台版本（嵌入式场景，可为空） */
    uint32_t    version_code;   /**< 数字版本码 0xMMIIPP，用于版本大小比较      */
} SdkVersionInfo_t;

/* ═══════════════════════════════════════════════════════════════════════════
 *  API
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * @brief  获取完整版本信息结构体的只读指针
 * @return 指向全局静态版本信息的常量指针，永不为 NULL
 */
const SdkVersionInfo_t *sdk_version_get_info(void);

/**
 * @brief  获取 semver 版本号字符串
 * @return 如 "1.2.3" 或 "1.2.3-dev.20260416+dirty"
 */
const char *sdk_version_get_str(void);

/**
 * @brief  获取完整上电横幅字符串
 * @return 如 "MY-SDK/1.2.3 HW:R3A-R3C GIT:a1b2c3d4"
 */
const char *sdk_version_get_banner(void);

/**
 * @brief  获取 8 位缩写 Git Hash
 * @return 8 位十六进制字符串，如 "a1b2c3d4"
 */
const char *sdk_version_get_git_hash(void);

/**
 * @brief  获取数字版本码（0xMMIIPP）
 * @return 32 位无符号整数，用于 OTA 降级保护及版本大小比较
 *
 * 示例：
 * @code
 *   if (sdk_version_get_code() < REQUIRED_MIN_VERSION_CODE) {
 *       // 版本过低，拒绝继续
 *   }
 * @endcode
 */
uint32_t sdk_version_get_code(void);

/**
 * @brief  查询当前构建是否来自洁净工作区
 * @return 1 = CLEAN，0 = DIRTY 或 UNKNOWN
 */
int sdk_version_is_clean(void);

/**
 * @brief  获取发布通道字符串
 * @return "dev" | "rc" | "mp"
 */
const char *sdk_version_get_channel(void);

/**
 * @brief  上电版本信息规范打印
 *
 * @details
 *   输出两部分内容：
 *     1. 人类可读的对齐表格（供运维 / 调试人员目视核查）
 *     2. 紧随其后的单行 JSON 摘要（供日志采集 / 自动化解析）
 *
 *   须在 SDK 初始化最早阶段调用，不受日志级别控制，始终输出。
 *
 * 输出示例：
 * @code
 *   ================================================
 *             SDK Version Info
 *   ================================================
 *     PRODUCT        : MY-SDK
 *     VERSION        : 1.2.3
 *     BANNER         : MY-SDK/1.2.3 HW:R3A-R3C GIT:a1b2c3d4
 *     GIT_HASH       : a1b2c3d4
 *     GIT_BRANCH     : main
 *     DIRTY          : CLEAN
 *     BUILD_TS       : 20260416T083000Z
 *     TOOLCHAIN      : GNU 13.3.0
 *     CHANNEL        : mp
 *     HW_COMPAT      : R3A ~ R3C
 *     VERSION_CODE   : 0x010203
 *   ================================================
 *   {"event":"sdk_init","product":"MY-SDK","version":"1.2.3",...}
 * @endcode
 */
void sdk_version_print(void);

#ifdef __cplusplus
}
#endif

#endif /* SDK_VERSION_H */
