#ifndef YORUKV_H
#define YORUKV_H

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================
 *  Minimal Integer Types (no stdint/stddef dependency)
 * ========================================================= */

typedef
#if defined(__UINT8_TYPE__)
__UINT8_TYPE__
#else
unsigned char
#endif
yorukv_u8_t;

typedef
#if defined(__INT32_TYPE__)
__INT32_TYPE__
#else
signed int
#endif
yorukv_i32_t;

typedef
#if defined(__UINT32_TYPE__)
__UINT32_TYPE__
#else
unsigned int
#endif
yorukv_u32_t;

typedef yorukv_u8_t yorukv_bool_t;

/* =========================================================
 *  User Configuration
 * ========================================================= */

#ifndef YORUKV_ENABLE
#define YORUKV_ENABLE 1
#endif

#ifndef YORUKV_LOCK
#define YORUKV_LOCK()   do{}while(0)
#endif

#ifndef YORUKV_UNLOCK
#define YORUKV_UNLOCK() do{}while(0)
#endif

#ifndef YORUKV_ARRAY_SIZE
#define YORUKV_ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

/* =========================================================
 *  Type Definitions
 * ========================================================= */

typedef enum
{
    YORUKV_OK = 0,
    YORUKV_ERROR = 1,
    YORUKV_INVALID_PARAM = 2,
    YORUKV_NOT_INITIALIZED = 3,
    YORUKV_NOT_FOUND = 4,
    YORUKV_TYPE_MISMATCH = 5,
    YORUKV_READ_ONLY = 6,
    YORUKV_NO_SPACE = 7
} YORUKV_StatusTypeDef;

typedef enum
{
    YORUKV_TYPE_BOOL = 1,
    YORUKV_TYPE_I32 = 2,
    YORUKV_TYPE_U32 = 3,
    YORUKV_TYPE_STRING = 4
} YORUKV_ValueKindTypeDef;

typedef struct
{
    const char *Ptr;
    yorukv_u32_t Len;
} YORUKV_StringViewTypeDef;

typedef struct
{
    yorukv_u8_t Type;
    union
    {
        yorukv_bool_t Bool;
        yorukv_i32_t I32;
        yorukv_u32_t U32;
        YORUKV_StringViewTypeDef String;
    } Value;
} YORUKV_ValueTypeDef;

typedef struct
{
    const char *Key;
    yorukv_u8_t Type;
    yorukv_u8_t Flags;
    void *DataPtr;
    const void *DefaultPtr;
    yorukv_u32_t Capacity;
} YORUKV_ItemTypeDef;

typedef struct
{
    const YORUKV_ItemTypeDef *Table;
    yorukv_u32_t Count;
    yorukv_u8_t Initialized;
} YORUKV_HandleTypeDef;

typedef void (*YORUKV_ListCallbackTypeDef)(const YORUKV_ItemTypeDef *item, void *user);

#define YORUKV_FLAG_WRITABLE 0x01u

#define YORUKV_ITEM_BOOL_RW(key, data_ptr, default_ptr) \
    { (key), YORUKV_TYPE_BOOL, YORUKV_FLAG_WRITABLE, (void *)(data_ptr), (const void *)(default_ptr), 0u }

#define YORUKV_ITEM_I32_RW(key, data_ptr, default_ptr) \
    { (key), YORUKV_TYPE_I32, YORUKV_FLAG_WRITABLE, (void *)(data_ptr), (const void *)(default_ptr), 0u }

#define YORUKV_ITEM_U32_RW(key, data_ptr, default_ptr) \
    { (key), YORUKV_TYPE_U32, YORUKV_FLAG_WRITABLE, (void *)(data_ptr), (const void *)(default_ptr), 0u }

#define YORUKV_ITEM_STR_RW(key, data_ptr, capacity, default_ptr) \
    { (key), YORUKV_TYPE_STRING, YORUKV_FLAG_WRITABLE, (void *)(data_ptr), (const void *)(default_ptr), (yorukv_u32_t)(capacity) }

/* Global singleton storage.
 * Define YORUKV_DEFINE_GLOBALS in exactly one .c file before including yorukv.h.
 */
#if defined(YORUKV_DEFINE_GLOBALS)
YORUKV_HandleTypeDef hYorukv;
#else
extern YORUKV_HandleTypeDef hYorukv;
#endif

/* =========================================================
 *  Internal Helpers
 * ========================================================= */

static inline unsigned yorukv__streq_(const char *a, const char *b)
{
    if (!a || !b) return 0u;
    while ((*a != '\0') && (*b != '\0')) {
        if (*a != *b) return 0u;
        ++a;
        ++b;
    }
    return (unsigned)(*a == *b);
}

static inline yorukv_u32_t yorukv__strlen_(const char *s)
{
    yorukv_u32_t n = 0u;
    if (!s) return 0u;
    while (*s != '\0') {
        ++s;
        ++n;
    }
    return n;
}

static inline void yorukv__copy_str_(char *dst, const char *src, yorukv_u32_t len)
{
    while (len-- != 0u) {
        *dst++ = *src++;
    }
}

static inline const YORUKV_ItemTypeDef *yorukv__find_(const YORUKV_HandleTypeDef *hkv, const char *key)
{
    yorukv_u32_t i;

    if (!hkv || !key) return (const YORUKV_ItemTypeDef *)0;

    for (i = 0u; i < hkv->Count; ++i) {
        const YORUKV_ItemTypeDef *item = &hkv->Table[i];
        if (yorukv__streq_(item->Key, key)) {
            return item;
        }
    }

    return (const YORUKV_ItemTypeDef *)0;
}

/* =========================================================
 *  API
 * ========================================================= */

#if YORUKV_ENABLE

static inline YORUKV_StatusTypeDef YORUKV_Init(YORUKV_HandleTypeDef *hkv, const YORUKV_ItemTypeDef *table, yorukv_u32_t count)
{
    if (!hkv || (!table && (count != 0u))) {
        return YORUKV_INVALID_PARAM;
    }

    hkv->Table = table;
    hkv->Count = count;
    hkv->Initialized = 1u;
    return YORUKV_OK;
}

static inline YORUKV_StatusTypeDef YORUKV_Get(YORUKV_HandleTypeDef *hkv, const char *key, YORUKV_ValueTypeDef *out)
{
    const YORUKV_ItemTypeDef *item;

    if (!hkv || !key || !out) return YORUKV_INVALID_PARAM;
    if (!hkv->Initialized) return YORUKV_NOT_INITIALIZED;

    item = yorukv__find_(hkv, key);
    if (!item) return YORUKV_NOT_FOUND;

    out->Type = item->Type;

    switch (item->Type) {
        case YORUKV_TYPE_BOOL:
            out->Value.Bool = *(const yorukv_bool_t *)item->DataPtr;
            return YORUKV_OK;
        case YORUKV_TYPE_I32:
            out->Value.I32 = *(const yorukv_i32_t *)item->DataPtr;
            return YORUKV_OK;
        case YORUKV_TYPE_U32:
            out->Value.U32 = *(const yorukv_u32_t *)item->DataPtr;
            return YORUKV_OK;
        case YORUKV_TYPE_STRING:
            out->Value.String.Ptr = (const char *)item->DataPtr;
            out->Value.String.Len = yorukv__strlen_((const char *)item->DataPtr);
            return YORUKV_OK;
        default:
            return YORUKV_ERROR;
    }
}

static inline YORUKV_StatusTypeDef YORUKV_Set(YORUKV_HandleTypeDef *hkv, const char *key, const YORUKV_ValueTypeDef *value)
{
    const YORUKV_ItemTypeDef *item;

    if (!hkv || !key || !value) return YORUKV_INVALID_PARAM;
    if (!hkv->Initialized) return YORUKV_NOT_INITIALIZED;

    item = yorukv__find_(hkv, key);
    if (!item) return YORUKV_NOT_FOUND;
    if ((item->Flags & YORUKV_FLAG_WRITABLE) == 0u) return YORUKV_READ_ONLY;
    if (item->Type != value->Type) return YORUKV_TYPE_MISMATCH;

    YORUKV_LOCK();
    switch (item->Type) {
        case YORUKV_TYPE_BOOL:
            *(yorukv_bool_t *)item->DataPtr = value->Value.Bool;
            break;
        case YORUKV_TYPE_I32:
            *(yorukv_i32_t *)item->DataPtr = value->Value.I32;
            break;
        case YORUKV_TYPE_U32:
            *(yorukv_u32_t *)item->DataPtr = value->Value.U32;
            break;
        case YORUKV_TYPE_STRING:
            if (!value->Value.String.Ptr) {
                YORUKV_UNLOCK();
                return YORUKV_INVALID_PARAM;
            }
            if ((item->Capacity == 0u) || (value->Value.String.Len + 1u > item->Capacity)) {
                YORUKV_UNLOCK();
                return YORUKV_NO_SPACE;
            }
            yorukv__copy_str_((char *)item->DataPtr, value->Value.String.Ptr, value->Value.String.Len);
            ((char *)item->DataPtr)[value->Value.String.Len] = '\0';
            break;
        default:
            YORUKV_UNLOCK();
            return YORUKV_ERROR;
    }
    YORUKV_UNLOCK();

    return YORUKV_OK;
}

static inline YORUKV_StatusTypeDef YORUKV_Reset(YORUKV_HandleTypeDef *hkv, const char *key)
{
    const YORUKV_ItemTypeDef *item;

    if (!hkv || !key) return YORUKV_INVALID_PARAM;
    if (!hkv->Initialized) return YORUKV_NOT_INITIALIZED;

    item = yorukv__find_(hkv, key);
    if (!item) return YORUKV_NOT_FOUND;
    if (!item->DefaultPtr) return YORUKV_ERROR;

    YORUKV_LOCK();
    switch (item->Type) {
        case YORUKV_TYPE_BOOL:
            *(yorukv_bool_t *)item->DataPtr = *(const yorukv_bool_t *)item->DefaultPtr;
            break;
        case YORUKV_TYPE_I32:
            *(yorukv_i32_t *)item->DataPtr = *(const yorukv_i32_t *)item->DefaultPtr;
            break;
        case YORUKV_TYPE_U32:
            *(yorukv_u32_t *)item->DataPtr = *(const yorukv_u32_t *)item->DefaultPtr;
            break;
        case YORUKV_TYPE_STRING:
        {
            yorukv_u32_t len = yorukv__strlen_((const char *)item->DefaultPtr);
            if ((item->Capacity == 0u) || (len + 1u > item->Capacity)) {
                YORUKV_UNLOCK();
                return YORUKV_NO_SPACE;
            }
            yorukv__copy_str_((char *)item->DataPtr, (const char *)item->DefaultPtr, len);
            ((char *)item->DataPtr)[len] = '\0';
            break;
        }
        default:
            YORUKV_UNLOCK();
            return YORUKV_ERROR;
    }
    YORUKV_UNLOCK();

    return YORUKV_OK;
}

static inline YORUKV_StatusTypeDef YORUKV_List(YORUKV_HandleTypeDef *hkv, YORUKV_ListCallbackTypeDef cb, void *user)
{
    yorukv_u32_t i;

    if (!hkv || !cb) return YORUKV_INVALID_PARAM;
    if (!hkv->Initialized) return YORUKV_NOT_INITIALIZED;

    for (i = 0u; i < hkv->Count; ++i) {
        cb(&hkv->Table[i], user);
    }

    return YORUKV_OK;
}

#endif /* YORUKV_ENABLE */

#ifdef __cplusplus
}
#endif

#endif /* YORUKV_H */
