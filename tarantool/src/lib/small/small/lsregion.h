#ifndef INCLUDES_TARANTOOL_LSREGION_H
#define INCLUDES_TARANTOOL_LSREGION_H
/*
 * Copyright 2010-2016, Tarantool AUTHORS, please see AUTHORS file.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the
 *    following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY <COPYRIGHT HOLDER> ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * <COPYRIGHT HOLDER> OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdint.h>
#include <assert.h>
#include <stdbool.h>

#include "rlist.h"
#include "quota.h"
#include "slab_cache.h"

#include "lsregion_internal.h"

#   if defined(TARMEMDBG) || defined(TARANTOOL_PICO_MEMORY_DEBUG_ON) || defined(TARARAM) // picodata memory debug

//void   lsregion_create(struct lsregion **lsregion, struct slab_arena *arena);
void * lsregion_aligned_reserve_slow(struct lsregion **lsregion, size_t size, size_t alignment, void **unaligned );
void * lsregion_aligned_reserve(struct lsregion *lsregion, size_t size, size_t alignment, void **unaligned);
void * lsregion_reserve(struct lsregion *lsregion, size_t size);
void * lsregion_alloc(struct lsregion *lsregion, size_t size, int64_t id);
void * lsregion_aligned_alloc(struct lsregion *lsregion, size_t size, size_t alignment, int64_t id);
void   lsregion_gc(struct lsregion *lsregion, int64_t min_id);
void   lsregion_destroy(struct lsregion *lsregion);
size_t lsregion_used(const struct lsregion *lsregion);
size_t lsregion_total(const struct lsregion *lsregion);
struct lsregion * get_lsregion( memory_epoch_queue ** lsregion_value );

#   else  // picodata memory debug

#      ifdef TARMEMDBG_ALLOW_INCLUDE 
           // если включён этот флаг, значит компилируется TaraRam. А внутри 
           // TaraRam не может быть неопределён флаг TARARAM
#          error Unknown error!!!
#      endif
/// @remark если что-то из-за макросов работать не будет, замените на static inline обёртку
#      define lsregion_create lsregion_create_orig
#      define lsregion_aligned_reserve_slow lsregion_aligned_reserve_slow_orig
#      define lsregion_aligned_reserve lsregion_aligned_reserve_orig
#      define lsregion_reserve lsregion_reserve_orig
#      define lsregion_alloc lsregion_alloc_orig
#      define lsregion_aligned lsregion_aligned_alloc
#      define lsregion_gc lsregion_gc_orig
#      define lsregion_destroy lsregion_destroy_orig
#      define lsregion_used lsregion_used_orig
#      define lsregion_total lsregion_total_orig
#      define get_lsregion get_lsregion_orig
#   endif // picodata memory debug

static inline void   lsregion_create_orig(struct lsregion *lsregion, struct slab_arena *arena);
void *               lsregion_aligned_reserve_slow_orig(struct lsregion *lsregion, size_t size, size_t alignment, void **unaligned );
static inline void * lsregion_aligned_reserve_orig(struct lsregion *lsregion, size_t size, size_t alignment, void **unaligned);
static inline void * lsregion_reserve_orig(struct lsregion *lsregion, size_t size);
static inline void * lsregion_alloc_orig(struct lsregion *lsregion, size_t size, int64_t id);
static inline void * lsregion_aligned_alloc_orig(struct lsregion *lsregion, size_t size, size_t alignment, int64_t id);
static inline void   lsregion_gc_orig(struct lsregion *lsregion, int64_t min_id);
static inline void   lsregion_destroy_orig(struct lsregion *lsregion);
static inline size_t lsregion_used_orig(const struct lsregion *lsregion);
static inline size_t lsregion_total_orig(const struct lsregion *lsregion);
static inline struct lsregion * get_lsregion( struct lsregion* lsregion_value ) { return lsregion_value; }

#endif
