# Yorukv

Yorukv 是一个面向 STM32 风格嵌入式项目的轻量级单头文件 KV 辅助库。

当前阶段主要聚焦于一套小而可预测的键值层，并可选接入单区域日志式持久化。它的设计目标是尽量简单：

* 不使用动态内存
* 不依赖 `printf`
* 不引入标准库重运行时负担
* 功能增长由用户自己通过宏决定

仓库地址：

* [ExMikuPro/Yorukv](https://github.com/ExMikuPro/Yorukv.git)

---

## 当前范围

当前阶段已完成：

* 单头文件 KV 核心
* 静态表注册
* `bool` / `i32` / `u32` / `string`
* `Init`
* `Get`
* `Set`
* `Reset`
* `Delete`
* `List`
* 可选的单区域日志式持久化后端
* 上电后从持久化日志重新加载
* 日志空间耗尽时自动紧凑 GC
* 即使 GC 后仍放不下时返回 `NO_SPACE`

当前阶段不包含：

* 磨损均衡策略
* 多区域 / 双 Bank 迁移
* 分页级 GC
* 外部 Flash 适配
* 文件系统风格抽象
* 动态 schema / 运行时注册 key

---

## 设计目标

* 小
* 行为可预测
* RAM 占用低
* 不隐藏分配器
* KV 策略和底层存储后端边界清晰

Yorukv 的定位是 KV 层，而不是一个庞大的存储框架。

---

## 快速开始

把 `yorukv.h` 放到你的工程里，例如：

```text
/Core/Yorukv/yorukv.h
```

必须且只需要在一个 `.c` 文件中：

```c
#define YORUKV_DEFINE_GLOBALS
#include "yorukv.h"
```

其他 `.c` 文件正常包含：

```c
#include "yorukv.h"
```

---

## 基础 KV 用法

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

## 核心 API

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

语义如下：

* `Set`：更新当前值
* `Reset`：恢复默认值
* `Delete`：在当前阶段会把该项恢复到默认值
* `List`：遍历已注册项

---

## 持久化模型

持久化是可选的，并且由后端决定。

Yorukv 不直接死绑某一种固定存储介质。当前阶段使用的是一个很小的回调式后端：

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

挂接方式：

```c
YORUKV_AttachPersist(&hYorukv, &persist_cfg);
YORUKV_FormatPersist(&hYorukv);
YORUKV_LoadPersist(&hYorukv);
YORUKV_RunGC(&hYorukv);
```

当前阶段的持久化行为：

* 单区域追加写日志
* 上电通过顺序扫描日志恢复
* 同一个 key 以最后一条有效记录为准
* 删除会写入删除记录
* 追加空间不足时会先尝试自动紧凑 GC

---

## 后端说明

当前持久化设计最适合这样的后端：

* `WriteBlockSize` 与真实存储介质写入粒度一致
* `Erase` 会把整个持久化区域或请求区域清成同一种擦除字节
* `ErasedValue` 与后端擦除态一致，通常是 `0xFF`

对于 STM32 内部 Flash，可以通过类似 Yorunvm 这样的底层 NVM 层来适配。

---

## 状态码

主要状态码包括：

* `YORUKV_OK`
* `YORUKV_ERROR`
* `YORUKV_INVALID_PARAM`
* `YORUKV_NOT_INITIALIZED`
* `YORUKV_NOT_FOUND`
* `YORUKV_TYPE_MISMATCH`
* `YORUKV_READ_ONLY`
* `YORUKV_NO_SPACE`

---

## 当前阶段验证情况

当前阶段已经通过两种方式验证：

* 主机侧回调后端测试
* STM32H743 真机内部 Flash 测试，包括：
  * 日志追加写
  * 重载恢复
  * 删除记录
  * 删除后恢复默认值

这意味着当前单区域日志路径不只是主机模拟，而是已经在真实 STM32 内部 Flash 上跑通过了。
