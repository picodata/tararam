#ifndef INCLUDES_TARANTOOL_SMALL_SLAB_ARENA_H
#define INCLUDES_TARANTOOL_SMALL_SLAB_ARENA_H
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
#include "lf_lifo.h"
#include <sys/mman.h>
#include <limits.h>

#include "slab_arena_internal.h"

#   if defined(TARMEMDBG) || defined(TARANTOOL_PICO_MEMORY_DEBUG_ON) || defined(TARARAM) // picodata memory debug

//extern int slab_arena_create(struct slab_arena **arena, struct quota *quota, size_t prealloc, uint32_t slab_size, int flags);
extern void slab_arena_destroy(struct slab_arena **arena);
extern void * slab_map(struct slab_arena *arena);
extern void slab_unmap(struct slab_arena *arena, void *ptr);
extern void slab_arena_mprotect(struct slab_arena *arena);
struct slab_arena * get_slab_arena( struct memory_epoch_queue ** arena );
extern void slab_cache_create(struct slab_cache *cache, struct memory_epoch_queue ** arena);
size_t get_slab_arena_used( struct memory_epoch_queue **arena );
struct quota * get_slab_arena_quota(struct memory_epoch_queue **arena);

#   else  // picodata memory debug

#      ifdef TARMEMDBG_ALLOW_INCLUDE 
           // ???????? ?????????????? ???????? ????????, ???????????? ?????????????????????????? TaraRam. ?? ???????????? 
           // TaraRam ???? ?????????? ???????? ?????????????????????? ???????? TARARAM
#          error Unknown error!!!
#      endif
/// @remark ???????? ??????-???? ????-???? ???????????????? ???????????????? ???? ??????????, ???????????????? ???? static inline ??????????????
#      define slab_arena_create    slab_arena_create_orig
#      define slab_arena_destroy   slab_arena_destroy_orig
#      define slab_map             slab_map_orig
#      define slab_unmap           slab_unmap_orig
#      define slab_arena_mprotect  slab_arena_mprotect_orig
#      define slab_cache_create    slab_cache_create_orig
#      define get_slab_arena_quota get_slab_arena_quota_orig
static inline struct slab_arena * get_slab_arena( struct slab_arena * arena ) {return arena; }
static inline size_t get_slab_arena_used(struct slab_arena *arena) {return arena->used;}
#   endif // picodata memory debug

#endif /* INCLUDES_TARANTOOL_SMALL_SLAB_ARENA_H */