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

#ifndef YORUKV_ENABLE_PERSIST
#define YORUKV_ENABLE_PERSIST 1
#endif

#ifndef YORUKV_PERSIST_AUTO_GC
#define YORUKV_PERSIST_AUTO_GC 1
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

typedef YORUKV_StatusTypeDef (*YORUKV_PersistReadFunc)(void *user, yorukv_u32_t offset, void *dst, yorukv_u32_t len);
typedef YORUKV_StatusTypeDef (*YORUKV_PersistWriteFunc)(void *user, yorukv_u32_t offset, const void *src, yorukv_u32_t len);
typedef YORUKV_StatusTypeDef (*YORUKV_PersistEraseFunc)(void *user, yorukv_u32_t offset, yorukv_u32_t len);

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

typedef struct
{
    const YORUKV_ItemTypeDef *Table;
    yorukv_u32_t Count;
    yorukv_u8_t Initialized;
#if YORUKV_ENABLE_PERSIST
    const YORUKV_PersistConfigTypeDef *Persist;
    yorukv_u32_t PersistWriteOffset;
    yorukv_u32_t PersistNextSeq;
#endif
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

static inline const YORUKV_ItemTypeDef *yorukv__find_n_(const YORUKV_HandleTypeDef *hkv, const char *key, yorukv_u32_t key_len)
{
    yorukv_u32_t i;

    if (!hkv || !key) return (const YORUKV_ItemTypeDef *)0;

    for (i = 0u; i < hkv->Count; ++i) {
        const YORUKV_ItemTypeDef *item = &hkv->Table[i];
        const char *p = item->Key;
        yorukv_u32_t n = 0u;

        if (!p) continue;
        while ((n < key_len) && (p[n] != '\0') && (p[n] == key[n])) {
            ++n;
        }

        if ((n == key_len) && (p[n] == '\0')) {
            return item;
        }
    }

    return (const YORUKV_ItemTypeDef *)0;
}

static inline void yorukv__copy_bytes_(void *dst, const void *src, yorukv_u32_t len)
{
    yorukv_u8_t *d = (yorukv_u8_t *)dst;
    const yorukv_u8_t *s = (const yorukv_u8_t *)src;

    while (len-- != 0u) {
        *d++ = *s++;
    }
}

static inline yorukv_u32_t yorukv__read_u32_le_(const yorukv_u8_t *p)
{
    return ((yorukv_u32_t)p[0])
         | ((yorukv_u32_t)p[1] << 8)
         | ((yorukv_u32_t)p[2] << 16)
         | ((yorukv_u32_t)p[3] << 24);
}

static inline void yorukv__write_u16_le_(yorukv_u8_t *p, unsigned value)
{
    p[0] = (yorukv_u8_t)(value & 0xFFu);
    p[1] = (yorukv_u8_t)((value >> 8) & 0xFFu);
}

static inline void yorukv__write_u32_le_(yorukv_u8_t *p, yorukv_u32_t value)
{
    p[0] = (yorukv_u8_t)(value & 0xFFu);
    p[1] = (yorukv_u8_t)((value >> 8) & 0xFFu);
    p[2] = (yorukv_u8_t)((value >> 16) & 0xFFu);
    p[3] = (yorukv_u8_t)((value >> 24) & 0xFFu);
}

static inline yorukv_u32_t yorukv__align_up_(yorukv_u32_t value, yorukv_u32_t align)
{
    if (align <= 1u) return value;
    return ((value + align - 1u) / align) * align;
}

static inline YORUKV_StatusTypeDef yorukv__set_value_to_item_(const YORUKV_ItemTypeDef *item, const YORUKV_ValueTypeDef *value)
{
    if (!item || !value) return YORUKV_INVALID_PARAM;
    if (item->Type != value->Type) return YORUKV_TYPE_MISMATCH;

    switch (item->Type) {
        case YORUKV_TYPE_BOOL:
            *(yorukv_bool_t *)item->DataPtr = value->Value.Bool;
            return YORUKV_OK;
        case YORUKV_TYPE_I32:
            *(yorukv_i32_t *)item->DataPtr = value->Value.I32;
            return YORUKV_OK;
        case YORUKV_TYPE_U32:
            *(yorukv_u32_t *)item->DataPtr = value->Value.U32;
            return YORUKV_OK;
        case YORUKV_TYPE_STRING:
            if (!value->Value.String.Ptr) return YORUKV_INVALID_PARAM;
            if ((item->Capacity == 0u) || (value->Value.String.Len + 1u > item->Capacity)) return YORUKV_NO_SPACE;
            yorukv__copy_str_((char *)item->DataPtr, value->Value.String.Ptr, value->Value.String.Len);
            ((char *)item->DataPtr)[value->Value.String.Len] = '\0';
            return YORUKV_OK;
        default:
            return YORUKV_ERROR;
    }
}

static inline YORUKV_StatusTypeDef yorukv__reset_item_(const YORUKV_ItemTypeDef *item)
{
    YORUKV_ValueTypeDef value;

    if (!item || !item->DefaultPtr) return YORUKV_ERROR;

    value.Type = item->Type;
    switch (item->Type) {
        case YORUKV_TYPE_BOOL:
            value.Value.Bool = *(const yorukv_bool_t *)item->DefaultPtr;
            break;
        case YORUKV_TYPE_I32:
            value.Value.I32 = *(const yorukv_i32_t *)item->DefaultPtr;
            break;
        case YORUKV_TYPE_U32:
            value.Value.U32 = *(const yorukv_u32_t *)item->DefaultPtr;
            break;
        case YORUKV_TYPE_STRING:
            value.Value.String.Ptr = (const char *)item->DefaultPtr;
            value.Value.String.Len = yorukv__strlen_((const char *)item->DefaultPtr);
            break;
        default:
            return YORUKV_ERROR;
    }

    return yorukv__set_value_to_item_(item, &value);
}

static inline YORUKV_StatusTypeDef yorukv__value_from_item_current_(const YORUKV_ItemTypeDef *item, YORUKV_ValueTypeDef *value)
{
    if (!item || !value) return YORUKV_INVALID_PARAM;

    value->Type = item->Type;
    switch (item->Type) {
        case YORUKV_TYPE_BOOL:
            value->Value.Bool = *(const yorukv_bool_t *)item->DataPtr;
            return YORUKV_OK;
        case YORUKV_TYPE_I32:
            value->Value.I32 = *(const yorukv_i32_t *)item->DataPtr;
            return YORUKV_OK;
        case YORUKV_TYPE_U32:
            value->Value.U32 = *(const yorukv_u32_t *)item->DataPtr;
            return YORUKV_OK;
        case YORUKV_TYPE_STRING:
            value->Value.String.Ptr = (const char *)item->DataPtr;
            value->Value.String.Len = yorukv__strlen_((const char *)item->DataPtr);
            return YORUKV_OK;
        default:
            return YORUKV_ERROR;
    }
}

#if YORUKV_ENABLE_PERSIST
#define YORUKV_PERSIST_MAGIC 0x4B59u
#define YORUKV_PERSIST_HEADER_SIZE 16u
#define YORUKV_PERSIST_FLAG_VALID   0x0001u
#define YORUKV_PERSIST_FLAG_DELETED 0x0002u

static inline yorukv_u32_t yorukv__persist_checksum_(const char *key, yorukv_u32_t key_len, const YORUKV_ValueTypeDef *value, yorukv_u32_t seq, unsigned flags)
{
    yorukv_u32_t sum = 0x314B5659u;
    yorukv_u32_t i;

    sum += key_len;
    sum += (yorukv_u32_t)value->Type << 8;
    sum += seq;
    sum += (yorukv_u32_t)flags << 16;

    for (i = 0u; i < key_len; ++i) {
        sum = (sum << 5) - sum + (yorukv_u8_t)key[i];
    }

    switch (value->Type) {
        case YORUKV_TYPE_BOOL:
            sum = (sum << 5) - sum + (yorukv_u8_t)value->Value.Bool;
            break;
        case YORUKV_TYPE_I32:
        {
            yorukv_u8_t buf[4];
            yorukv__write_u32_le_(buf, (yorukv_u32_t)value->Value.I32);
            for (i = 0u; i < 4u; ++i) sum = (sum << 5) - sum + buf[i];
            break;
        }
        case YORUKV_TYPE_U32:
        {
            yorukv_u8_t buf[4];
            yorukv__write_u32_le_(buf, value->Value.U32);
            for (i = 0u; i < 4u; ++i) sum = (sum << 5) - sum + buf[i];
            break;
        }
        case YORUKV_TYPE_STRING:
            for (i = 0u; i < value->Value.String.Len; ++i) {
                sum = (sum << 5) - sum + (yorukv_u8_t)value->Value.String.Ptr[i];
            }
            break;
        default:
            break;
    }

    return sum;
}

static inline yorukv_u32_t yorukv__persist_value_len_(const YORUKV_ValueTypeDef *value)
{
    switch (value->Type) {
        case YORUKV_TYPE_BOOL: return 1u;
        case YORUKV_TYPE_I32: return 4u;
        case YORUKV_TYPE_U32: return 4u;
        case YORUKV_TYPE_STRING: return value->Value.String.Len;
        default: return 0u;
    }
}

static inline YORUKV_StatusTypeDef yorukv__persist_append_flags_(YORUKV_HandleTypeDef *hkv, const char *key, const YORUKV_ValueTypeDef *value, unsigned flags)
{
    yorukv_u8_t record[512];
    yorukv_u32_t key_len;
    yorukv_u32_t value_len;
    yorukv_u32_t raw_len;
    yorukv_u32_t total_len;
    yorukv_u32_t seq;
    yorukv_u32_t block_size;
    yorukv_u32_t value_offset;
    yorukv_u32_t i;

    if (!hkv || !hkv->Persist || !key || !value) return YORUKV_INVALID_PARAM;

    key_len = yorukv__strlen_(key);
    value_len = yorukv__persist_value_len_(value);
    seq = hkv->PersistNextSeq;
    block_size = (hkv->Persist->WriteBlockSize == 0u) ? 1u : hkv->Persist->WriteBlockSize;
    raw_len = YORUKV_PERSIST_HEADER_SIZE + key_len + value_len;
    total_len = yorukv__align_up_(raw_len, block_size);
    value_offset = YORUKV_PERSIST_HEADER_SIZE + key_len;

    if ((key_len == 0u) || (key_len > 255u) || (value_len > 65535u) || (total_len > (yorukv_u32_t)sizeof(record))) return YORUKV_INVALID_PARAM;
    if (hkv->PersistWriteOffset + total_len > hkv->Persist->Size) return YORUKV_NO_SPACE;

    for (i = 0u; i < total_len; ++i) {
        record[i] = hkv->Persist->ErasedValue;
    }

    record[0] = (yorukv_u8_t)(YORUKV_PERSIST_MAGIC & 0xFFu);
    record[1] = (yorukv_u8_t)((YORUKV_PERSIST_MAGIC >> 8) & 0xFFu);
    record[2] = (yorukv_u8_t)key_len;
    record[3] = value->Type;
    yorukv__write_u16_le_(&record[4], (unsigned)value_len);
    yorukv__write_u16_le_(&record[6], flags);
    yorukv__write_u32_le_(&record[8], seq);
    yorukv__write_u32_le_(&record[12], yorukv__persist_checksum_(key, key_len, value, seq, flags));
    yorukv__copy_bytes_(&record[YORUKV_PERSIST_HEADER_SIZE], key, key_len);

    switch (value->Type) {
        case YORUKV_TYPE_BOOL:
            record[value_offset] = value->Value.Bool;
            break;
        case YORUKV_TYPE_I32:
            yorukv__copy_bytes_(&record[value_offset], &value->Value.I32, 4u);
            break;
        case YORUKV_TYPE_U32:
            yorukv__copy_bytes_(&record[value_offset], &value->Value.U32, 4u);
            break;
        case YORUKV_TYPE_STRING:
            if (value_len != 0u) {
                yorukv__copy_bytes_(&record[value_offset], value->Value.String.Ptr, value_len);
            }
            break;
        default:
            return YORUKV_ERROR;
    }

    if (hkv->Persist->Write(hkv->Persist->User, hkv->PersistWriteOffset, record, total_len) != YORUKV_OK) return YORUKV_ERROR;

    hkv->PersistWriteOffset += total_len;
    hkv->PersistNextSeq = seq + 1u;
    return YORUKV_OK;
}

static inline YORUKV_StatusTypeDef yorukv__persist_append_(YORUKV_HandleTypeDef *hkv, const char *key, const YORUKV_ValueTypeDef *value)
{
    return yorukv__persist_append_flags_(hkv, key, value, YORUKV_PERSIST_FLAG_VALID);
}

static inline YORUKV_StatusTypeDef yorukv__persist_gc_(YORUKV_HandleTypeDef *hkv)
{
    yorukv_u32_t i;

    if (!hkv || !hkv->Persist) return YORUKV_INVALID_PARAM;
    if (!hkv->Initialized) return YORUKV_NOT_INITIALIZED;

    if (hkv->Persist->Erase(hkv->Persist->User, 0u, hkv->Persist->Size) != YORUKV_OK) return YORUKV_ERROR;

    hkv->PersistWriteOffset = 0u;
    hkv->PersistNextSeq = 1u;

    for (i = 0u; i < hkv->Count; ++i) {
        const YORUKV_ItemTypeDef *item = &hkv->Table[i];
        YORUKV_ValueTypeDef value;
        YORUKV_StatusTypeDef status;

        status = yorukv__value_from_item_current_(item, &value);
        if (status != YORUKV_OK) return status;

        status = yorukv__persist_append_(hkv, item->Key, &value);
        if (status != YORUKV_OK) return status;
    }

    return YORUKV_OK;
}

static inline YORUKV_StatusTypeDef yorukv__persist_append_with_gc_(YORUKV_HandleTypeDef *hkv, const char *key, const YORUKV_ValueTypeDef *value)
{
    YORUKV_StatusTypeDef status = yorukv__persist_append_flags_(hkv, key, value, YORUKV_PERSIST_FLAG_VALID);

#if YORUKV_PERSIST_AUTO_GC
    if (status == YORUKV_NO_SPACE) {
        status = yorukv__persist_gc_(hkv);
        if (status != YORUKV_OK) return status;
        status = yorukv__persist_append_flags_(hkv, key, value, YORUKV_PERSIST_FLAG_VALID);
    }
#endif

    return status;
}

static inline YORUKV_StatusTypeDef yorukv__persist_delete_with_gc_(YORUKV_HandleTypeDef *hkv, const char *key, const YORUKV_ItemTypeDef *item)
{
    YORUKV_ValueTypeDef value;
    YORUKV_StatusTypeDef status;

    status = yorukv__value_from_item_current_(item, &value);
    if (status != YORUKV_OK) return status;

    status = yorukv__persist_append_flags_(hkv, key, &value, YORUKV_PERSIST_FLAG_DELETED);
#if YORUKV_PERSIST_AUTO_GC
    if (status == YORUKV_NO_SPACE) {
        status = yorukv__persist_gc_(hkv);
        if (status != YORUKV_OK) return status;
        status = yorukv__persist_append_flags_(hkv, key, &value, YORUKV_PERSIST_FLAG_DELETED);
    }
#endif
    return status;
}
#endif

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
#if YORUKV_ENABLE_PERSIST
    hkv->Persist = (const YORUKV_PersistConfigTypeDef *)0;
    hkv->PersistWriteOffset = 0u;
    hkv->PersistNextSeq = 1u;
#endif
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

#if YORUKV_ENABLE_PERSIST
    if (hkv->Persist) {
        YORUKV_StatusTypeDef persist_status = yorukv__persist_append_with_gc_(hkv, key, value);
        if (persist_status != YORUKV_OK) return persist_status;
    }
#endif

    YORUKV_LOCK();
    {
        YORUKV_StatusTypeDef status = yorukv__set_value_to_item_(item, value);
        YORUKV_UNLOCK();
        return status;
    }
}

static inline YORUKV_StatusTypeDef YORUKV_Reset(YORUKV_HandleTypeDef *hkv, const char *key)
{
    const YORUKV_ItemTypeDef *item;

    if (!hkv || !key) return YORUKV_INVALID_PARAM;
    if (!hkv->Initialized) return YORUKV_NOT_INITIALIZED;

    item = yorukv__find_(hkv, key);
    if (!item) return YORUKV_NOT_FOUND;
    if (!item->DefaultPtr) return YORUKV_ERROR;

#if YORUKV_ENABLE_PERSIST
    if (hkv->Persist) {
        YORUKV_ValueTypeDef value;

        value.Type = item->Type;
        switch (item->Type) {
            case YORUKV_TYPE_BOOL:
                value.Value.Bool = *(const yorukv_bool_t *)item->DefaultPtr;
                break;
            case YORUKV_TYPE_I32:
                value.Value.I32 = *(const yorukv_i32_t *)item->DefaultPtr;
                break;
            case YORUKV_TYPE_U32:
                value.Value.U32 = *(const yorukv_u32_t *)item->DefaultPtr;
                break;
            case YORUKV_TYPE_STRING:
                value.Value.String.Ptr = (const char *)item->DefaultPtr;
                value.Value.String.Len = yorukv__strlen_((const char *)item->DefaultPtr);
                break;
            default:
                return YORUKV_ERROR;
        }

        {
            YORUKV_StatusTypeDef persist_status = yorukv__persist_append_with_gc_(hkv, key, &value);
            if (persist_status != YORUKV_OK) return persist_status;
        }
    }
#endif

    YORUKV_LOCK();
    {
        YORUKV_StatusTypeDef status = yorukv__reset_item_(item);
        YORUKV_UNLOCK();
        return status;
    }
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

static inline YORUKV_StatusTypeDef YORUKV_Delete(YORUKV_HandleTypeDef *hkv, const char *key)
{
    const YORUKV_ItemTypeDef *item;

    if (!hkv || !key) return YORUKV_INVALID_PARAM;
    if (!hkv->Initialized) return YORUKV_NOT_INITIALIZED;

    item = yorukv__find_(hkv, key);
    if (!item) return YORUKV_NOT_FOUND;
    if ((item->Flags & YORUKV_FLAG_WRITABLE) == 0u) return YORUKV_READ_ONLY;
    if (!item->DefaultPtr) return YORUKV_ERROR;

#if YORUKV_ENABLE_PERSIST
    if (hkv->Persist) {
        YORUKV_StatusTypeDef persist_status = yorukv__persist_delete_with_gc_(hkv, key, item);
        if (persist_status != YORUKV_OK) return persist_status;
    }
#endif

    YORUKV_LOCK();
    {
        YORUKV_StatusTypeDef status = yorukv__reset_item_(item);
        YORUKV_UNLOCK();
        return status;
    }
}

#if YORUKV_ENABLE_PERSIST
static inline YORUKV_StatusTypeDef YORUKV_AttachPersist(YORUKV_HandleTypeDef *hkv, const YORUKV_PersistConfigTypeDef *persist)
{
    if (!hkv || !persist || !persist->Read || !persist->Write || !persist->Erase || (persist->Size < YORUKV_PERSIST_HEADER_SIZE)) {
        return YORUKV_INVALID_PARAM;
    }
    if (!hkv->Initialized) return YORUKV_NOT_INITIALIZED;
    if ((persist->WriteBlockSize != 0u) && (persist->Size % persist->WriteBlockSize != 0u)) return YORUKV_INVALID_PARAM;

    hkv->Persist = persist;
    hkv->PersistWriteOffset = 0u;
    hkv->PersistNextSeq = 1u;
    return YORUKV_OK;
}

static inline YORUKV_StatusTypeDef YORUKV_FormatPersist(YORUKV_HandleTypeDef *hkv)
{
    if (!hkv || !hkv->Persist) return YORUKV_INVALID_PARAM;
    if (!hkv->Initialized) return YORUKV_NOT_INITIALIZED;
    if (hkv->Persist->Erase(hkv->Persist->User, 0u, hkv->Persist->Size) != YORUKV_OK) return YORUKV_ERROR;
    hkv->PersistWriteOffset = 0u;
    hkv->PersistNextSeq = 1u;
    return YORUKV_OK;
}

static inline YORUKV_StatusTypeDef YORUKV_LoadPersist(YORUKV_HandleTypeDef *hkv)
{
    yorukv_u32_t i;
    yorukv_u32_t offset = 0u;
    yorukv_u32_t max_seq = 0u;
    yorukv_u8_t header[YORUKV_PERSIST_HEADER_SIZE];

    if (!hkv || !hkv->Persist) return YORUKV_INVALID_PARAM;
    if (!hkv->Initialized) return YORUKV_NOT_INITIALIZED;

    for (i = 0u; i < hkv->Count; ++i) {
        YORUKV_StatusTypeDef status = yorukv__reset_item_(&hkv->Table[i]);
        if (status != YORUKV_OK) return status;
    }

    while (offset + YORUKV_PERSIST_HEADER_SIZE <= hkv->Persist->Size) {
        yorukv_u32_t key_len;
        yorukv_u32_t value_len;
        yorukv_u32_t seq;
        yorukv_u32_t total_len;
        yorukv_u32_t stored_crc;
        yorukv_u32_t flags;
        yorukv_u8_t type;
        const YORUKV_ItemTypeDef *item;
        char key_buf[256];
        YORUKV_ValueTypeDef value;
        yorukv_u32_t block_size = (hkv->Persist->WriteBlockSize == 0u) ? 1u : hkv->Persist->WriteBlockSize;

        if (hkv->Persist->Read(hkv->Persist->User, offset, header, YORUKV_PERSIST_HEADER_SIZE) != YORUKV_OK) return YORUKV_ERROR;

        if ((header[0] == hkv->Persist->ErasedValue) && (header[1] == hkv->Persist->ErasedValue) &&
            (header[2] == hkv->Persist->ErasedValue) && (header[3] == hkv->Persist->ErasedValue)) {
            break;
        }

        if (((unsigned)header[0] | ((unsigned)header[1] << 8)) != YORUKV_PERSIST_MAGIC) {
            break;
        }

        key_len = header[2];
        type = header[3];
        value_len = (yorukv_u32_t)header[4] | ((yorukv_u32_t)header[5] << 8);
        flags = (yorukv_u32_t)header[6] | ((yorukv_u32_t)header[7] << 8);
        seq = yorukv__read_u32_le_(&header[8]);
        stored_crc = yorukv__read_u32_le_(&header[12]);
        total_len = yorukv__align_up_(YORUKV_PERSIST_HEADER_SIZE + key_len + value_len, block_size);

        if ((key_len == 0u) || (key_len >= (yorukv_u32_t)sizeof(key_buf)) || (offset + total_len > hkv->Persist->Size)) {
            break;
        }

        if (hkv->Persist->Read(hkv->Persist->User, offset + YORUKV_PERSIST_HEADER_SIZE, key_buf, key_len) != YORUKV_OK) return YORUKV_ERROR;
        key_buf[key_len] = '\0';

        item = yorukv__find_n_(hkv, key_buf, key_len);
        if (item) {
            value.Type = type;
            switch (type) {
                case YORUKV_TYPE_BOOL:
                    if (value_len == 1u) {
                        if (hkv->Persist->Read(hkv->Persist->User, offset + YORUKV_PERSIST_HEADER_SIZE + key_len, &value.Value.Bool, 1u) != YORUKV_OK) return YORUKV_ERROR;
                    } else {
                        item = (const YORUKV_ItemTypeDef *)0;
                    }
                    break;
                case YORUKV_TYPE_I32:
                    if (value_len == 4u) {
                        if (hkv->Persist->Read(hkv->Persist->User, offset + YORUKV_PERSIST_HEADER_SIZE + key_len, &value.Value.I32, 4u) != YORUKV_OK) return YORUKV_ERROR;
                    } else {
                        item = (const YORUKV_ItemTypeDef *)0;
                    }
                    break;
                case YORUKV_TYPE_U32:
                    if (value_len == 4u) {
                        if (hkv->Persist->Read(hkv->Persist->User, offset + YORUKV_PERSIST_HEADER_SIZE + key_len, &value.Value.U32, 4u) != YORUKV_OK) return YORUKV_ERROR;
                    } else {
                        item = (const YORUKV_ItemTypeDef *)0;
                    }
                    break;
                case YORUKV_TYPE_STRING:
                    if ((item->Capacity != 0u) && (value_len + 1u <= item->Capacity)) {
                        value.Value.String.Ptr = (const char *)item->DataPtr;
                        value.Value.String.Len = value_len;
                        if (hkv->Persist->Read(hkv->Persist->User, offset + YORUKV_PERSIST_HEADER_SIZE + key_len, item->DataPtr, value_len) != YORUKV_OK) return YORUKV_ERROR;
                        ((char *)item->DataPtr)[value_len] = '\0';
                    } else {
                        item = (const YORUKV_ItemTypeDef *)0;
                    }
                    break;
                default:
                    item = (const YORUKV_ItemTypeDef *)0;
                    break;
            }

            if (item) {
                if (stored_crc == yorukv__persist_checksum_(key_buf, key_len, &value, seq, (unsigned)flags)) {
                    if ((flags & YORUKV_PERSIST_FLAG_DELETED) != 0u) {
                        YORUKV_StatusTypeDef status = yorukv__reset_item_(item);
                        if (status != YORUKV_OK) return status;
                    } else {
                        if (type != YORUKV_TYPE_STRING) {
                            YORUKV_StatusTypeDef status = yorukv__set_value_to_item_(item, &value);
                            if (status != YORUKV_OK) return status;
                        }
                    }
                    if ((flags & (YORUKV_PERSIST_FLAG_VALID | YORUKV_PERSIST_FLAG_DELETED)) != 0u) {
                        if (seq > max_seq) max_seq = seq;
                    }
                }
            }
        }

        offset += total_len;
    }

    hkv->PersistWriteOffset = offset;
    hkv->PersistNextSeq = max_seq + 1u;
    if (hkv->PersistNextSeq == 0u) hkv->PersistNextSeq = 1u;
    return YORUKV_OK;
}

static inline YORUKV_StatusTypeDef YORUKV_RunGC(YORUKV_HandleTypeDef *hkv)
{
    return yorukv__persist_gc_(hkv);
}
#endif

#endif /* YORUKV_ENABLE */

#ifdef __cplusplus
}
#endif

#endif /* YORUKV_H */
