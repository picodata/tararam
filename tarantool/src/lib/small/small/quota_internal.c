
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

#include "quota.h"

/** Release used memory */
static inline ssize_t
quota_release(struct quota *quota, size_t size)
{
	assert(size < QUOTA_MAX);
	uint32_t size_in_units = (size + (QUOTA_UNIT_SIZE - 1))
				  / QUOTA_UNIT_SIZE;
	assert(size_in_units);
	while (1) {
		uint64_t value = quota->value;
		uint32_t total_in_units = value >> 32;
		uint32_t used_in_units = value & UINT32_MAX;

		assert(size_in_units <= used_in_units);
		uint32_t new_used_in_units = used_in_units - size_in_units;

		uint64_t new_value =
			((uint64_t) total_in_units << 32) | new_used_in_units;

		if (pm_atomic_compare_exchange_strong(&quota->value, &value, new_value))
			break;
	}
	return size_in_units * QUOTA_UNIT_SIZE;
}
