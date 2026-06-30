# Yorukv

Yorukv is a lightweight single-header KV helper for STM32-style embedded projects.

Repository:

---

> **Yoru Series**
>
> A family of lightweight utility libraries for STM32 HAL. Each library can be used independently or combined as needed.
>
> | Library | Role |
> | --- | --- |
> | [Yorulog](https://github.com/ExMikuPro/Yorulog) | Lightweight UART logger |
> | [Yorush](https://github.com/ExMikuPro/Yorush) | Lightweight UART shell / command parser |
> | [Yorunvm](https://github.com/ExMikuPro/Yorunvm) | STM32 on-chip NVM / Flash / EEPROM access helper |
> | [Yorukv](https://github.com/ExMikuPro/Yorukv) | Lightweight KV configuration library |
> | [Yorubench](https://github.com/ExMikuPro/Yorubench) | Lightweight performance measurement library |
> | [Yoruassert](https://github.com/ExMikuPro/Yoruassert.git) | Lightweight assertion helper |

---

## Features

* single-header
* no dynamic memory
* fixed key table
* `bool` / `i32` / `u32` / `string`
* optional persistence backend
* append-only log + reload + delete + GC

## Quick Start

In one `.c` file:

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

Use it:

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

## Optional Persistence

Attach your own backend:

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
YORUKV_Format(&hYorukv); /* first time */
YORUKV_Load(&hYorukv);   /* next boot */
```

Yorukv does not bind itself to one storage driver.
You can connect it to STM32 internal Flash, EEPROM, RAM simulation, or your own backend callbacks.

## Notes

* `ResetKey` restores the default value
* `DeleteKey` currently also restores the default value
* `FULL` API is a thin helper layer for easier usage
* core API is still available for smaller or stricter builds
