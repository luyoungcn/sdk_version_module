#ifndef __SDK_VERSION_H__
#define __SDK_VERSION_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *version_str;
    const char *git_hash;
    const char *git_short;
    const char *git_branch;
    const char *build_date;
    const char *build_time;
    const char *compiler;
    const char *platform;
    const char *build_type;
    uint32_t version_code;
} sdk_version_info_t;

const char *sdk_version_get_str(void);
const char *sdk_version_get_git_short(void);
uint32_t sdk_version_get_code(void);
void sdk_version_print_all(void);
const sdk_version_info_t *sdk_version_get_info(void);

#ifdef __cplusplus
}
#endif

#endif
