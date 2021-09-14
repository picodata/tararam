#ifndef INCLUDES_TARANTOOL_SMALL_QUOTA_INTERNAL_H
#define INCLUDES_TARANTOOL_SMALL_QUOTA_INTERNAL_H
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

#if defined(__cplusplus)
extern "C" {
#endif /* defined(__cplusplus) */

struct quota;

/** Release used memory */
ssize_t quota_release(struct quota *quota, size_t size);

#define QUOTA_UNIT_SIZE 1024ULL

static const size_t QUOTA_MAX = QUOTA_UNIT_SIZE * UINT32_MAX > SIZE_MAX ?
				SIZE_MAX - QUOTA_UNIT_SIZE + 1 :
				QUOTA_UNIT_SIZE * UINT32_MAX;

/** A basic limit on memory usage */
struct quota {
	/**
	 * High order dword is the total available memory
	 * and the low order dword is the  currently used amount.
	 * Both values are represented in units of size
	 * QUOTA_UNIT_SIZE.
	 */
	uint64_t value;
};

/**
 * Initialize quota with a given memory limit
 */
static inline void
quota_init(struct quota *quota, size_t total)
{
	uint64_t new_total = (total + (QUOTA_UNIT_SIZE - 1)) /
				QUOTA_UNIT_SIZE;
	quota->value = new_total << 32;
}


#if defined(__cplusplus)
} /* extern "C" { */
#endif /* defined(__cplusplus) */

#endif /* INCLUDES_TARANTOOL_SMALL_QUOTA_INTERNAL_H */
