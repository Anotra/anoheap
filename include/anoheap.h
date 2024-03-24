// MIT License
// Copyright (c) 2024 Anotra
// https://github.com/Anotra/anoheap

#pragma once

#ifndef ANOHEAP_H
#define ANOHEAP_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ANOHEAP_MAX_KEY_SIZE 4096
#define ANOHEAP_MAX_VAL_SIZE 8192

#define ANOHEAP_DECLARE_COMPARE_FUNCTION(function_name, data_type)            \
  static int                                                                  \
  function_name(const void *a, const void *b) {                               \
    if (*(data_type *)a == *(data_type *)b) return 0;                         \
    return *(data_type *)a > *(data_type *)b ? 1 : -1;                        \
  }


typedef struct anoheap anoheap;
typedef unsigned anoheap_id;

typedef enum {
  anoheap_min      = 0 << 0,
  anoheap_max      = 1 << 0,
} anoheap_options;

anoheap *anoheap_create(size_t key_size, size_t val_size,
  int (*cmp)(const void *, const void *), anoheap_options options);
void anoheap_destroy(anoheap *h);
size_t anoheap_enabled_count(anoheap *h);
size_t anoheap_disabled_count(anoheap *h);
size_t anoheap_total_count(anoheap *h);

anoheap_id anoheap_add_key(anoheap *h, const void *key, bool enable);
anoheap_id anoheap_add(anoheap *h, const void *key, const void *val,
                       bool enable);
bool anoheap_delete(anoheap *h, anoheap_id id);

bool anoheap_exists(anoheap *h, anoheap_id id);
bool anoheap_is_enabled(anoheap *h, anoheap_id id);

bool anoheap_enable(anoheap *h, anoheap_id id);
bool anoheap_disable(anoheap *h, anoheap_id id);

bool anoheap_update(anoheap *h, anoheap_id id,
                    const void *key, const void *val);
bool anoheap_update_key(anoheap *h, anoheap_id id, const void *key);
bool anoheap_update_val(anoheap *h, anoheap_id id, const void *val);

bool anoheap_get(anoheap *h, anoheap_id id, void *key, void *val);
bool anoheap_get_key(anoheap *h, anoheap_id id, void *key);
bool anoheap_get_val(anoheap *h, anoheap_id id, void *val);
void *anoheap_get_val_direct(anoheap *h, anoheap_id id);

anoheap_id anoheap_peek_id(anoheap *h);
anoheap_id anoheap_peek_key(anoheap *h, void *key);
anoheap_id anoheap_peek(anoheap *h, void *key, void *val);
anoheap_id anoheap_pop_id(anoheap *h, bool delete_item);
anoheap_id anoheap_pop_key(anoheap *h, void *key, bool delete_item);
anoheap_id anoheap_pop(anoheap *h, void *key, void *val, bool delete_item);

#ifdef __cplusplus
}
#endif

#endif // !ANOHEAP_H
