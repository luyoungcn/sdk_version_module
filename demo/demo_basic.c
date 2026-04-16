/**
 * @file  demo_basic.c
 * @brief 场景1：最简集成 —— 上电打印版本信息
 *
 * 这是将 sdk_version 模块集成进自己 SDK 的最小用法。
 * 只需两步：
 *   1. 在初始化函数最开头调用 sdk_version_print()
 *   2. 其余业务逻辑照常
 */

#include <stdio.h>
#include "sdk_version.h"

/* ── 模拟你的 SDK 初始化函数 ─────────────────────────────────────────────── */
void my_sdk_init(void)
{
    /* 第一行永远是版本打印，不受日志级别控制 */
    sdk_version_print();

    /* 后续正常业务初始化 */
    printf("[my_sdk] 硬件外设初始化...\n");
    printf("[my_sdk] 通信协议栈启动...\n");
    printf("[my_sdk] 初始化完成\n");
}

int main(void)
{
    my_sdk_init();
    return 0;
}
