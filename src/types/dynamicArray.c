
#define _GNU_SOURCE
#include "types/dynamicArray.h"

#include "defaultTrace.h"
#include "err.h"

#include <string.h>

THROWS err_t darrayCreate(size_t size, size_t elementSize, const memoryAllocator *const allocator,
						  darray **const newArray)
{
	err_t err = NO_ERRORCODE;

	CHECK(newArray != NULL);
	CHECK(size > 0);
	CHECK(elementSize > 0);
	CHECK(allocator != NULL);

	RETHROW(allocator->alloc((void **)newArray, 1, sizeof(darray), ALLOCATOR_CLEAR_MEMORY));
	(*newArray)->maxSize = size;
	(*newArray)->currentSize = 0;
	(*newArray)->elementSize = elementSize;
	(*newArray)->allocator = allocator;

	RETHROW(allocator->alloc(&((*newArray)->data), size, elementSize, ALLOCATOR_CLEAR_MEMORY));

cleanup:
	return err;
}

THROWS err_t darrayFree(darray **arr)
{
	err_t err = NO_ERRORCODE;
	const memoryAllocator *temp = NULL;

	CHECK(arr != NULL);
	CHECK(*arr != NULL);

	temp = (*arr)->allocator;

	RETHROW(temp->free(&(*arr)->data));
	RETHROW(temp->free((void **)arr));

cleanup:
	return err;
}

THROWS err_t darrayResize(darray *arr, size_t newSize)
{
	err_t err = NO_ERRORCODE;
	CHECK(arr != NULL);
	CHECK(newSize >= arr->currentSize);

	RETHROW(arr->allocator->realloc(&arr->data, newSize, arr->elementSize, 0));
	arr->maxSize = newSize;

cleanup:
	return err;
}

THROWS err_t darrayGet(darray *arr, uint32_t i, void **buff, size_t buffSize)
{
	err_t err = NO_ERRORCODE;
	CHECK(arr != NULL);
	CHECK(i >= 0 && i < arr->maxSize);
	CHECK(buff != NULL);
	CHECK(buffSize == arr->elementSize);

	memcpy(buff, (char *)arr->data + arr->elementSize * i, buffSize);

cleanup:
	return err;
}

THROWS err_t darraySet(darray *arr, uint32_t i, void *resBuf, size_t bufSize)
{
	err_t err = NO_ERRORCODE;
	CHECK(arr != NULL);
	CHECK(i >= 0 && i < arr->maxSize);
	CHECK(resBuf != NULL);
	CHECK(bufSize == arr->elementSize);

	if (i > arr->currentSize)
	{
		arr->currentSize = i;
	}

	memcpy((char *)arr->data + arr->elementSize * i, resBuf, bufSize);

cleanup:
	return err;
}

THROWS err_t darrayPush(darray *arr, void *buf, size_t bufSize)
{
	err_t err = NO_ERRORCODE;
	CHECK(arr != NULL);
	CHECK(arr->currentSize < arr->maxSize);
	CHECK(buf != NULL);
	CHECK(bufSize == arr->elementSize);
	//  LOG_INFO("%lu", arr->currentSize);
	memcpy((char *)arr->data + arr->elementSize * arr->currentSize, buf, bufSize);
	arr->currentSize++;

cleanup:
	return err;
}

THROWS err_t darrayPop(darray *arr, void **resBuf, size_t bufSize)
{
	err_t err = NO_ERRORCODE;
	CHECK(arr != NULL);
	CHECK(arr->currentSize > 0);
	CHECK(resBuf != NULL);
	CHECK(bufSize == arr->elementSize);
	//  LOG_INFO("%lu", arr->currentSize);
	arr->currentSize--;
	memcpy(resBuf, (char *)arr->data + arr->elementSize * arr->currentSize, bufSize);

cleanup:
	return err;
}

THROWS err_t darraySwitchAllocator(darray **arr, const memoryAllocator *newAllocator)
{
	err_t err = NO_ERRORCODE;
	const memoryAllocator *temp = NULL;
	darray *arrCopy = NULL;

	CHECK(newAllocator != NULL);
	CHECK(arr != NULL);
	CHECK(*arr != NULL);

	RETHROW(newAllocator->alloc((void **)&arrCopy, 1, sizeof(darray), ALLOCATOR_CLEAR_MEMORY));
	arrCopy->maxSize = (*arr)->maxSize;
	arrCopy->currentSize = (*arr)->currentSize;
	arrCopy->elementSize = (*arr)->elementSize;
	arrCopy->allocator = newAllocator;

	RETHROW(newAllocator->alloc(&(arrCopy->data), (*arr)->maxSize, (*arr)->elementSize, ALLOCATOR_CLEAR_MEMORY));

	memcpy(arrCopy->data, (*arr)->data, (*arr)->currentSize * (*arr)->elementSize);

	temp = (*arr)->allocator;

	RETHROW(temp->free(&(*arr)->data));
	RETHROW(temp->free((void **)arr));

cleanup:
	*arr = arrCopy;

	return err;
}

#ifdef __cplusplus
}
#endif
