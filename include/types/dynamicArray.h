/**
 * @file dynamicArray.h
 * @author Itai Lupo
 * @brief a simple dynamic array in c that can use a generic allocator
 * @version 0.1
 * @date 2024-07-28
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include "types/err_t.h"
#include "types/memoryAllocator.h"

#define IS_VALID_DARRAY(arr)                                                                                           \
	(arr != NULL && (arr->maxSize > 0 && arr->elementSize != 0 && arr->allocator != NULL && arr->data != NULL))

#define DARRAY_START(arr) arr->data
#define DARRAY_END(arr) ((char *)arr->data + arr->currentSize * arr->elementSize)
#define DARRAY_NEXT(arr, i) (void *)((char *)i + arr->elementSize)

#ifdef __cplusplus
extern "C"
{
#endif

	/**
	 * @brief the dynamic array header data
	 *
	 * @var darray::maxSize
	 *		the allocated size/elementSize of data
	 * @var darray::elementSize
			the size of one element in data
	 * @var darray::currentSize
			the current element size(used for push&pop)
	 * @var darray::allocator
	 *		the allocator the array should use, will save the struct and data on it
	 * @var darray::data
	 *		the real array
	 *
	 */
	typedef struct
	{
		size_t maxSize;
		size_t elementSize;
		size_t currentSize;

		const memoryAllocator *allocator;

		void *data;

	} darray;

	/**
	 * @brief create a new darray
	 *
	 * @param size the max size of the array
	 * @param element_size the size of an element in the array
	 * @param allocator the allocator to use for the array
	 * @param newArray the result will be here
	 */
	THROWS err_t darrayCreate(size_t size, size_t element_size, const memoryAllocator *const allocator,
							  darray **const newArray);

	/**
	 * @brief free the darray
	 */
	THROWS err_t darrayFree(darray **arr);

	/**
	 * @brief copy the array data to a larger buffer and the free the original .
	 *
	 * @param arr the array to resize.
	 * @param newSize the new max size.
	 */
	THROWS err_t darrayResize(darray *arr, size_t newSize);

	/**
	 * @brief memcpy element i from the array to resBuf.
	 */
	THROWS err_t darrayGet(darray *arr, uint32_t i, void **resBuf, size_t bufSize);

	/**
	 * @brief memcpy resBuf from to element i in arr
	 * @note can change currentSize if i > currentSize then currentSize will be set to i.
	 */
	THROWS err_t darraySet(darray *arr, uint32_t i, void *buf, size_t bufSize);

	/**
	 * @brief inc currentSize by one and memcpy buf to the currentSize element in the arr.
	 */
	THROWS err_t darrayPush(darray *arr, void *buf, size_t bufSize);

	/**
	 * @brief dec currentSize by one and the currentSize element in the arr to buf.
	 */
	THROWS err_t darrayPop(darray *arr, void **resBuf, size_t bufSize);

	/**
	 * @brief allocate to buffers in newAllocator copy the array and it's data up to currentSize to them and the free
	 * the original. also set allocator to newAllocator.
	 */
	THROWS err_t darraySwitchAllocator(darray **arr, const memoryAllocator *newAllocator);

#ifdef __cplusplus
}
#endif
