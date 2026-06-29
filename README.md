# Yorukv

Yorukv is a lightweight single-header KV helper for STM32-style embedded projects.

The current stage focuses on a small and predictable key-value layer with optional single-region log persistence. It is designed to stay simple:

* no dynamic memory
* no `printf`
* no stdlib-heavy runtime dependency
* user-controlled feature growth through macros

Repository:

* [ExMikuPro/Yorukv](https://github.com/ExMikuPro/Yorukv.git)

---

## Current Scope

Current stage includes:

* single-header KV core
* fixed table registration
* `bool` / `i32` / `u32` / `string`
* `Init`
* `Get`
* `Set`
* `Reset`
* `Delete`
* `List`
* optional single-region log persistence backend
* startup reload from persisted log
* automatic compact GC when log space is exhausted
* `NO_SPACE` return when data still cannot fit after GC

Not included in the current stage:

* wear leveling policy
* multi-region / dual-bank migration
* page-level GC
* external Flash adaptation
* file system style abstraction
* dynamic schema / runtime key registration

---

## Design Goals

* small
* predictable
* low RAM cost
* no hidden allocator
* clear separation between KV policy and storage backend

Yorukv is intended to be a KV layer, not a large storage framework.

---

## Quick Start

Place `yorukv.h` into your project, for example:

```text
/Core/Yorukv/yorukv.h
```

In exactly one `.c` file:

```c
#define YORUKV_DEFINE_GLOBALS
#include "yorukv.h"
```

In other `.c` files:

```c
#include "yorukv.h"
```

---

## Basic KV Usage

```c
#define YORUKV_DEFINE_GLOBALS
#include "yorukv.h"

static yorukv_bool_t led_enable;
static yorukv_i32_t threshold;
static yorukv_u32_t baud;
static char name[16];

static const yorukv_bool_t led_default = 0u;
static const yorukv_i32_t threshold_default = -10;
static const yorukv_u32_t baud_default = 115200u;
static const char name_default[] = "yoru";

static const YORUKV_ItemTypeDef app_kv[] = {
    YORUKV_ITEM_BOOL_RW("led", &led_enable, &led_default),
    YORUKV_ITEM_I32_RW("threshold", &threshold, &threshold_default),
    YORUKV_ITEM_U32_RW("baud", &baud, &baud_default),
    YORUKV_ITEM_STR_RW("name", name, sizeof(name), name_default),
};

void app_kv_init(void)
{
    YORUKV_Init(&hYorukv, app_kv, YORUKV_ARRAY_SIZE(app_kv));
}
```

---

## Core API

```c
YORUKV_StatusTypeDef YORUKV_Init(YORUKV_HandleTypeDef *hkv,
                                 const YORUKV_ItemTypeDef *table,
                                 yorukv_u32_t count);

YORUKV_StatusTypeDef YORUKV_Get(YORUKV_HandleTypeDef *hkv,
                                const char *key,
                                YORUKV_ValueTypeDef *out);

YORUKV_StatusTypeDef YORUKV_Set(YORUKV_HandleTypeDef *hkv,
                                const char *key,
                                const YORUKV_ValueTypeDef *value);

YORUKV_StatusTypeDef YORUKV_Reset(YORUKV_HandleTypeDef *hkv,
                                  const char *key);

YORUKV_StatusTypeDef YORUKV_Delete(YORUKV_HandleTypeDef *hkv,
                                   const char *key);

YORUKV_StatusTypeDef YORUKV_List(YORUKV_HandleTypeDef *hkv,
                                 YORUKV_ListCallbackTypeDef cb,
                                 void *user);
```

Meaning:

* `Set`: updates the current value
* `Reset`: restores the default value
* `Delete`: for the current stage, restores the item to its default value
* `List`: iterates over registered items

---

## Persistence Model

Persistence is optional and backend-driven.

Yorukv does not directly bind itself to one fixed storage medium. Instead, the current stage uses a small callback-based backend:

```c
typedef struct
{
    YORUKV_PersistReadFunc Read;
    YORUKV_PersistWriteFunc Write;
    YORUKV_PersistEraseFunc Erase;
    void *User;
    yorukv_u32_t Size;
    yorukv_u32_t WriteBlockSize;
    yorukv_u8_t ErasedValue;
} YORUKV_PersistConfigTypeDef;
```

Attach and use:

```c
YORUKV_AttachPersist(&hYorukv, &persist_cfg);
YORUKV_FormatPersist(&hYorukv);
YORUKV_LoadPersist(&hYorukv);
YORUKV_RunGC(&hYorukv);
```

Current persistence behavior:

* single-region append-only log
* reload by scanning the log from the beginning
* latest valid record wins
* delete is stored as a delete record
* automatic compact GC is attempted when append space is exhausted

---

## Backend Notes

The current persistence design works best when:

* `WriteBlockSize` matches the real write granularity of the storage medium
* `Erase` clears the whole persistence region or the requested range to one erased byte pattern
* `ErasedValue` matches the backend's erased byte value, typically `0xFF`

For STM32 internal Flash, this can be adapted through a lower NVM layer such as Yorunvm.

---

## Status Codes

Main status codes:

* `YORUKV_OK`
* `YORUKV_ERROR`
* `YORUKV_INVALID_PARAM`
* `YORUKV_NOT_INITIALIZED`
* `YORUKV_NOT_FOUND`
* `YORUKV_TYPE_MISMATCH`
* `YORUKV_READ_ONLY`
* `YORUKV_NO_SPACE`

---

## Verified Current Stage

The current stage has been verified in two ways:

* host-side callback backend tests
* STM32H743 real Flash test with:
  * append log
  * reload
  * delete record
  * automatic recovery to default after delete

This means the current single-region log path is not only a host simulation path. It has already been exercised on real STM32 internal Flash.

