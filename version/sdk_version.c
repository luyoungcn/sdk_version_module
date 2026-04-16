#include "sdk_version.h"
#include "version_auto.h"
#include <stdio.h>

const sdk_version_info_t g_sdk_version = {
    .version_str = SDK_VERSION_STR,
    .git_hash = SDK_GIT_HASH,
    .git_short = SDK_GIT_SHORT_HASH,
    .git_branch = SDK_GIT_BRANCH,
    .build_date = SDK_BUILD_DATE,
    .build_time = SDK_BUILD_TIME,
    .compiler = SDK_COMPILER_VERSION,
    .platform = SDK_HW_PLATFORM,
    .build_type = SDK_BUILD_TYPE,
    .version_code = SDK_VERSION_CODE,
};

const char *sdk_version_get_str(void) {
    return g_sdk_version.version_str;
}

const char *sdk_version_get_git_short(void) {
    return g_sdk_version.git_short;
}

uint32_t sdk_version_get_code(void) {
    return g_sdk_version.version_code;
}

const sdk_version_info_t *sdk_version_get_info(void) {
    return &g_sdk_version;
}

void sdk_version_print_all(void)
{
    const sdk_version_info_t *ver = &g_sdk_version;
    printf("=============================================\n");
    printf("          Radar SDK Version Info\n");
    printf("Version:  %s\n", ver->version_str);
    printf("GitHash:  %s\n", ver->git_short);
    printf("Branch:   %s\n", ver->git_branch);
    printf("Build:    %s %s\n", ver->build_date, ver->build_time);
    printf("Platform: %s\n", ver->platform);
    printf("Compiler: %s\n", ver->compiler);
    printf("Type:     %s\n", ver->build_type);
    printf("=============================================\n");
}
