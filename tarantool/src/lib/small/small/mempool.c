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
#include "mempool.h"
#include <stdlib.h>
#include <string.h>
#include <valgrind/valgrind.h>
#include <valgrind/memcheck.h>

#include "slab_cache.h"
//#include "../../say.h"

/** Log levels */
/*
enum say_level {
	S_FATAL,		
	S_SYSERROR,
	S_ERROR,
	S_CRIT,
	S_WARN,
	S_INFO,
	S_VERBOSE,
	S_DEBUG
};

typedef void (*sayfunc_t)(int, const char *, int, const char *,
		    const char *, ...);
CFORMAT(printf, 5, 0) extern sayfunc_t _say;
#define say_file_line(level, file, line, error, format, ...) ({ \
	_say(level, file, line, error, format, ##__VA_ARGS__); })
#define say(level, error, format, ...) ({ \
	say_file_line(level, __FILE__, __LINE__, error, format, ##__VA_ARGS__); })
#define say_info(format, ...) say(S_INFO, NULL, format, ##__VA_ARGS__)
*/
//#define my_say_info printf
#define my_say_info(...)

/* slab fragmentation must reach 1/8 before it's recycled */
enum { MAX_COLD_FRACTION_LB = 3 };

static inline int
mslab_cmp(const struct mslab *lhs, const struct mslab *rhs)
{
	/* pointer arithmetics may overflow int * range. */
	return lhs > rhs ? 1 : (lhs < rhs ? -1 : 0);
}

rb_proto(, mslab_tree_, mslab_tree_t, struct mslab)

rb_gen(, mslab_tree_, mslab_tree_t, struct mslab, next_in_hot, mslab_cmp)

static inline void
mslab_create(struct mslab *slab, struct mempool *pool)
{
	slab->nfree = pool->objcount;
	slab->free_offset = pool->offset;
	slab->free_list = NULL;
	slab->in_hot_slabs = false;
	rlist_create(&slab->next_in_cold);
}

void *
mslab_alloc(struct mempool *pool, struct mslab *slab)
{
	assert(slab->nfree);
	void *result;
	if (slab->free_list) {
		/* Recycle an object from the garbage pool. */
		result = slab->free_list;
		slab->free_list = *(void **)slab->free_list;
	} else {
		/* Use an object from the "untouched" area of the slab. */
		result = (char *)slab + slab->free_offset;
		slab->free_offset += pool->objsize;
	}

	/* If the slab is full, remove it from the rb tree. */
	if (--slab->nfree == 0) {
		if (slab == pool->first_hot_slab) {
			pool->first_hot_slab = mslab_tree_next(&pool->hot_slabs,
								slab);
		}
		mslab_tree_remove(&pool->hot_slabs, slab);
		slab->in_hot_slabs = false;
	}
	return result;
}

void
mslab_free(struct mempool *pool, struct mslab *slab, void *ptr)
{
	/* put object to garbage list */
	*(void **)ptr = slab->free_list;
	slab->free_list = ptr;
	VALGRIND_FREELIKE_BLOCK(ptr, 0);
	VALGRIND_MAKE_MEM_DEFINED(ptr, sizeof(void *));

	slab->nfree++;

	if (slab->in_hot_slabs == false &&
	    slab->nfree >= (pool->objcount >> MAX_COLD_FRACTION_LB)) {
		/**
		 * Add this slab to the rbtree which contains
		 * sufficiently fragmented slabs.
		 */
		rlist_del_entry(slab, next_in_cold);
		mslab_tree_insert(&pool->hot_slabs, slab);
		slab->in_hot_slabs = true;
		/*
		 * Update first_hot_slab pointer if the newly
		 * added tree node is the leftmost.
		 */
		if (pool->first_hot_slab == NULL ||
		    mslab_cmp(pool->first_hot_slab, slab) == 1) {

			pool->first_hot_slab = slab;
		}
	} else if (slab->nfree == 1) {
		rlist_add_entry(&pool->cold_slabs, slab, next_in_cold);
	} else if (slab->nfree == pool->objcount) {
		/** Free the slab. */
		if (slab == pool->first_hot_slab) {
			pool->first_hot_slab =
				mslab_tree_next(&pool->hot_slabs, slab);
		}
		mslab_tree_remove(&pool->hot_slabs, slab);
		slab->in_hot_slabs = false;
		if (pool->spare > slab) {
			slab_list_del(&pool->slabs, &pool->spare->slab,
				      next_in_list);
			slab_put_with_order(pool->cache, &pool->spare->slab);
			pool->spare = slab;
		 } else if (pool->spare) {
			 slab_list_del(&pool->slabs, &slab->slab,
				       next_in_list);
			 slab_put_with_order(pool->cache, &slab->slab);
		 } else {
			 pool->spare = slab;
		 }
	}
}

void
mempool_create_with_order(struct mempool *pool, struct slab_cache *cache,
			  uint32_t objsize, uint8_t order)
{
	assert(order <= cache->order_max);
	lifo_init(&pool->link);
	lifo_init(&pool->delayed);
	pool->cache = cache;
	slab_list_create(&pool->slabs);
	mslab_tree_new(&pool->hot_slabs);
	pool->first_hot_slab = NULL;
	rlist_create(&pool->cold_slabs);
	pool->spare = NULL;
	pool->objsize = objsize;
	pool->slab_order = order;
	/* Total size of slab */
	uint32_t slab_size = slab_order_size(pool->cache, pool->slab_order);
	/* Calculate how many objects will actually fit in a slab. */
	pool->objcount = (slab_size - mslab_sizeof()) / objsize;
	assert(pool->objcount);
	pool->offset = slab_size - pool->objcount * pool->objsize;
	pool->slab_ptr_mask = ~(slab_order_size(cache, order) - 1);
}

void
mempool_destroy(struct mempool *pool)
{
	struct slab *slab, *tmp;
	rlist_foreach_entry_safe(slab, &pool->slabs.slabs,
				 next_in_list, tmp)
		slab_put_with_order(pool->cache, slab);
}


struct vy_lsm_env;
extern struct vy_lsm_env * g_vy_lsm_env;
extern void * g_vy_lsm_env_protected_area;
extern void * g_pgen_addr;
extern void * g_pgenptr_in_env;                
extern size_t g_size_env;                     

void *
mempool_alloc(struct mempool *pool)
{
        //int stack_adr = 0;
        my_say_info("\nmemcheckkk mepool 00"); 
	struct mslab *slab = pool->first_hot_slab;
        my_say_info("\nmemcheckkk mepool 01"); 
	if (slab == NULL) {
        my_say_info("\nmemcheckkk mepool 02"); 
		if (pool->spare) {
                        my_say_info("\nmemcheckkk mepool 03"); 
			slab = pool->spare;                       
                        my_say_info("\nmemcheckkk mepool 04"); 
			pool->spare = NULL;

		} else if ((slab = (struct mslab *)
			    slab_get_with_order(pool->cache,
						pool->slab_order))) {
                        my_say_info("(env_p_gen = %p) pgen_ptr_in_env=%p envsize=%d protected = %p up_protected=%p",  g_pgen_addr, g_pgenptr_in_env, g_size_env, g_vy_lsm_env_protected_area,
                                     g_vy_lsm_env_protected_area+4095 );
                        my_say_info("\nmemcheckkk mepool 05 slab=%p pool=%p stack=%p", slab, pool, (&stack_adr-1) ); 
			mslab_create(slab, pool);
                        my_say_info("\nmemcheckkk mepool 06"); 
			slab_list_add(&pool->slabs, &slab->slab, next_in_list);    
                        my_say_info("\nmemcheckkk mepool 07"); 
		} else if (! rlist_empty(&pool->cold_slabs)) {
                        my_say_info("\nmemcheckkk mepool 08"); 
			slab = rlist_shift_entry(&pool->cold_slabs, struct mslab,
						 next_in_cold);
                        my_say_info("\nmemcheckkk mepool 09"); 
		} else {
			return NULL;
		}
                my_say_info("\nmemcheckkk mepool 10"); 
		assert(slab->in_hot_slabs == false);
                my_say_info("\nmemcheckkk mepool 11"); 
		mslab_tree_insert(&pool->hot_slabs, slab);
                my_say_info("\nmemcheckkk mepool 12"); 
		slab->in_hot_slabs = true;
                my_say_info("\nmemcheckkk mepool 13"); 
		pool->first_hot_slab = slab;
                my_say_info("\nmemcheckkk mepool 14"); 
	}
        my_say_info("\nmemcheckkk mepool 15"); 
	pool->slabs.stats.used += pool->objsize;
        my_say_info("\nmemcheckkk mepool 16"); 
	void *ptr = mslab_alloc(pool, slab);
        my_say_info("\nmemcheckkk mepool 17");
	assert(ptr != NULL);
        my_say_info("\nmemcheckkk mepool 18"); 
	VALGRIND_MALLOCLIKE_BLOCK(ptr, pool->objsize, 0, 0);
        my_say_info("\nmemcheckkk mepool 19"); 
	return ptr;
}

void
mempool_stats(struct mempool *pool, struct mempool_stats *stats)
{
	/* Object size. */
	stats->objsize = pool->objsize;
	/* Number of objects. */
	stats->objcount = mempool_count(pool);
	/* Size of the slab. */
	stats->slabsize = slab_order_size(pool->cache, pool->slab_order);
	/* The number of slabs. */
	stats->slabcount = pool->slabs.stats.total/stats->slabsize;
	/* How much memory is used for slabs. */
	stats->totals.used = pool->slabs.stats.used;
	/*
	 * How much memory is available. Subtract the slab size,
	 * which is allocation overhead and is not available
	 * memory.
	 */
	stats->totals.total = pool->slabs.stats.total -
		mslab_sizeof() * stats->slabcount;
}
