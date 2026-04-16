/**
 * @file  demo_fault.c
 * @brief 场景4：现场故障快速回溯 —— 从崩溃日志精确定位代码基线
 *
 * 演示当 ECU 发生故障时，如何将版本信息写入故障记录，
 * 以及售后工程师如何通过故障记录快速还原问题现场。
 *
 * 故障回溯完整链路：
 *   现场故障日志  →  提取版本信息  →  git checkout <hash>
 *   →  复现构建  →  根因分析  →  修复发版
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include "sdk_version.h"

/* ── 故障记录结构（实际存储在 ECU 的非易失性存储区，如 EEPROM / Flash） ── */
typedef struct {
    /* 故障基本信息 */
    uint32_t    fault_code;         /* DTC 故障码                    */
    uint32_t    fault_timestamp;    /* 故障发生时间（Unix timestamp） */
    uint32_t    occurrence_count;   /* 累计发生次数                  */

    /* 版本溯源信息（从 sdk_version 模块自动填充） */
    char        sdk_version[32];    /* 如 "1.4.2" 或 "1.4.2-rc.1"   */
    char        git_hash[9];        /* 8 位缩写 Hash + '\0'          */
    char        build_date[9];      /* YYYYMMDD + '\0'               */
    char        channel[8];         /* "dev" | "rc" | "mp"           */

    /* 故障上下文快照 */
    uint32_t    register_snapshot[4]; /* 故障时的关键寄存器值        */
    uint8_t     stack_trace[64];    /* 简化的调用栈摘要              */
} FaultRecord_t;

/* ── 模拟 DTC 故障码定义 ─────────────────────────────────────────────────── */
#define DTC_SENSOR_TIMEOUT      0xC0A001U
#define DTC_COMM_CHECKSUM_ERR   0xC0B002U
#define DTC_ALGO_OVERFLOW       0xC0C003U

/* ─────────────────────────────────────────────────────────────────────────── */

/**
 * @brief 创建故障记录（在故障发生时调用）
 *
 * 版本信息字段由本函数自动从 sdk_version 模块填充，
 * 调用方无需手动填写，确保版本信息与故障一一对应。
 */
static void fault_record_create(FaultRecord_t    *rec,
                                 uint32_t          fault_code,
                                 const uint32_t   *regs,
                                 uint32_t          reg_count)
{
    const SdkVersionInfo_t *ver = sdk_version_get_info();
    uint32_t i;

    if (rec == NULL || ver == NULL) { return; }

    /* 填充故障基本信息 */
    rec->fault_code        = fault_code;
    rec->fault_timestamp   = (uint32_t)time(NULL);
    rec->occurrence_count  = 1U;

    /* ── 自动填充版本溯源信息 ─────────────────────────────────────────────
     * 这是版本模块最核心的价值：故障发生的瞬间，版本信息已经被
     * 锁定在故障记录里，事后无论经历多少次版本迭代，都能精确还原
     * 故障时的代码基线。
     */
    strncpy(rec->sdk_version, ver->version_str,  sizeof(rec->sdk_version)  - 1U);
    strncpy(rec->git_hash,    ver->git_hash_short, sizeof(rec->git_hash)   - 1U);
    strncpy(rec->build_date,  ver->build_date,    sizeof(rec->build_date)  - 1U);
    strncpy(rec->channel,     ver->channel,       sizeof(rec->channel)     - 1U);

    /* 确保字符串终止 */
    rec->sdk_version[sizeof(rec->sdk_version) - 1U] = '\0';
    rec->git_hash[sizeof(rec->git_hash)       - 1U] = '\0';
    rec->build_date[sizeof(rec->build_date)   - 1U] = '\0';
    rec->channel[sizeof(rec->channel)         - 1U] = '\0';

    /* 填充寄存器快照 */
    for (i = 0U; i < 4U && i < reg_count; i++) {
        rec->register_snapshot[i] = regs[i];
    }
}

/**
 * @brief 打印故障记录（模拟从非易失性存储读取后的输出）
 */
static void fault_record_print(const FaultRecord_t *rec)
{
    if (rec == NULL) { return; }

    printf("┌─────────────────────────────────────────────\n");
    printf("│  故障记录\n");
    printf("├─────────────────────────────────────────────\n");
    printf("│  DTC 故障码    : 0x%06X\n",  (unsigned int)rec->fault_code);
    printf("│  发生时间      : %u\n",       (unsigned int)rec->fault_timestamp);
    printf("│  累计次数      : %u\n",       (unsigned int)rec->occurrence_count);
    printf("├─────────────────────────────────────────────\n");
    printf("│  [版本溯源信息]\n");
    printf("│  SDK 版本      : %s\n",  rec->sdk_version);
    printf("│  Git Hash      : %s\n",  rec->git_hash);
    printf("│  构建日期      : %s\n",  rec->build_date);
    printf("│  发布通道      : %s\n",  rec->channel);
    printf("├─────────────────────────────────────────────\n");
    printf("│  寄存器快照    : PC=0x%08X LR=0x%08X\n",
           (unsigned int)rec->register_snapshot[0],
           (unsigned int)rec->register_snapshot[1]);
    printf("│                  SP=0x%08X xPSR=0x%08X\n",
           (unsigned int)rec->register_snapshot[2],
           (unsigned int)rec->register_snapshot[3]);
    printf("└─────────────────────────────────────────────\n");
}

/**
 * @brief 生成售后工程师回溯操作指引
 *
 * 根据故障记录中的版本信息，自动生成可直接执行的 Git 命令，
 * 售后工程师无需手动查找版本对应关系。
 */
static void fault_record_gen_trace_guide(const FaultRecord_t *rec)
{
    if (rec == NULL) { return; }

    printf("\n── 自动生成的版本回溯操作指引 ──────────────────\n");
    printf("\n# 步骤1：切换到故障时的代码基线\n");
    printf("git checkout %s\n", rec->git_hash);

    printf("\n# 步骤2：验证切换成功\n");
    printf("git log --oneline -1   # 应包含 hash: %s\n", rec->git_hash);

    printf("\n# 步骤3：用相同构建参数复现故障版本\n");
    printf("cmake -B build -DSDK_CHANNEL=%s\n", rec->channel);
    printf("cmake --build build\n");

    printf("\n# 步骤4：校验复现产物与故障版本一致\n");
    printf("# 在复现的二进制上电日志中确认：\n");
    printf("#   VERSION  = %s\n", rec->sdk_version);
    printf("#   GIT_HASH = %s\n", rec->git_hash);
    printf("\n# 步骤5：在复现环境中重现故障\n");
    printf("# DTC: 0x%06X  寄存器 PC: 0x%08X\n",
           (unsigned int)rec->fault_code,
           (unsigned int)rec->register_snapshot[0]);
}

/* ─────────────────────────────────────────────────────────────────────────── */

int main(void)
{
    /* 模拟故障时的寄存器快照 */
    static const uint32_t regs[4] = {
        0x080123A4U,   /* PC：故障时程序计数器，指向出错指令地址 */
        0x08011F80U,   /* LR：链接寄存器，调用者返回地址         */
        0x20007FF0U,   /* SP：栈指针                             */
        0x21000000U,   /* xPSR：程序状态寄存器                   */
    };

    FaultRecord_t fault_rec;

    printf("========================================\n");
    printf("  Demo: 现场故障快速回溯\n");
    printf("========================================\n");

    /* 上电打印版本信息 */
    sdk_version_print();

    /* ── 模拟故障发生 ─────────────────────────────────────────────────── */
    printf("\n[系统] 运行中...\n");
    printf("[系统] 检测到算法溢出故障，记录 DTC\n\n");

    /* 故障发生时，自动将当前版本信息写入故障记录 */
    fault_record_create(&fault_rec, DTC_ALGO_OVERFLOW, regs, 4U);

    /* 打印故障记录（模拟从 EEPROM 读出） */
    fault_record_print(&fault_rec);

    /* 生成回溯操作指引 */
    fault_record_gen_trace_guide(&fault_rec);

    printf("\n========================================\n");
    printf("  说明：\n");
    printf("  将上述 git hash 提供给研发工程师，\n");
    printf("  即可精确还原故障时的源代码，无需猜测版本。\n");
    printf("========================================\n");

    return 0;
}
