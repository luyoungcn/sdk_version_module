#include <stdio.h>
#include "sdk_version.h"

int main()
{
    printf("===== 独立版本模块测试开始 =====\n");

    // 打印全量版本信息
    sdk_version_print_all();

    // 测试API
    printf("\n[API测试]\n");
    printf("版本字符串: %s\n", sdk_version_get_str());
    printf("Git短Hash : %s\n", sdk_version_get_git_short());
    printf("数字版本码: 0x%06X\n", sdk_version_get_code());

    printf("\n===== 版本模块测试通过 =====\n");
    return 0;
}
