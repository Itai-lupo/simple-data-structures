#include "types/buddyAllocator.h"
#include "log.h"

//#include "defaultTrace.h"
#include "err.h"

#include <cmath>
#include <sys/param.h>
#include <unistd.h>
#include <string.h>

#define IS_POWER_OF_2(x) ((x & (x - 1)) == 0)

THROWS err_t initBuddyAllocator(buddyAllocator *allocator)
{
	err_t err = NO_ERRORCODE;

	QUITE_CHECK(allocator != NULL);
	QUITE_CHECK(allocator->memorySource.startAddr != nullptr);


	QUITE_CHECK(allocator->smallestAllocationSizeExponent > 0);
	QUITE_CHECK(allocator->poolSizeExponent > allocator->smallestAllocationSizeExponent);
	QUITE_CHECK(allocator->freeListSize ==
	GET_BUDDY_MAX_ELEMENT_COUNT(allocator->poolSizeExponent, allocator->smallestAllocationSizeExponent));

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

static constexpr ssize_t findEmptyCellLessThen64Size(buddyAllocator *allocator, size_t cellCount)
{
  uint64_t cellBitMask = (pow(2, cellCount) - 1); 
  
  for(ssize_t i = 0; i < allocator->freeListSize; i += cellCount)
  {
    if((((uint64_t*)allocator->freeList)[i / 64] & (cellBitMask << ((64 - cellCount) - (i % 64)))) == 0)
    {
      ((uint64_t*)allocator->freeList)[i / 64] |= (cellBitMask << ((64 - cellCount) - (i % 64)));
      return i;
    }
  }

  return -1;
}

static constexpr ssize_t findEmptyCell64Size(buddyAllocator *allocator)
{
  for(ssize_t i = 0; i < allocator->freeListSize; i += 64)
  {
    if(((uint64_t*)allocator->freeList)[i / 64] == 0)
    {
      ((uint64_t*)allocator->freeList)[i / 64] = UINT64_MAX;
      return i;
    }
  }

  return -1;
}


static constexpr int64_t findEmptyCellMoreThen64Size(buddyAllocator *allocator, size_t cellCount)
{
  bool AllEmpty = false;

  for(ssize_t i = 0; i < allocator->freeListSize; i += cellCount)
  { 
    AllEmpty = true;
    for(size_t j = 0; j < cellCount / 64; j++)
    {
      if(((uint64_t*)allocator->freeList)[i / 64] != 0)
      {
        AllEmpty = false;
      }
    }

    if(AllEmpty)
    {
      for(size_t j = 0; j < cellCount / 64; j++)
      {
        ((uint64_t*)allocator->freeList)[i / 64 + j] = UINT64_MAX;
      }
      return i;
    }
  }

  return -1;
}


THROWS err_t buddyAlloc(buddyAllocator *allocator, void **const ptr, size_t size)
{
	err_t err = NO_ERRORCODE;
	size_t poolSize = 0;
	uint8_t allocationCellCount = 0;
	uint8_t *order = nullptr;
	size_t maxNeededPoolSize = 0;
  ssize_t cellIndex = -1;

	QUITE_CHECK(allocator != NULL);
	QUITE_CHECK(allocator->memorySource.startAddr != nullptr);

	QUITE_CHECK(ptr != NULL);
	QUITE_CHECK(size > 0);
	QUITE_CHECK(size < pow(2, allocator->poolSizeExponent));

	size++;

	QUITE_RETHROW(allocator->memorySource.getSize(&poolSize));
	allocationCellCount = pow(2, MAX(ceil(log2((double)size) - allocator->smallestAllocationSizeExponent), 0));

	QUITE_CHECK(allocator->freeListSize >= allocationCellCount);
  if(allocationCellCount < 64)
  {
    cellIndex = findEmptyCellLessThen64Size(allocator, allocationCellCount);
  }
  else if(allocationCellCount == 64)
  {
    cellIndex = findEmptyCell64Size(allocator); 
  }
  else
  {
    cellIndex = findEmptyCellMoreThen64Size(allocator, allocationCellCount);
  }

  CHECK_NOTRACE_ERRORCODE(cellIndex != -1, ENOMEM); 
  
  *ptr = (char*)allocator->memorySource.startAddr + cellIndex * (size_t)pow(2, allocator->smallestAllocationSizeExponent);

	QUITE_CHECK(*ptr != NULL);

	maxNeededPoolSize = (cellIndex + allocationCellCount) * pow(2, allocator->smallestAllocationSizeExponent);

	if (maxNeededPoolSize > poolSize)
	{
		QUITE_RETHROW(allocator->memorySource.setSize(maxNeededPoolSize * 2));
	}

	// save the freelist to use when freeing the data to the start of the allocation so it can be freed later
	order = (uint8_t *)*ptr;
	*order = log2(allocationCellCount);
	*ptr = (char *)*ptr + 1;

cleanup:
	return err;
}

THROWS err_t buddyFree(buddyAllocator *allocator, void **const ptr)
{
	err_t err = NO_ERRORCODE;
	uint8_t order = 0;
  size_t cellCount = 0;
  size_t cellIndex = 0;

	QUITE_CHECK(allocator != NULL);
	QUITE_CHECK(allocator->memorySource.startAddr != nullptr);

	QUITE_CHECK(ptr != NULL);
	QUITE_CHECK(allocator->memorySource.startAddr != nullptr);
	QUITE_CHECK(*ptr >= allocator->memorySource.startAddr &&
				*ptr <= (char *)allocator->memorySource.startAddr + (size_t)pow(2, allocator->poolSizeExponent));

  
	order = *((uint8_t *)*ptr - 1);
	*ptr = (char *)*ptr - 1;
  
  QUITE_CHECK(order <= allocator->poolSizeExponent -  allocator->smallestAllocationSizeExponent);

  cellCount =  pow(2, order);
  
  cellIndex =  ( (char*)*ptr - (char*)allocator->memorySource.startAddr) / (size_t)pow(2, allocator->smallestAllocationSizeExponent);

  if(cellCount < 8)
  {
    QUITE_CHECK((allocator->freeList[cellIndex / 8] & ~(((uint8_t)pow(2, cellCount) - 1) << ((8 - cellCount) - (cellIndex % 8)))) > 0);
    allocator->freeList[cellIndex / 8] &= ~(((uint8_t)pow(2, cellCount) - 1) << ((8 - cellCount) - (cellIndex % 8)));
  }
  else
  {
    bzero(&allocator->freeList[cellIndex / 8], cellCount / 8);
  }

	*ptr = nullptr;
cleanup:
	return err;
}
  
THROWS err_t buddyGetCellStartAddrFromAddrInCell(buddyAllocator *allocator, void *ptr, void **res)
{
 	err_t err = NO_ERRORCODE;
  size_t cellIndex = 0;

	QUITE_CHECK(allocator != NULL);
	QUITE_CHECK(allocator->memorySource.startAddr != nullptr);
  
  QUITE_CHECK(res != NULL);
	QUITE_CHECK(ptr != NULL);
	QUITE_CHECK(allocator->memorySource.startAddr != nullptr);
	QUITE_CHECK(ptr >= allocator->memorySource.startAddr);
  QUITE_CHECK(ptr <= (char *)allocator->memorySource.startAddr + (size_t)pow(2, allocator->poolSizeExponent));

  cellIndex =  ((char*)ptr - (char*)allocator->memorySource.startAddr) / (size_t)pow(2, allocator->smallestAllocationSizeExponent);
  *res = (char*)allocator->memorySource.startAddr + cellIndex * (size_t)pow(2, allocator->smallestAllocationSizeExponent);
  *res = (char*)*res + 1;
cleanup:
  return err;
  
}


