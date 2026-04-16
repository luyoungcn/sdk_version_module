# sdk_version_module

通用 SDK 版本管理组件，可集成进任意嵌入式 / 桌面 C 工程。

## 特性

- **零依赖**：仅需 C99 标准库，无任何第三方依赖
- **全自动**：Git Hash、分支名、时间戳、工具链版本全部由 CMake 在构建时自动注入
- **三通道**：`dev` / `rc` / `mp` 独立发布通道，MP 通道强制洁净工作区校验
- **双格式输出**：上电打印同时输出人类可读表格 + 机器可读 JSON 单行摘要
- **版本守卫宏**：`SDK_VERSION_AT_LEAST(maj, min, pat)` 供集成方做编译期 API 兼容性守卫
- **嵌入式友好**：版本信息固定写入 `.rodata.version` 段，JTAG / OTA 工具可直接读取
- **跨平台**：GCC / Clang / MSVC 兼容，PC 和 MCU 均可使用

## 目录结构

```
sdk_version_module/
├── CMakeLists.txt          # 顶层构建（示例工程）
├── README.md
├── version/                # ← 集成时只需复制此目录
│   ├── CMakeLists.txt      #   版本注入构建逻辑
│   ├── version_auto.h.in   #   自动生成头文件模板（禁止手动编辑生成产物）
│   ├── sdk_version.h       #   公共 API 头文件
│   └── sdk_version.c       #   API 实现
└── test/
    └── test_main.c         # 独立测试程序（10组，38个断言）
```

## 集成步骤（3步）

**步骤1**：将 `version/` 目录复制到你的工程

```
your_project/
├── CMakeLists.txt
├── version/          ← 复制到这里
│   ├── CMakeLists.txt
│   ├── version_auto.h.in
│   ├── sdk_version.h
│   └── sdk_version.c
└── src/
    └── main.c
```

**步骤2**：在你的顶层 `CMakeLists.txt` 中添加配置和子目录

```cmake
# 按需修改这些参数
set(SDK_VERSION_MAJOR "2")
set(SDK_VERSION_MINOR "0")
set(SDK_VERSION_PATCH "1")
set(SDK_PRODUCT_NAME  "YOUR-SDK-NAME")   # 替换为你的产品名
set(SDK_CHANNEL       "dev")             # 开发时用 dev，发布时用 mp

# 嵌入式工程可选配置硬件兼容范围，桌面工程留空即可
# set(SDK_HW_COMPAT_MIN "R3A")
# set(SDK_HW_COMPAT_MAX "R3C")

add_subdirectory(version)
```

**步骤3**：链接到你的目标并调用初始化函数

```cmake
target_link_libraries(your_target PRIVATE sdk_version)
```

```c
#include "sdk_version.h"

void your_sdk_init(void) {
    sdk_version_print();   // 上电打印版本信息，始终第一行调用
    // ... 其余初始化
}
```

## 构建与运行（示例工程）

```bash
# 日常开发（允许脏工作区，默认 dev 通道）
cmake -B build
cmake --build build
./build/version_test

# 指定产品名和版本
cmake -B build \
    -DSDK_PRODUCT_NAME="FOO-SDK" \
    -DSDK_VERSION_MAJOR=2 \
    -DSDK_VERSION_MINOR=1 \
    -DSDK_VERSION_PATCH=0

# 量产构建（脏工作区将 FATAL_ERROR 阻断）
cmake -B build -DSDK_CHANNEL=mp
cmake --build build
```

## 通道行为说明

| 通道 | 脏工作区 | 版本字符串示例 |
|------|---------|---------------|
| `dev` | ✅ 允许，附加 `+dirty` | `1.0.0-dev.20260416+dirty` |
| `rc`  | ✅ 允许，附加 `+dirty` | `1.0.0-rc.3+dirty` |
| `mp`  | ❌ FATAL_ERROR 阻断   | `1.0.0` |

## 上电输出示例

```
================================================
           SDK Version Info
================================================
  PRODUCT          : MY-SDK
  VERSION          : 1.0.0-dev.20260416
  BANNER           : MY-SDK/1.0.0-dev.20260416 GIT:a1b2c3d4
  GIT_HASH         : a1b2c3d4
  GIT_BRANCH       : main
  DIRTY            : CLEAN
  BUILD_TS         : 20260416T083000Z
  TOOLCHAIN        : GNU 13.3.0
  CHANNEL          : dev
  VERSION_CODE     : 0x010000
================================================
{"event":"sdk_init","product":"MY-SDK","version":"1.0.0-dev.20260416",...}
```

## API 速查

```c
const SdkVersionInfo_t *sdk_version_get_info(void);   // 完整结构体
const char             *sdk_version_get_str(void);    // 版本号字符串
const char             *sdk_version_get_banner(void); // 完整横幅
const char             *sdk_version_get_git_hash(void); // 8位 Hash
uint32_t                sdk_version_get_code(void);   // 数字版本码 0xMMIIPP
int                     sdk_version_is_clean(void);   // 1=CLEAN, 0=DIRTY
const char             *sdk_version_get_channel(void);// "dev"|"rc"|"mp"
void                    sdk_version_print(void);      // 上电打印

// 编译期版本守卫宏
#if SDK_VERSION_AT_LEAST(1, 2, 0)
    new_api();
#endif

// 编译期洁净度断言（C11，可选用于敏感源文件）
SDK_ASSERT_CLEAN_BUILD;
```
