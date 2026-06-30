# Yorukv

Yorukv 是一个面向 STM32 风格嵌入式项目的轻量级单头文件 KV 库。

仓库地址：

* [ExMikuPro/Yorukv](https://github.com/ExMikuPro/Yorukv.git)

---

> **Yoru 系列**
>
> 一组面向 STM32 HAL 的轻量级工具库。各库可独立使用，也可以组合使用。
>
> | 库 | 定位 |
> | --- | --- |
> | [Yorulog](https://github.com/ExMikuPro/Yorulog) | 轻量级 UART 日志库 |
> | [Yorush](https://github.com/ExMikuPro/Yorush) | 轻量级 UART Shell / 命令解析器 |
> | [Yorunvm](https://github.com/ExMikuPro/Yorunvm) | STM32 片上 NVM / Flash 访问辅助库 |
> | [Yorukv](https://github.com/ExMikuPro/Yorukv) | 轻量级 KV 配置库 |
> | [Yorubench](https://github.com/ExMikuPro/Yorubench) | 轻量级性能测量库 |

---

## 特性

* 单头文件
* 不使用动态内存
* 固定 key 表
* 支持 `bool` / `i32` / `u32` / `string`
* 可选持久化后端
* 追加写日志、重载恢复、删除、GC

## 快速开始

在一个 `.c` 文件中：

```c
#define YORUKV_ENABLE_FULL_API 1
#define YORUKV_DEFINE_GLOBALS
#include "yorukv.h"

static yorukv_bool_t led_enable;
static yorukv_u32_t baud;
static char name[16];

static const yorukv_bool_t led_default = 0u;
static const yorukv_u32_t baud_default = 115200u;
static const char name_default[] = "yoru";

static const YORUKV_ItemTypeDef app_kv[] = {
    YORUKV_ITEM_BOOL_RW("led", &led_enable, &led_default),
    YORUKV_ITEM_U32_RW("baud", &baud, &baud_default),
    YORUKV_ITEM_STR_RW("name", name, sizeof(name), name_default),
};

void app_kv_init(void)
{
    YORUKV_Init(&hYorukv, app_kv, YORUKV_ARRAY_SIZE(app_kv));
}
```

使用方式：

```c
bool led = false;
uint32_t baud_now = 0;
char name_buf[16];

YORUKV_SetBool(&hYorukv, "led", true);
YORUKV_SetU32(&hYorukv, "baud", 230400u);
YORUKV_SetStr(&hYorukv, "name", "yorukv");

YORUKV_GetBool(&hYorukv, "led", &led);
YORUKV_GetU32(&hYorukv, "baud", &baud_now);
YORUKV_GetStr(&hYorukv, "name", name_buf, sizeof(name_buf));
```

## 可选持久化

挂接你自己的后端：

```c
static const YORUKV_PersistConfigTypeDef kv_persist = {
    kv_read,
    kv_write,
    kv_erase,
    0,
    0x00040000u,
    0x00020000u,
    2u,
    32u,
    0xFFu
};

YORUKV_UsePersist(&hYorukv, &kv_persist);
YORUKV_Format(&hYorukv); /* 首次格式化 */
YORUKV_Load(&hYorukv);   /* 下次上电恢复 */
```

Yorukv 不死绑某一种存储驱动。
你可以接 STM32 内部 Flash、EEPROM、RAM 模拟后端，或者自己的回调。

## 说明

* `ResetKey` 会恢复默认值
* `DeleteKey` 当前也会恢复默认值
* `FULL` API 是一层更易用的薄封装
* 核心 API 仍然保留，适合更小或更严格的构建方式
