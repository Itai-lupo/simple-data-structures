#include "types/buddyAllocator.h"
#include "defaultTrace.h"
#include "err.h"

#include <cmath>
#include <sys/param.h>
#include <unistd.h>

#define IS_POWER_OF_2(x) ((x & (x - 1)) == 0)

THROWS err_t initBuddyAllocator(buddyAllocator *allocator)
{
	err_t err = NO_ERRORCODE;

	QUITE_CHECK(allocator != NULL);
	QUITE_CHECK(allocator->memorySource.startAddr != nullptr);

	QUITE_CHECK(allocator->smallestAllocationSizeExponent > 0);
	QUITE_CHECK(allocator->poolSizeExponent > allocator->smallestAllocationSizeExponent);
	QUITE_CHECK(allocator->freeListsCount ==
				GET_NEEDED_FREE_LISTS_COUNT(allocator->poolSizeExponent, allocator->smallestAllocationSizeExponent));

	for (size_t i = 0; i < allocator->freeListsCount; i++)
	{
		QUITE_CHECK(IS_VALID_DARRAY(allocator->freeLists[i]));
		QUITE_CHECK(allocator->freeLists[i]->currentSize == 0);
		QUITE_CHECK(allocator->freeLists[i]->elementSize == sizeof(void **));
	}
	QUITE_RETHROW(darrayPush(allocator->freeLists[allocator->freeListsCount - 1], &allocator->memorySource.startAddr,
							 sizeof(void *)));

cleanup:
	return err;
}

THROWS err_t closeBuddyAllocator(buddyAllocator *allocator)
{
	err_t err = NO_ERRORCODE;

	QUITE_CHECK(allocator != NULL);
	QUITE_CHECK(allocator->memorySource.startAddr != nullptr);

cleanup:

	return err;
}

THROWS err_t buddyAlloc(buddyAllocator *allocator, void **const ptr, size_t size)
{
	err_t err = NO_ERRORCODE;
	size_t poolSize = 0;
	uint8_t wantedFreeListIndex = 0;
	int i = 0;
	uint8_t *order = nullptr;
	size_t maxNeededPoolSize = 0;
	void *temp = NULL;

	QUITE_CHECK(allocator != NULL);
	QUITE_CHECK(allocator->memorySource.startAddr != nullptr);

	QUITE_CHECK(ptr != NULL);
	QUITE_CHECK(size > 0);
	QUITE_CHECK(size < pow(2, allocator->poolSizeExponent));

	size++;

	QUITE_RETHROW(allocator->memorySource.getSize(&poolSize));

	wantedFreeListIndex = MAX(ceil(log2((double)size)), allocator->smallestAllocationSizeExponent) -
						  allocator->smallestAllocationSizeExponent;

	QUITE_CHECK(allocator->freeListsCount >= wantedFreeListIndex);

	if (allocator->freeLists[wantedFreeListIndex]->currentSize > 0)
	{
		QUITE_RETHROW(darrayPop(allocator->freeLists[wantedFreeListIndex], ptr, sizeof(void *)));
	}
	else
	{
		i = wantedFreeListIndex + 1;
		while ((size_t)i < allocator->freeListsCount && allocator->freeLists[i]->currentSize == 0)
		{
			i++;
		}

		CHECK_NOTRACE_ERRORCODE(i < allocator->freeListsCount, ENOMEM);

		QUITE_RETHROW(darrayPop(allocator->freeLists[i], ptr, sizeof(void *)));

		i--;

		for (; i >= (int)wantedFreeListIndex; i--)
		{
			temp = (void *)((char *)(*ptr) + (size_t)(pow(2, i + allocator->smallestAllocationSizeExponent)));

			QUITE_RETHROW(darrayPush(allocator->freeLists[i], &temp, sizeof(void *)));
		}
	}

	QUITE_CHECK(*ptr != NULL);

	maxNeededPoolSize = (size_t)*ptr - (size_t)allocator->memorySource.startAddr +
						pow(2, wantedFreeListIndex + allocator->smallestAllocationSizeExponent);

	if (maxNeededPoolSize > poolSize)
	{
		QUITE_RETHROW(allocator->memorySource.setSize(maxNeededPoolSize * 2));
	}

	// save the freelist to use when freeing the data to the start of the allocation so it can be freed later
	order = (uint8_t *)*ptr;
	*order = wantedFreeListIndex;
	*ptr = (char *)*ptr + 1;

cleanup:
	return err;
}

THROWS err_t buddyFree(buddyAllocator *allocator, void **const ptr)
{
	err_t err = NO_ERRORCODE;
	size_t buddyIndex = 0;
	uint8_t order = 0;
	size_t size = 0;
	void *buddyAddr = nullptr;
	bool foundBuddy = true;

	QUITE_CHECK(allocator != NULL);
	QUITE_CHECK(allocator->memorySource.startAddr != nullptr);

	QUITE_CHECK(ptr != NULL);
	QUITE_CHECK(allocator->memorySource.startAddr != nullptr);
	QUITE_CHECK(*ptr >= allocator->memorySource.startAddr &&
				*ptr <= (char *)allocator->memorySource.startAddr + (size_t)pow(2, allocator->poolSizeExponent));

	order = *((uint8_t *)*ptr - 1);
	*ptr = (char *)*ptr - 1;

	for (; order <= allocator->freeListsCount && foundBuddy; order++)
	{
		foundBuddy = false;

		size = pow(2, order + allocator->smallestAllocationSizeExponent);

		// we can't just add/remove size to ptr as we don't if ptr is the left or right buddy
		buddyAddr =
			(void *)((char *)*ptr +
					 (1 - ((((char *)*ptr - (char *)allocator->memorySource.startAddr) / size) % 2) * 2) * size);

		buddyIndex = 0;
		for (void *i = DARRAY_START(allocator->freeLists[order]); i < DARRAY_END(allocator->freeLists[order]);
			 DARRAY_NEXT(allocator->freeLists[order], i))
		{
			buddyIndex++;
			if (i == buddyAddr)
			{
				foundBuddy = true;
			}
		}

		if (foundBuddy)
		{
			QUITE_RETHROW(darrayPop(allocator->freeLists[order],
									(void **)((char *)allocator->freeLists[order]->data + buddyIndex), sizeof(void *)));

			*ptr = MIN(buddyAddr, *ptr);

			QUITE_RETHROW(darrayPush(allocator->freeLists[order + 1], ptr, sizeof(void *)));
		}
	}

	*ptr = nullptr;
cleanup:
	return err;
}
