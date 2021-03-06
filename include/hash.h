/*
Copyright (c) 2003-2014, Troy D. Hanson     http://troydhanson.github.com/uthash/
Copyright (c) 2014, Vsevolod Stakhov <vsevolod@highsecure.ru>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef __hash_h_included
#define __hash_h_included

#include <string.h>   /* memcmp,strlen */
#include <stddef.h>   /* ptrdiff_t */
#include <stdlib.h>   /* exit() */

/* These macros use decltype or the earlier __typeof GNU extension.
   As decltype is only available in newer compilers (VS2010 or gcc 4.3+
   when compiling c++ source) this code uses whatever method is needed
   or, for VS2008 where neither is available, uses casting workarounds. */
#if defined(_MSC_VER)   /* MS compiler */
#if _MSC_VER >= 1600 && defined(__cplusplus)  /* VS2010 or newer in C++ mode */
#define DECLTYPE(x) (decltype(x))
#else                   /* VS2008 or older (or VS2010 in C mode) */
#define NO_DECLTYPE
#define DECLTYPE(x)
#endif
#elif defined(__BORLANDC__) || defined(__LCC__) || defined(__WATCOMC__)
#define NO_DECLTYPE
#define DECLTYPE(x)
#else                   /* GNU, Sun and other compilers */
#define DECLTYPE(x) (__typeof(x))
#endif

#ifndef _HU
# ifdef __GNUC__
#   define _HU(x) _HU_ ## x __attribute__((__unused__))
# else
#   define _HU(x) _HU_ ## x
# endif
#endif

#ifndef _HU_FUNCTION
# ifdef __GNUC__
#   define _HU_FUNCTION(x) __attribute__((__unused__)) x
# else
#   define _HU_FUNCTION(x) x
# endif
#endif

#ifdef NO_DECLTYPE
#define DECLTYPE_ASSIGN(dst,src)                                                 \
do {                                                                             \
  char **_da_dst = (char**)(&(dst));                                             \
  *_da_dst = (char*)(src);                                                       \
} while(0)
#else
#define DECLTYPE_ASSIGN(dst,src)                                                 \
do {                                                                             \
  (dst) = DECLTYPE(dst)(src);                                                    \
} while(0)
#endif

/* a number of the hash function use uint32_t which isn't defined on Pre VS2010 */
#if defined (_WIN32)
#if defined(_MSC_VER) && _MSC_VER >= 1600
#include <stdint.h>
#elif defined(__WATCOMC__)
#include <stdint.h>
#else
typedef unsigned int uint32_t;
typedef unsigned char uint8_t;
#endif
#else
#include <stdint.h>
#endif

#ifndef HASH_RANDOM_SEED
#define HASH_RANDOM_SEED rand
#endif

/* initial number of buckets */
#define HASH_INITIAL_NUM_BUCKETS 32      /* initial number of buckets        */
#define HASH_INITIAL_NUM_BUCKETS_LOG2 5  /* lg2 of initial number of buckets */
#define HASH_BKT_CAPACITY_THRESH 10      /* expand when bucket count reaches */

/* random signature used only to find hash tables in external analysis */
#define HASH_SIGNATURE 0xa0111fe1
#define HASH_BLOOM_SIGNATURE 0xb12220f2

#ifndef HASH_TYPE
#define HASH_TYPE uint32_t
#endif
/*
 * Operations structure, defines all common functions aplicable to a hash table
 */
#ifndef HASH_OPS
#define HASH_OPS(type, field)                                                 \
struct  _hash_ops_##type##_##field {                                           \
  int (*hash_cmp)(const struct type *a, const struct type *b, void *d);        \
  void *cmpd;                                                                  \
  HASH_TYPE (*hash_func)(const struct type *a, void *d);                       \
  void (*hash_init)(void *d);                                                  \
  void *hashd;                                                                 \
  void * (*lock_init)(void *d);                                                \
  void (*lock_read_lock)(void *l, void *d);                                    \
  void (*lock_read_unlock)(void *l, void *d);                                  \
  void (*lock_write_lock)(void *l, void *d);                                   \
  void (*lock_write_unlock)(void *l, void *d);                                 \
  void (*lock_destroy)(void *l, void *d);                                      \
  void *lockd;                                                                 \
  void * (*lockn_init)(void *d);                                                \
  void (*lockn_read_lock)(void *l, void *d);                                    \
  void (*lockn_read_unlock)(void *l, void *d);                                  \
  void (*lockn_write_lock)(void *l, void *d);                                   \
  void (*lockn_write_unlock)(void *l, void *d);                                 \
  void (*lockn_destroy)(void *l, void *d);                                      \
  void *locknd;                                                                 \
  void *(*bloom_init)(void *d);                                                \
  void (*bloom_add)(void *b, HASH_TYPE h);                                     \
  void (*bloom_test)(void *b, HASH_TYPE h);                                    \
  void (*bloom_destroy)(void *b, void *d);                                    \
  void *bloomd;                                                                \
  void *(*alloc)(size_t len, void *d);                                         \
  void (*free)(size_t len, void *p, void *d);                                 \
  void *allocd;                                                                \
}
#endif
/*
 * Hash entry. Overhead: 1 pointer + 1 hash number
 */
#ifndef HASH_ENTRY
#define HASH_ENTRY(type)                                                       \
struct {                                                                       \
  struct type *next;                                                           \
  HASH_TYPE hv;                                                                \
}
#endif

typedef struct _hash_node_s {
  void *first;
  void *lock;
  unsigned entries;
  unsigned expand_mult;
} _hash_node_t;

/*
 * Hash table itself. Overhead is negligible as it is one per hash table
 */
#ifndef HASH_HEAD
#define HASH_HEAD(name, type, field)                                           \
  struct name {                                                                \
   _hash_node_t *buckets;                                                      \
   struct _hash_ops_##type##_##field *ops;                                     \
   unsigned num_buckets, log2_num_buckets;                                     \
   unsigned ideal_chain_maxlen;                                                \
   unsigned nonideal_items;                                                    \
   unsigned ineff_expands, noexpand;                                           \
   unsigned num_items;                                                         \
   unsigned need_expand;                                                       \
   unsigned generation;                                                        \
   uint32_t signature; /* used only to find hash tables in external analysis */\
   uint8_t *bloom_bv;                                                          \
   char bloom_nbits;                                                           \
   void *resize_lock;                                                          \
  }
#endif

/*
 * Prior to using of this macro, one should define ops structure with
 * HASH_GENERATE_*(type, field);
 */
#ifndef HASH_INIT
#define HASH_INIT(head, type, field) do {                                     \
  memset((head), 0, sizeof((*head)));                                         \
  (head)->ops = &_hash_ops_##type##_##field_glob;                             \
} while(0);
#endif

#ifndef HASH_INIT_OPS
#define HASH_INIT_OPS(head, ops) do {                                         \
  memset((head), 0, sizeof((*head)));                                         \
  memcpy(&(head)->ops, ops, sizeof((head)->ops));                             \
} while(0);
#endif

/* Locking methods */
#define HASH_LOCK_READ(head) do {                                             \
   if ((head)->resize_lock) {                                                    \
     (head)->ops->lock_read_lock((head)->resize_lock, (head)->ops->lockd);             \
   }                                                                           \
} while(0)
#define HASH_LOCK_WRITE(head) do {                                            \
   if ((head)->resize_lock) {                                                    \
     (head)->ops->lock_write_lock((head)->resize_lock, (head)->ops->lockd);            \
   }                                                                           \
} while(0)
#define HASH_UNLOCK_READ(head) do {                                           \
   if ((head)->resize_lock) {                                                    \
     (head)->ops->lock_read_unlock((head)->resize_lock, (head)->ops->lockd);           \
   }                                                                           \
} while(0)
#define HASH_UNLOCK_WRITE(head) do {                                          \
   if ((head)->resize_lock) {                                                    \
     (head)->ops->lock_write_unlock((head)->resize_lock, (head)->ops->lockd);          \
   }                                                                           \
} while(0)
#define HASH_LOCK_NODE_READ(head, node) do {                                  \
   if ((node)->lock) {                                                           \
     (head)->ops->lockn_read_lock((node)->lock, (head)->ops->locknd);                    \
   }                                                                           \
} while(0)
#define HASH_LOCK_NODE_WRITE(head, node) do {                                       \
   if ((node)->lock) {                                                           \
     (head)->ops->lockn_write_lock((node)->lock, (head)->ops->locknd);                   \
   }                                                                           \
} while(0)
#define HASH_UNLOCK_NODE_READ(head, node) do {                                      \
   if ((node)->lock) {                                                           \
     (head)->ops->lockn_read_unlock((node)->lock, (head)->ops->locknd);                  \
   }                                                                           \
} while(0)
#define HASH_UNLOCK_NODE_WRITE(head, node) do {                                     \
   if ((node)->lock) {                                                           \
     (head)->ops->lockn_write_unlock((node)->lock, (head)->ops->locknd);                 \
   }                                                                           \
} while(0)

/* Allocating methods */
#ifndef HASH_ALLOC_NODES
#define HASH_ALLOC_NODES(head, nodes, size) do {                              \
  if ((head)->ops->alloc) (nodes) = (head)->ops->alloc(sizeof(*(nodes)) * (size), \
      (head)->ops->allocd);                                                    \
  else (nodes) = malloc(sizeof(*(nodes)) * (size));                           \
  memset(nodes, 0, sizeof(*(nodes)) * (size));                                \
  if ((head)->ops->lockn_init) {                                                 \
    for (unsigned _i = 0; _i < size; _i ++) {                                 \
      (nodes)[_i].lock = (head)->ops->lockn_init((head)->ops->locknd);                \
    }                                                                          \
  }                                                                            \
} while(0)
#endif

#ifndef HASH_FREE_NODES
#define HASH_FREE_NODES(head, nodes, size) do {                              \
  if ((head)->ops->lockn_destroy) {                                              \
    for (unsigned _i = 0; _i < size; _i ++) {                                 \
      (head)->ops->lockn_destroy((nodes)[_i].lock, (head)->ops->locknd);           \
    }                                                                          \
  }                                                                            \
  if ((head)->ops->free) (head)->ops->free(sizeof(*(nodes)) * (size), (nodes), (head)->ops->allocd); \
  else free(nodes);                                                           \
} while(0)
#endif

/* Basic ops */
#ifndef HASH_INSERT
#define HASH_INSERT(head, type, field, elm) do {                               \
  if ((head)->buckets == NULL) HASH_MAKE_TABLE(head);                          \
  HASH_TYPE _hv;                                                               \
  _hv = (head)->ops->hash_func((elm), (head)->ops->hashd);                     \
  (elm)->field.hv = _hv;                                                       \
  HASH_LOCK_READ(head);                                                        \
  _hash_node_t *bkt = HASH_FIND_BKT((head)->buckets, (head)->num_buckets, _hv); \
  HASH_LOCK_NODE_WRITE(head, bkt);                                             \
  (head)->num_items++;                                                         \
  if (bkt->entries + 1 >= ((bkt->expand_mult+1) * HASH_BKT_CAPACITY_THRESH) && \
    (head)->need_expand != 2) {                                                \
    (head)->need_expand = 1;                                                   \
  }                                                                            \
  HASH_UNLOCK_READ(head);                                                      \
  HASH_INSERT_BKT(bkt, type, field, elm);                                      \
  HASH_UNLOCK_NODE_WRITE(head, bkt);                                           \
  if ((head)->need_expand == 1) {                                              \
        HASH_EXPAND_BUCKETS(head, type, field);                                \
  }                                                                            \
} while(0)
#endif

#ifndef HASH_FIND_ELT
#define HASH_FIND_ELT(head, type, field, elm, found) do {                      \
  if ((head)->buckets == NULL) (found) = NULL;                                 \
  else {                                                                       \
	  HASH_TYPE _hv;                                                             \
	  struct type *_telt;                                                        \
	  _hv = (head)->ops->hash_func((elm), (head)->ops->hashd);                   \
	  HASH_LOCK_READ(head);                                                      \
	  _hash_node_t *bkt = HASH_FIND_BKT((head)->buckets, (head)->num_buckets, _hv); \
	  HASH_LOCK_NODE_READ(head, bkt);                                            \
	  _telt = (struct type *)bkt->first;                                         \
	  HASH_UNLOCK_READ(head);                                                    \
	  while(_telt != NULL && (head)->ops->hash_cmp((elm), _telt, (head)->ops->hashd) != 0) \
	    _telt = _telt->field.next;                                               \
	  (found) = _telt;                                                           \
	  HASH_UNLOCK_NODE_READ((head), bkt);                                        \
  }                                                                            \
} while(0)
#endif

#ifndef HASH_DELETE_ELT
#define HASH_DELETE_ELT(head, type, field, elm) do {                          \
  if ((head)->buckets != NULL) {                                               \
    HASH_LOCK_READ(head);                                                      \
    struct type *_telt, *_prev = NULL;                                        \
    _hash_node_t *bkt = HASH_FIND_BKT((head)->buckets, (head)->num_buckets, _hv); \
    HASH_LOCK_NODE_WRITE(head, bkt);                                            \
    _telt = (struct type *)bkt->first;                                        \
    HASH_UNLOCK_READ(head);                                                    \
    while(_telt != NULL && (head)->ops->hash_cmp((elm), _telt, (head)->ops->hashd) != 0) { \
         _prev = _telt;                                                        \
         _telt = _telt->field.next;                                            \
    }                                                                          \
    if (_telt != NULL) {                                                       \
      if (prev != NULL) _prev->field.next = _telt->field.next;                 \
      else bkt->first = (void *)_telt->field.next;                            \
      _telt->field.next = NULL;                                                \
      HASH_LOCK_READ(head);                                                    \
      (head)->num_items--;                                                     \
      HASH_UNLOCK_READ(head);                                                  \
      bkt->entries --;                                                         \
    }                                                                          \
    HASH_UNLOCK_NODE_WRITE((head), bkt);                                       \
  }                                                                            \
} while(0)
#endif

#ifndef HASH_CLEANUP_NODES
#define HASH_CLEANUP_NODES(head, type, field, free_func) do {                 \
  if ((head)->buckets != NULL) {                                               \
      struct type *_telt, *_tmp;                                              \
      _hash_node_t *bkt;                                                       \
      HASH_LOCK_READ(head);                                                    \
      for (unsigned _i = 0; _i < (head)->num_buckets; _i ++) {                \
        bkt = &(head)->buckets[_i];                                            \
        HASH_LOCK_NODE_WRITE(head, bkt);                                       \
        _telt = (struct type *)bkt->first;                                    \
        while(_telt != NULL) {                                                \
          _tmp = _telt;                                                        \
          _telt = _telt->field.next;                                           \
          _tmp->field.next = NULL;                                             \
          if ((free_func) != NULL) _hash_op_##type##_##field##_delete_node((free_func), _tmp); \
        }                                                                      \
        bkt->first = NULL;                                                     \
        HASH_UNLOCK_NODE_WRITE(head, bkt);                                   \
      }                                                                        \
      (head)->num_items = 0;                                                   \
      HASH_UNLOCK_READ(head);                                                  \
    }                                                                          \
} while(0)
#endif

#ifndef HASH_DESTROY
#define HASH_DESTROY(head, type, field, free_func) do {                       \
  HASH_CLEANUP_NODES(head, type, field, free_func);                            \
  HASH_LOCK_WRITE(head);                                                       \
  HASH_FREE_NODES((head), (head)->buckets, (head)->num_buckets);               \
  (head)->buckets = NULL;                                                      \
  (head)->num_buckets = 0;                                                     \
  (head)->log2_num_buckets = 0;                                                \
  (head)->generation = 0;                                                      \
  (head)->num_items = 0;                                                       \
  HASH_UNLOCK_WRITE(head);                                                     \
  if ((head)->ops->lock_destroy) (head)->ops->lock_destroy((head)->resize_lock, (head)->ops->lockd); \
} while(0)
#endif

#ifndef HASH_INSERT_BKT
#define HASH_INSERT_BKT(bkt, type, field, elm) do {                            \
  (elm)->field.next = (struct type *)(bkt)->first;                             \
  (bkt)->first = (void*)(elm);                                                 \
  (bkt)->entries ++;                                                           \
} while(0)
#endif

/* Private methods */
#ifndef HASH_MAKE_TABLE
#define HASH_MAKE_TABLE(head) do {                                             \
  (head)->num_buckets = HASH_INITIAL_NUM_BUCKETS;                              \
  (head)->log2_num_buckets = HASH_INITIAL_NUM_BUCKETS_LOG2;                    \
  HASH_ALLOC_NODES((head), (head)->buckets, (head)->num_buckets);              \
  if ((head)->ops->lock_init) (head)->resize_lock = (head)->ops->lock_init((head)->ops->lockd); \
  if ((head)->ops->hash_init) (head)->ops->hash_init((head)->ops->hashd);      \
  (head)->signature = HASH_SIGNATURE;                                          \
} while(0)
#endif

#define HASH_ROUNDUP32(x)                                                     \
  (--(x), (x)|=(x)>>1, (x)|=(x)>>2, (x)|=(x)>>4, (x)|=(x)>>8, (x)|=(x)>>16, ++(x))

#ifndef HASH_EXPAND_BUCKETS
#define HASH_EXPAND_BUCKETS(head, type, field)                                \
do {                                                                           \
  unsigned _saved_generation = (head)->generation;                            \
  HASH_LOCK_WRITE(head);                                                       \
  if ((head)->generation == _saved_generation) {                               \
      _hash_node_t *_new_nodes, *bkt;                                            \
	  unsigned _new_num = (head)->num_buckets + 1;                              \
	  HASH_ROUNDUP32(_new_num);                                                 \
	  HASH_ALLOC_NODES((head), _new_nodes, _new_num);                            \
	  if (_new_nodes != NULL) {                                                 \
		(head)->ideal_chain_maxlen =                                               \
		    ((head)->num_items >> ((head)->log2_num_buckets+1)) +                  \
		    (((head)->num_items & (((head)->num_buckets*2)-1)) ? 1 : 0);           \
		(head)->nonideal_items = 0;                                                \
	  for (size_t _i = 0; _i < (head)->num_buckets; _i ++) {                    \
		  struct type *_elt = (struct type *)(head)->buckets[_i].first, *_tmp_elt; \
		  HASH_LOCK_NODE_WRITE(head, &(head)->buckets[_i]);                        \
		  while (_elt) {                                                          \
			  _tmp_elt = _elt->field.next;                                           \
		    bkt = HASH_FIND_BKT(_new_nodes, _new_num, _elt->field.hv);            \
		    HASH_INSERT_BKT(bkt, type, field, _elt);                               \
		    if (bkt->entries > (head)->ideal_chain_maxlen) {                       \
		    	(head)->nonideal_items++;                                            \
		    	bkt->expand_mult = bkt->entries / (head)->ideal_chain_maxlen;         \
		    }                                                                      \
		    _elt = _tmp_elt;                                                       \
		  }                                                                        \
		  HASH_UNLOCK_NODE_WRITE(head, &(head)->buckets[_i]);                      \
    }                                                                          \
    HASH_FREE_NODES((head), (head)->buckets, (head)->num_buckets);             \
    (head)->buckets = _new_nodes;                                              \
    (head)->num_buckets = _new_num;                                            \
    (head)->log2_num_buckets++;                                                \
    (head)->ineff_expands = ((head)->nonideal_items > ((head)->num_items >> 1)) ? \
      ((head)->ineff_expands+1) : 0;                                           \
    (head)->generation ++;                                                     \
    }                                                                          \
  }                                                                            \
  if ((head)->ineff_expands > 1) (head)->need_expand = 2;                      \
  else (head)->need_expand = 0;                                                \
  HASH_UNLOCK_WRITE(head);                                                     \
} while(0)
#endif

#ifndef HASH_FIND_BKT
#define HASH_FIND_BKT(nodes, size, hv)                                        \
  (&(nodes)[(hv) & ((size) - 1)])
#endif

#ifndef HASH_ITER
#define HASH_ITER(type, field)                                                \
  struct {                                                                    \
    struct type *e;                                                           \
    _hash_node_t *bucket;                                                     \
  }
#endif

#ifndef HASH_ITER_INITIALIZER
#define HASH_ITER_INITIALIZER                                                 \
		{ NULL, NULL }
#endif

#ifndef HASH_ITERATE
#define HASH_ITERATE(head, type, field, iter, elt)                             \
    for ((iter).bucket = (head)->buckets;                                      \
      (iter).bucket != (head)->buckets + (head)->num_buckets; ++(iter).bucket) \
      if ((iter).bucket->first != NULL)                                        \
        for ((elt) = (iter).e = (struct type *)(iter).bucket->first;           \
            (iter).e != NULL;                                                  \
            (elt) = (iter).e = (iter).e->field.next)
#endif

#ifndef HASH_ITERATE_FUNC
#define HASH_ITERATE_FUNC(head, type, field, func, data) do {                  \
  int _finished = 0;                                                           \
  _hash_node_t *_node = (head)->buckets,                                       \
      *_end = (head)->buckets + (head)->num_buckets;                           \
  HASH_LOCK_READ(head);                                                        \
  for (; !_finished && _node != _end; _node ++) {                              \
    if (_node->first) {                                                        \
      HASH_LOCK_NODE_READ(head, _node);                                        \
      struct type *_cur = (struct type *)_node->first;                         \
      while (_cur != NULL && !_finished) {                                     \
        if (!(func)(_cur, data)) _finished = 1;                                \
        _cur = _cur->field.next;                                               \
      }                                                                        \
      HASH_UNLOCK_NODE_READ(head, _node);                                      \
    }                                                                          \
  }                                                                            \
  HASH_UNLOCK_READ(head);                                                      \
} while(0)
#endif


#ifndef HASH_FILTER_FUNC
#define HASH_FILTER_FUNC(head, type, field, func, free_func, data) do {        \
  _hash_node_t *_node = (head)->buckets,                                       \
      *_end = (head)->buckets + (head)->num_buckets;                           \
  HASH_LOCK_READ(head);                                                        \
  for (; _node != _end; _node ++) {                                            \
    if (_node->first) {                                                        \
      HASH_LOCK_NODE_WRITE(head, _node);                                       \
      struct type *_cur = (struct type *)_node->first, *_tmp = NULL;           \
      while (_cur != NULL) {                                                   \
        if (!(func)(_cur, data)) {                                             \
          if (_cur == _node->first) _node->first = _cur->field.next;           \
          else _tmp->field.next = _cur->field.next;                            \
          _cur = _cur->field.next;                                             \
          (free_func)(_cur, data);                                             \
        }                                                                      \
        else {                                                                 \
          _tmp = _cur;                                                         \
          _cur = _cur->field.next;                                             \
        }                                                                      \
      }                                                                        \
      HASH_UNLOCK_NODE_WRITE(head, _node);                                     \
    }                                                                          \
  }                                                                            \
  HASH_UNLOCK_READ(head);                                                      \
} while(0)
#endif

typedef struct _hash_generic_hash_s {
  void* (*hash_init)(void *d, unsigned seed, void *space, unsigned spacelen);
  void (*hash_update)(void *s, const unsigned char *in, size_t inlen, void *d);
  HASH_TYPE (*hash_final)(void *s, void *d);
  unsigned seed;
  void *d;
} _hash_generic_hash_t;
/*
 * Space required for generic hash functions
 */
#define HASH_SPACE_SIZE 64

/*
 * Generic Murmur3 hash function
 */
#define HASH_INIT_MURMUR(type, field)                                          \
	struct _hash_murmur_state_##type##_##field {                               \
    uint32_t h;                                                                \
    size_t count;                                                              \
  };                                                                           \
  static void* _HU_FUNCTION(_hash_mur_##type##_##field##_init)(void *_HU(d),   \
      unsigned seed, void *space, unsigned _HU(spacelen))                      \
  { \
    struct _hash_murmur_state_##type##_##field *s = (struct _hash_murmur_state_##type##_##field *)space; \
    s->h = seed;                                                               \
    s->count = 0;                                                              \
    return space;                                                              \
  }                                                                            \
  static void _HU_FUNCTION(_hash_mur_##type##_##field##_update)(void *s, const unsigned char *in, size_t inlen, void *_HU(d)) \
  { \
    struct _hash_murmur_state_##type##_##field *st = (struct _hash_murmur_state_##type##_##field *)s; \
    const uint32_t c1 = 0xcc9e2d51; \
    const uint32_t c2 = 0x1b873593; \
    const int nblocks = inlen / 4; \
    const uint32_t *blocks = (const uint32_t *) (in); \
    const uint8_t *tail = (const uint8_t *) (in + (nblocks * 4)); \
    int i; \
    uint32_t k, h = st->h; \
    for (i = 0; i < nblocks; i++) { \
      k = blocks[i]; \
      k *= c1; \
      k = (k << 15) | (k >> (32 - 15)); \
      k *= c2; \
      h ^= k; \
      h = (h << 13) | (h >> (32 - 13)); \
      h = (h * 5) + 0xe6546b64; \
    } \
    k = 0; \
    switch (inlen & 3) { \
    case 3: \
      k ^= tail[2] << 16; \
    case 2: \
      k ^= tail[1] << 8; \
    case 1: \
      k ^= tail[0]; \
      k *= c1; \
      k = (k << 15) | (k >> (32 - 15)); \
      k *= c2; \
      h ^= k; \
    } \
    st->h = h; \
    st->count += inlen; \
  } \
  HASH_TYPE _HU_FUNCTION(_hash_mur_##type##_##field##_final)(void *s, void * _HU(d)) \
  { \
	  struct _hash_murmur_state_##type##_##field *st = (struct _hash_murmur_state_##type##_##field *)s; \
	  uint32_t h; \
	  h = st->h; \
	  h ^= st->count; \
	  h ^= h >> 16; \
	  h *= 0x85ebca6b; \
	  h ^= h >> 13; \
	  h *= 0xc2b2ae35; \
	  h ^= h >> 16; \
	  return h; \
  } \
  static _hash_generic_hash_t _hash_murmur_##type##_##field = { \
	 .hash_init = &_hash_mur_##type##_##field##_init, \
	 .hash_update = &_hash_mur_##type##_##field##_update, \
	 .hash_final = &_hash_mur_##type##_##field##_final, \
	 .d = NULL \
  }

/*
 * Generic Jenkins hash function
 * http://eternallyconfuzzled.com/tuts/algorithms/jsw_tut_hashing.aspx
 */
#define JEN_MIX(a,b,c)                                                        \
{                                                                              \
  a -= b; a -= c; a ^= ( c >> 13 );                                            \
  b -= c; b -= a; b ^= ( a << 8 );                                             \
  c -= a; c -= b; c ^= ( b >> 13 );                                            \
  a -= b; a -= c; a ^= ( c >> 12 );                                            \
  b -= c; b -= a; b ^= ( a << 16 );                                            \
  c -= a; c -= b; c ^= ( b >> 5 );                                             \
  a -= b; a -= c; a ^= ( c >> 3 );                                             \
  b -= c; b -= a; b ^= ( a << 10 );                                            \
  c -= a; c -= b; c ^= ( b >> 15 );                                            \
}
#define HASH_INIT_JENKINS(type, field) \
  struct _hash_jen_##type##_##field##_state { \
    uint32_t h; \
    uint32_t init; \
  }; \
  static void* _HU_FUNCTION(_hash_jen_##type##_##field##_init)(void * _HU(d),  \
    unsigned seed, void *space, unsigned _HU(spacelen))                        \
  { \
    struct _hash_jen_##type##_##field##_state *s = (struct _hash_jen_##type##_##field##_state *)space; \
    s->h = seed; \
    s->init = 0x9e3779b9; \
    return space; \
  } \
  static void _HU_FUNCTION(_hash_jen_##type##_##field##_update)(void *s, const unsigned char *k, size_t inlen, \
      void *_HU(d)) \
  { \
    struct _hash_jen_##type##_##field##_state *st = (struct _hash_jen_##type##_##field##_state *) s; \
    unsigned a, b; \
    unsigned c; \
    unsigned len = inlen; \
    c = st->init; \
    a = b = 0x9e3779b9; \
    while (len >= 12) { \
      a += (k[0] + ((unsigned) k[1] << 8) + ((unsigned) k[2] << 16) \
          + ((unsigned) k[3] << 24)); \
      b += (k[4] + ((unsigned) k[5] << 8) + ((unsigned) k[6] << 16) \
          + ((unsigned) k[7] << 24)); \
      c += (k[8] + ((unsigned) k[9] << 8) + ((unsigned) k[10] << 16) \
          + ((unsigned) k[11] << 24)); \
      JEN_MIX(a, b, c); \
      k += 12; \
      len -= 12; \
    } \
    c += inlen; \
    switch (len) { \
    case 11: \
      c += ((unsigned) k[10] << 24); \
    case 10: \
      c += ((unsigned) k[9] << 16); \
    case 9: \
      c += ((unsigned) k[8] << 8); \
    case 8: \
      b += ((unsigned) k[7] << 24); \
    case 7: \
      b += ((unsigned) k[6] << 16); \
    case 6: \
      b += ((unsigned) k[5] << 8); \
    case 5: \
      b += k[4]; \
    case 4: \
      a += ((unsigned) k[3] << 24); \
    case 3: \
      a += ((unsigned) k[2] << 16); \
    case 2: \
      a += ((unsigned) k[1] << 8); \
    case 1: \
      a += k[0]; \
    } \
    JEN_MIX(a, b, c); \
    st->h = c; \
  } \
  HASH_TYPE _HU_FUNCTION(_hash_jen_##type##_##field##_final)(void *s, void *_HU(d)) \
  { \
    struct _hash_jen_##type##_##field##_state *st = (struct _hash_jen_##type##_##field##_state *)s; \
    return st->h; \
  } \
  static _hash_generic_hash_t _hash_jenkins_##type##_##field = { \
    .hash_init = &_hash_jen_##type##_##field##_init, \
    .hash_update = &_hash_jen_##type##_##field##_update, \
    .hash_final = &_hash_jen_##type##_##field##_final, \
    .d = NULL \
  }

/*
 * Generators part
 */
/*
 * String generator needs NULL terminated string in a key field
 * Filter func is used to filter hashed or compared keys to allow, for instance
 * case insensitive hash.
 */
typedef struct _hash_filter_data_s {
  _hash_generic_hash_t *ht;
  unsigned char (*filter_func)(unsigned char in, void *d);
  void *d;
} _hash_filter_data_t;


#ifndef _HASH_USE_XXHASH
#define HASH_GENERATE_STR(type, field, keyfield)                              \
	HASH_INIT_MURMUR(type, field);                                              \
	HASH_GENERATE_STR_GENERIC(type, field, keyfield, murmur);                           \
	HASH_GENERATE_OPS(type, field, keyfield,                                     \
        &_str_hash_op_##type##_##field##_hash,                                \
        &_str_hash_op_##type##_##field##_cmp,                                 \
        &_hash_string_data_##type##_##field_glob)
#else
#define HASH_GENERATE_STR(type, field, keyfield)                               \
  HASH_INIT_XXHASH(type, field);                                               \
  HASH_GENERATE_STR_GENERIC(type, field, keyfield, xxhash);                    \
  HASH_GENERATE_OPS(type, field, keyfield,                                     \
        &_str_hash_op_##type##_##field##_hash,                                 \
        &_str_hash_op_##type##_##field##_cmp,                                  \
        &_hash_string_data_##type##_##field_glob)
#endif

#define HASH_GENERATE_STR_GENERIC(type, field, keyfield, hash_type)           \
  static HASH_TYPE _HU_FUNCTION(_str_hash_op_##type##_##field##_hash)(const struct type *e, void *d) \
  {                                                                            \
    _hash_filter_data_t *dt = (_hash_filter_data_t *)d;                          \
    unsigned char space[HASH_SPACE_SIZE];                                     \
    void *s;                                                                   \
    const unsigned char *key = (const unsigned char *)e->keyfield;             \
    HASH_TYPE hash, i;                                                         \
    s = dt->ht->hash_init(dt->ht->d, dt->ht->seed, space, sizeof(space));      \
    if (!dt->filter_func) dt->ht->hash_update(s, key, strlen((const char*)key), \
        dt->ht->d);                                                             \
    else {                                                                     \
      for(hash = i = 0; key[i] != 0; ++i) {                                    \
        unsigned char _c = dt->filter_func(key[i], dt->d);                    \
        dt->ht->hash_update(s, &_c, 1, dt->ht->d);                             \
      }                                                                        \
    }                                                                          \
    return dt->ht->hash_final(s, dt->ht->d);                                    \
  }                                                                            \
  static int _HU_FUNCTION(_str_hash_op_##type##_##field##_cmp)(const struct type *e1, const struct type *e2, void *d) \
  {                                                                            \
    _hash_filter_data_t *dt = (_hash_filter_data_t *)d;                         \
    const unsigned char *k1 = (const unsigned char *)e1->keyfield,         \
      *k2 = (const unsigned char *)e2->keyfield;                             \
    if (dt->filter_func == NULL) return strcmp((const char*)k1, (const char*)k2); \
    else {                                                                    \
      while (*k1) {                                                           \
        if (*k2 == 0) return 1;                                               \
        unsigned char t1 = dt->filter_func(*k1, dt->d),                      \
          t2 = dt->filter_func(*k2, dt->d);                                    \
        if (t1 != t2) return t1 - t2;                                         \
        ++k1; ++k2;                                                            \
      }                                                                        \
      if (*k2 != 0) return -1;                                                \
    }                                                                          \
    return 0;                                                                 \
  }                                                                            \
  static _hash_filter_data_t _hash_string_data_##type##_##field_glob = {     \
	  .ht = &_hash_##hash_type##_##type##_##field,                              \
	  .filter_func = NULL,                                                       \
	  .d = NULL                                                                  \
  };                                                                           \


#define HASH_GENERATE_INT(type, field, keyfield)                               \
	HASH_INIT_JENKINS(type, field);                                            \
	HASH_GENERATE_INT_GENERIC(type, field, keyfield, jenkins);                 \
  HASH_GENERATE_OPS(type, field, keyfield,                                      \
          &_int_hash_op_##type##_##field##_hash,                               \
          &_int_hash_op_##type##_##field##_cmp,                                \
          &_hash_int_data_##type##_##field_glob)

#define HASH_GENERATE_INT_GENERIC(type, field, keyfield, hash_type)           \
  static HASH_TYPE _HU_FUNCTION(_int_hash_op_##type##_##field##_hash)(const struct type *e, void *d) \
  {                                                                            \
    _hash_filter_data_t *dt = (_hash_filter_data_t *)d;                        \
    unsigned char space[HASH_SPACE_SIZE];                                      \
    void *s;                                                                   \
    const unsigned char *key = (const unsigned char *)&e->keyfield;            \
    HASH_TYPE hash, i;                                                         \
    s = dt->ht->hash_init(dt->ht->d, dt->ht->seed, space, sizeof(space));      \
    dt->ht->hash_update(s, key, sizeof(int), dt->ht->d);                       \
    return dt->ht->hash_final(s, dt->ht->d);                                   \
  }                                                                            \
  static int _HU_FUNCTION(_int_hash_op_##type##_##field##_cmp)(const struct type *e1, const struct type *e2, void* _HU(d)) \
  {                                                                            \
    int k1 = (int)e1->keyfield, k2 = (int)e2->keyfield;                       \
    return k1 - k2;                                                           \
  }                                                                            \
  static _hash_filter_data_t _hash_int_data_##type##_##field_glob = {         \
    .ht = &_hash_##hash_type##_##type##_##field,                              \
    .filter_func = NULL,                                                       \
    .d = NULL                                                                  \
  }

/*
 * Generic operations generator
 */
#define HASH_GENERATE_OPS(type, field, keyfield, hashf, cmpf, d)               \
		static void _HU_FUNCTION(_hash_op_##type##_##field##_init_hash)(void *ud)  \
		{                                                                          \
		  _hash_filter_data_t *dt = (_hash_filter_data_t *)ud;                     \
		  if (dt) dt->ht->seed = HASH_RANDOM_SEED();                               \
		}                                                                          \
		static HASH_OPS(type, field) _hash_ops_##type##_##field_glob = {           \
		  .hash_func = (hashf),                                                    \
		  .hash_cmp = (cmpf),                                                      \
		  .hash_init = &_hash_op_##type##_##field##_init_hash,                     \
		  .hashd = (d)                                                             \
		};                                                                         \
    static struct type* _HU_FUNCTION(_hash_op_##type##_##field##_find)(void *_head, const void *k) \
    {                                                                          \
      struct type s, *p;                                                       \
      HASH_HEAD(, type, field) *_h;                                             \
      memcpy((&s.keyfield), k, sizeof(s.keyfield));                            \
      DECLTYPE_ASSIGN(_h, _head);                                                \
      HASH_FIND_ELT(_h, type, field, &s, p);                                    \
      return p;                                                                \
    }                                                                          \
    static void _HU_FUNCTION(_hash_op_##type##_##field##_delete_node)(void (*free_func)(struct type *p), struct type *p) { \
       if (free_func != NULL) free_func(p);                                    \
    }

#define HASH_FIND(head, type, field, key)                                     \
  (_hash_op_##type##_##field##_find((head), (void *)(key)))

#ifdef HASH_BLOOM
#define HASH_BLOOM_BITLEN (1ULL << HASH_BLOOM)
#define HASH_BLOOM_BYTELEN (HASH_BLOOM_BITLEN/8) + ((HASH_BLOOM_BITLEN%8) ? 1:0)
#define HASH_BLOOM_MAKE(tbl)                                                     \
do {                                                                             \
  (tbl)->bloom_nbits = HASH_BLOOM;                                               \
  (tbl)->bloom_bv = (uint8_t*)uthash_malloc(HASH_BLOOM_BYTELEN);                 \
  if (!((tbl)->bloom_bv))  { uthash_fatal( "out of memory"); }                   \
  memset((tbl)->bloom_bv, 0, HASH_BLOOM_BYTELEN);                                \
  (tbl)->bloom_sig = HASH_BLOOM_SIGNATURE;                                       \
} while (0)

#define HASH_BLOOM_FREE(tbl)                                                     \
do {                                                                             \
  uthash_free((tbl)->bloom_bv, HASH_BLOOM_BYTELEN);                              \
} while (0)

#define HASH_BLOOM_BITSET(bv,idx) (bv[(idx)/8] |= (1U << ((idx)%8)))
#define HASH_BLOOM_BITTEST(bv,idx) (bv[(idx)/8] & (1U << ((idx)%8)))

#define HASH_BLOOM_ADD(tbl,hashv)                                                \
  HASH_BLOOM_BITSET((tbl)->bloom_bv, (hashv & (uint32_t)((1ULL << (tbl)->bloom_nbits) - 1)))

#define HASH_BLOOM_TEST(tbl,hashv)                                               \
  HASH_BLOOM_BITTEST((tbl)->bloom_bv, (hashv & (uint32_t)((1ULL << (tbl)->bloom_nbits) - 1)))

#else
#define HASH_BLOOM_MAKE(tbl)
#define HASH_BLOOM_FREE(tbl)
#define HASH_BLOOM_ADD(tbl,hashv)
#define HASH_BLOOM_TEST(tbl,hashv) (1)
#define HASH_BLOOM_BYTELEN 0
#endif


#endif /* __hash_h_included */
