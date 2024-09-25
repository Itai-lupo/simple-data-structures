/**
 * @file buddyAllocator.h
 * @author Itai Lupo
 * @brief c/cpp implementation of a buddy allocator
 * @note all functions use quite checks unless otherwise is specfied.
 * @version 0.1
 * @date 2024-07-27
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once
#include "types/err_t.h"

// #include <cstddef>
#include <unistd.h>

#include "types/memoryMapInfo.h"

#include "types/dynamicArray.h"

/**
 * @brief get how many free lists will a buddy allocator will need
 * @note use in order to get the wanted size of a buddyAllocator struct
 * @param poolSizeExponent uint8_t the exponent of the wanted max pool size(2^x)
 * @param smallestAllocationSizeExponent uint8_t the exponent of the smallest wanted allocations size(2^ x)
 */
#define GET_NEEDED_FREE_LISTS_COUNT(poolSizeExponent, smallestAllocationSizeExponent)                                  \
	(poolSizeExponent - smallestAllocationSizeExponent)

/**
 * @brief this is all the data the buddyAllocator uses for itself
 * @note to allow better freedom when using the buddy allocator and simplify the allocator itself,
			it is up to the user to allocate and free this struct
 *
 * @var buddyAllocator::memorySource the info of the address range manged by the allocator.
 * @see memoryMapInfo
 *
 * @var buddyAllocator::poolSizeExponent
 *		 the exponent of the wanted max pool size(2^x)
 * @var buddyAllocator::smallestAllocationSizeExponent
 *			the exponent of the smallest wanted allocations size(2^ x)
 * @var buddyAllocator::freeListsCount
 *			must be GET_NEEDED_FREE_LISTS_COUNT(buddyAllocator::poolSizeExponent,
 buddyAllocator::smallestAllocationSizeExponent)
 * @note used to verify there are enough freeLists allocated
 * @var buddyAllocator::freeLists
 *			a list of darray that is used to keep track of the buddy tree free leafs
 */
typedef struct
{

	memoryMapInfo memorySource;

	uint8_t poolSizeExponent;
	uint8_t smallestAllocationSizeExponent;

	uint8_t freeListsCount;
	darray *freeLists[];
} buddyAllocator;

#ifdef __cplusplus
extern "C"
{
#endif
	/**
	 * @brief verify buddyAllocator is empty but valid and put the start state in it
	 *
	 * @param allocator
	 * @return THROWS if any of the valued is invalid
	 */
	THROWS err_t initBuddyAllocator(buddyAllocator *allocator);

	/**
	 * @brief doesn't do much as it is better for the user to just free the underline address range,
	 *			and the buddyAllocator memory is manged by the use anyway.
	 * @note if the buddyAllocator is saved on manged address range freeing the underline range is all the user need to
	 *do
	 */
	THROWS err_t closeBuddyAllocator(buddyAllocator *allocator);

	/**
	 * @brief return address of size in *ptr
	 * @note the real size of the address is rounded up to the closest power of 2 with a minumum of
	 * smallestAllocationSizeExponent
	 * @return THROWS is any of the values is invalid or if there is no more memory
	 */
	THROWS err_t buddyAlloc(buddyAllocator *allocator, void **const ptr, size_t size);

	/**
	 * @brief free the value that is in *ptr and set it to NULL
	 * @return THROWS is any of the values is invalid
	 */
	THROWS err_t buddyFree(buddyAllocator *allocator, void **const ptr);
#ifdef __cplusplus
}
#endif
