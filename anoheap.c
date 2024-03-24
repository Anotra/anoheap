// MIT License
// Copyright (c) 2024 Anotra
// https://github.com/Anotra/anoheap

#include <stdlib.h>
#include <string.h>

#include "anoheap.h"

#define CHUNK_SIZE 1024

#define MAP_PTR(mpos)                                                         \
  (h->map.arr[(mpos - 1) / 1024] + ((mpos - 1) % 1024))

#define POS_PTR(id)                                                           \
  (h->items.arr[(id - 1) / CHUNK_SIZE].pos + ((id - 1) % CHUNK_SIZE))
#define KEY_PTR(id)                                                           \
  ((void *)(h->items.arr[(id - 1) / CHUNK_SIZE].keys +                        \
    ((id - 1) % CHUNK_SIZE) * h->key_size))
#define VAL_PTR(id)                                                           \
  ((void *)(h->items.arr[(id - 1) / CHUNK_SIZE].vals +                        \
    ((id - 1) % CHUNK_SIZE) * h->val_size))

struct anoheap {
  int (*cmp)(const void *, const void *);
  anoheap_options options;
  size_t key_size : 32;
  size_t val_size : 32;

  struct {
    anoheap_id **arr;
    size_t len;
    size_t cap;
    size_t arr_cap;
  } map;

  struct {
    size_t len;
    size_t cap;
    size_t arr_cap;
    anoheap_id lowest;
    struct {
      size_t len : 32;
      anoheap_id lowest : 32;
      anoheap_id *pos;
      char *keys;
      char *vals;
    } *arr;
  } items;
};

anoheap *
anoheap_create(size_t key_size, size_t val_size,
               int (*cmp)(const void *, const void *),
               anoheap_options options)
{
  struct anoheap *h;
  if (!cmp
   || !key_size || key_size > ANOHEAP_MAX_KEY_SIZE
   ||              val_size > ANOHEAP_MAX_VAL_SIZE
   || !(h = calloc(1, sizeof *h)))
    return NULL;

  h->cmp = cmp;
  h->key_size = key_size;
  h->val_size = val_size;
  h->options = options;
  return h;
}

void
anoheap_destroy(anoheap *h) {
  for (anoheap_id i = 0; i < h->map.cap / 1024; i++)
    free(h->map.arr[i]);
  free(h->map.arr);

  for (anoheap_id i = 0; i < h->items.cap / CHUNK_SIZE; i++) {
    free(h->items.arr[i].keys);
    free(h->items.arr[i].vals);
    free(h->items.arr[i].pos);
  }
  free(h->items.arr);

  memset(h, 0, sizeof *h);
  free(h);
}

size_t
anoheap_enabled_count(anoheap *h) {
  return h->map.len;
}

size_t
anoheap_disabled_count(anoheap *h) {
  return h->items.len - h->map.len;
}

size_t
anoheap_total_count(anoheap *h) {
  return h->items.len;
}

static bool
_anoheap_map_ensure_space(anoheap *h, size_t space_needed) {
  while (h->map.cap - h->map.len < space_needed) {
    size_t array_len = h->map.cap / 1024;
    size_t cap = 8;
    for (; cap && cap < array_len + 1; cap <<= 1);
    if (cap > h->map.arr_cap) {
      void *tmp = realloc(h->map.arr, cap * sizeof *h->map.arr);
      if (!tmp) return false;
      h->map.arr = tmp;
      h->map.arr_cap = cap;
    }
    if (!(h->map.arr[array_len] = malloc(1024 * sizeof *h->map.arr[0])))
      return false;
    h->map.cap += 1024;
  }
  return true;
}

static bool
_anoheap_items_ensure_space(anoheap *h, size_t space_needed) {
  while (h->items.cap - h->items.len < space_needed) {
    size_t chunk = h->items.cap / CHUNK_SIZE;
    void *tmp;
    size_t cap = 8;
    for (; cap && cap < chunk + 1; cap <<= 1);
    if (cap > h->map.arr_cap) {
      if (!(tmp = realloc(h->items.arr, cap * sizeof *h->items.arr)))
        return false;
      h->items.arr = tmp;
      h->items.arr_cap = cap;
    }
    memset(&h->items.arr[chunk], 0, sizeof h->items.arr[0]);
    h->items.arr[chunk].pos = malloc(sizeof *h->items.arr[0].pos * CHUNK_SIZE);
    h->items.arr[chunk].keys = malloc(h->key_size * CHUNK_SIZE);
    if (h->val_size)
      h->items.arr[chunk].vals = malloc(h->val_size * CHUNK_SIZE);
    if (!h->items.arr[chunk].pos
     || !h->items.arr[chunk].keys
     || (h->val_size && !h->items.arr[chunk].vals))
    {
      free(h->items.arr[chunk].pos);
      free(h->items.arr[chunk].keys);
      free(h->items.arr[chunk].vals);
      return false;
    }
    for (int i = 0; i < CHUNK_SIZE; i++)
      h->items.arr[chunk].pos[i] = ~(anoheap_id)0;
    h->items.cap += CHUNK_SIZE;
  }
  return true;
}

anoheap_id
anoheap_add_key(anoheap *h, const void *key, bool enable) {
  return anoheap_add(h, key, NULL, enable);
}

anoheap_id
anoheap_add(anoheap *h, const void *key, const void *val, bool enable) {
  if (!_anoheap_items_ensure_space(h, 1))
    return 0;
  for (anoheap_id i = h->items.lowest; i < h->items.cap / CHUNK_SIZE; i++) {
    anoheap_id *pos = h->items.arr[i].pos;
    if (h->items.arr[i].len == CHUNK_SIZE)
      continue;
    for (anoheap_id j = h->items.arr[i].lowest; j < CHUNK_SIZE; j++) {
      if (pos[j] == ~(anoheap_id)0) {
        const anoheap_id id = i * CHUNK_SIZE + j + 1;
        pos[j] = 0;
        h->val_size ? anoheap_update(h, id, key, val)
                    : anoheap_update_key(h, id, key);
        if (enable && !anoheap_enable(h, id))
          return pos[j] = ~(anoheap_id)0, 0;
        h->items.arr[i].len++;
        h->items.arr[i].lowest = j;
        h->items.len++;
        h->items.lowest = i;
        return id;
      }
    }
  }
  return 0;
}

bool
anoheap_delete(anoheap *h, anoheap_id id) {
  if (!anoheap_exists(h, id))
    return false;
  anoheap_disable(h, id);
  *POS_PTR(id) = ~(anoheap_id)0;

  h->items.arr[(id - 1) / CHUNK_SIZE].len--;
  if (h->items.arr[(id - 1) / CHUNK_SIZE].lowest > (id - 1) % CHUNK_SIZE)
    h->items.arr[(id - 1) / CHUNK_SIZE].lowest = (id - 1) % CHUNK_SIZE;

  h->items.len--;
  if (h->items.lowest > (id - 1) / CHUNK_SIZE)
    h->items.lowest = (id - 1) / CHUNK_SIZE;

  return true;
}

bool
anoheap_get(anoheap *h, anoheap_id id, void *key, void *val) {
  return !anoheap_exists(h, id) ? false :
    memcpy(key, KEY_PTR(id), h->key_size),
    memcpy(val, VAL_PTR(id), h->val_size),
    true;
}

bool
anoheap_get_key(anoheap *h, anoheap_id id, void *key) {
  return !anoheap_exists(h, id) ? false :
    memcpy(key, KEY_PTR(id), h->key_size), true;
}

bool
anoheap_get_val(anoheap *h, anoheap_id id, void *val) {
  return !anoheap_exists(h, id) ? false :
    memcpy(val, VAL_PTR(id), h->val_size), true;
}

void *
anoheap_get_val_direct(anoheap *h, anoheap_id id) {
  return anoheap_exists(h, id) ? VAL_PTR(id) : NULL;
}

bool
anoheap_exists(anoheap *h, anoheap_id id) {
  return 0 == id || id > h->items.cap ? false : ~(anoheap_id)0 != *POS_PTR(id);
}

bool
anoheap_is_enabled(anoheap *h, anoheap_id id) {
  return anoheap_exists(h, id) ? 0 != *POS_PTR(id) : false;
}

static void
_anoheap_bubble_up(anoheap *h, anoheap_id map_pos) {
  anoheap_id item_pos = *MAP_PTR(map_pos);
  const bool is_max_heap = h->options & anoheap_max;
  while (map_pos > 1) {
    anoheap_id parent_map_pos = map_pos >> 1;
    anoheap_id parent_item_pos = *MAP_PTR(parent_map_pos);
    if (is_max_heap) {
     if (0 >= h->cmp(KEY_PTR(item_pos), KEY_PTR(parent_item_pos)))
      break;
    } else {
     if (0 <= h->cmp(KEY_PTR(item_pos), KEY_PTR(parent_item_pos)))
      break;
    }

    *MAP_PTR(map_pos) = parent_item_pos;
    *MAP_PTR(parent_map_pos) = item_pos;
    *POS_PTR(item_pos) = parent_map_pos;
    *POS_PTR(parent_item_pos) = map_pos;
    map_pos = parent_map_pos;
  }
}

static void
_anoheap_bubble_down(anoheap *h, anoheap_id map_pos) {
  anoheap_id item_pos = *MAP_PTR(map_pos);
  const bool is_max_heap = h->options & anoheap_max;
  while (1) {
    anoheap_id lchild_map_pos = map_pos << 1;
    anoheap_id rchild_map_pos = lchild_map_pos + 1;

    if (lchild_map_pos > h->map.len)
      break;
    anoheap_id *lchild_item_pos = MAP_PTR(lchild_map_pos);
    anoheap_id successor_map_pos = lchild_map_pos;
    anoheap_id successor_item_pos = *lchild_item_pos;
    if (is_max_heap) {
      if (rchild_map_pos <= h->map.len) {
        anoheap_id *rchild_item_pos = MAP_PTR(rchild_map_pos);
        if (0 > h->cmp(KEY_PTR(*lchild_item_pos), KEY_PTR(*rchild_item_pos))) {
          successor_map_pos = rchild_map_pos;
          successor_item_pos = *rchild_item_pos;
        }
      }
      if (0 >= h->cmp(KEY_PTR(successor_item_pos), KEY_PTR(item_pos)))
        break;
    } else {
      if (rchild_map_pos <= h->map.len) {
        anoheap_id *rchild_item_pos = MAP_PTR(rchild_map_pos);
        if (0 < h->cmp(KEY_PTR(*lchild_item_pos), KEY_PTR(*rchild_item_pos))) {
          successor_map_pos = rchild_map_pos;
          successor_item_pos = *rchild_item_pos;
        }
      }
      if (0 <= h->cmp(KEY_PTR(successor_item_pos), KEY_PTR(item_pos)))
        break;
    }
    *MAP_PTR(map_pos) = successor_item_pos;
    *MAP_PTR(successor_map_pos) = item_pos;
    *POS_PTR(item_pos) = successor_map_pos;
    *POS_PTR(successor_item_pos) = map_pos;
    map_pos = successor_map_pos;
  }
}

bool
anoheap_enable(anoheap *h, anoheap_id id) {
  if (!anoheap_exists(h, id))
    return false;
  if (anoheap_is_enabled(h, id))
    return true;
  if (!_anoheap_map_ensure_space(h, 1))
    return false;
  anoheap_id map_pos = ++h->map.len;
  *POS_PTR(id) = map_pos;
  *MAP_PTR(map_pos) = id;
  _anoheap_bubble_up(h, map_pos);
  return true;
}

bool
anoheap_disable(anoheap *h, anoheap_id id) {
  if (!anoheap_exists(h, id))
    return false;
  if (!anoheap_is_enabled(h, id))
    return true;
  anoheap_id map_pos = *POS_PTR(id);
  *POS_PTR(id) = 0;
  if (map_pos < h->map.len) {
    anoheap_id last_item_pos = *MAP_PTR(map_pos) = *MAP_PTR(h->map.len);
    *POS_PTR(last_item_pos) = map_pos;
    _anoheap_bubble_up(h, map_pos);
    _anoheap_bubble_down(h, map_pos);
  }
  h->map.len--;
  return true;
}

anoheap_id
anoheap_peek_id(anoheap *h) {
  return h->map.len ? *MAP_PTR(1) : 0;
}

anoheap_id
anoheap_peek_key(anoheap *h, void *key) {
  anoheap_id id = anoheap_peek_id(h);
  if (id) memcpy(key, KEY_PTR(id), h->key_size);
  return id;
}

anoheap_id
anoheap_peek(anoheap *h, void *key, void *val) {
  anoheap_id id = anoheap_peek_key(h, key);
  if (id) memcpy(val, VAL_PTR(id), h->val_size);
  return id;
}

anoheap_id
anoheap_pop_id(anoheap *h, bool delete_item) {
  anoheap_id id = anoheap_peek_id(h);
  if (id) (delete_item ? anoheap_delete : anoheap_disable)(h, id);
  return id;
}

anoheap_id
anoheap_pop_key(anoheap *h, void *key, bool delete_item) {
  anoheap_id id = anoheap_peek_key(h, key);
  if (id) (delete_item ? anoheap_delete : anoheap_disable)(h, id);
  return id;
}

anoheap_id
anoheap_pop(anoheap *h, void *key, void *val, bool delete_item) {
  anoheap_id id = anoheap_peek(h, key, val);
  if (id) (delete_item ? anoheap_delete : anoheap_disable)(h, id);
  return id;
}

static void
_anoheap_update_key(anoheap *h, anoheap_id id, const void *key) {
  void *dest = KEY_PTR(id);
  if (key) memcpy(dest, key, h->key_size);
  else memset(dest, 0, h->key_size);
  if (anoheap_is_enabled(h, id)) {
    anoheap_id mpos = *POS_PTR(id);
    _anoheap_bubble_up(h, mpos);
    _anoheap_bubble_down(h, mpos);
  }
}

static void
_anoheap_update_val(anoheap *h, anoheap_id id, const void *val) {
  void *dest = VAL_PTR(id);
  if (val) memcpy(dest, val, h->val_size);
  else memset(dest, 0, h->val_size);
}

bool
anoheap_update(anoheap *h, anoheap_id id, const void *key, const void *val) {
  if (!anoheap_exists(h, id))
    return false;
  _anoheap_update_key(h, id, key);
  _anoheap_update_val(h, id, val);
  return true;
}

bool
anoheap_update_key(anoheap *h, anoheap_id id, const void *key) {
  if (!anoheap_exists(h, id))
    return false;
  _anoheap_update_key(h, id, key);
  return true;
}

bool
anoheap_update_val(anoheap *h, anoheap_id id, const void *val) {
  if (!anoheap_exists(h, id))
    return false;
  _anoheap_update_val(h, id, val);
  return true;
}
