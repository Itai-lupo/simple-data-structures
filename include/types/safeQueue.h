#pragma once
#include "types/err_t.h"
#include "types/fd_t.h"

#include "types/memoryAllocator.h"

typedef struct node
{
	struct node *prev;
	bool isValid;
	char data[];
} node;

typedef struct
{
	size_t nodeDataSize;
	size_t batchAllocSize;

	const memoryAllocator *allocator;
	fd_t eventFd;

	node *head;
	node *tail;
} safeQueue;

#define IS_VALID_FREELIST(freeList) ((freeList) != NULL && (freeList)->allocator != NULL && (freeList)->head != NULL)

#ifdef __cplusplus
extern "C"
{
#endif

	THROWS err_t initSafeQueue(size_t nodeDataSize, const memoryAllocator *allocator, size_t batchAllocSize, safeQueue **const res);
	THROWS err_t freeSafeQueue(safeQueue **const freeList);

	THROWS err_t safeQueuePush(safeQueue *freeList, void *newNodeData);

	THROWS err_t safeQueuePop(safeQueue *freeList, void *buf, size_t bufSize);

	THROWS err_t safeQueueIsEmpty(safeQueue *freeList, bool *isEmpty);

	THROWS err_t safeQueueGetEmptyNode(safeQueue *freeList, void **res);

#ifdef __cplusplus
}
#endif
