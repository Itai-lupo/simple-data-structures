#include "types/safeQueue.h"

#include "defaultTrace.h"

#include "err.h"
#include "files.h"

#include "types/memoryAllocator.h"

#include <cstddef>
#include <stdatomic.h>
#include <string.h>
#include <sys/eventfd.h>

THROWS err_t initSafeQueue(size_t nodeDataSize, const memoryAllocator *allocator, size_t batchAllocSize,
								safeQueue **const res)
{
	err_t err = NO_ERRORCODE;
	node *dummyNode = NULL;

	QUITE_CHECK(res != NULL);
	QUITE_CHECK(*res == NULL);

	QUITE_CHECK(nodeDataSize > 0);
	QUITE_CHECK(allocator != NULL);
	QUITE_CHECK(batchAllocSize > 0);

	QUITE_RETHROW(allocator->alloc((void **)res, 1, sizeof(safeQueue), 0, allocator->data));

	(*res)->nodeDataSize = nodeDataSize;
	(*res)->batchAllocSize = batchAllocSize;
	(*res)->allocator = allocator;

	QUITE_RETHROW(allocator->alloc((void **)&dummyNode, 1, sizeof(node) + nodeDataSize, 0, allocator->data));
	dummyNode->isValid = false;

	(*res)->head = dummyNode;
	(*res)->tail = dummyNode;


	(*res)->eventFd.fd = eventfd(0, 0);

cleanup:
	return err;
}

THROWS err_t freeSafeQueue(safeQueue **freeList)
{
	err_t err = NO_ERRORCODE;
	const memoryAllocator *allocator = NULL;
	node *currentNode = NULL;
	node *nextNode = NULL;

	QUITE_CHECK(freeList != nullptr);
	QUITE_CHECK(IS_VALID_FREELIST(*freeList));

	allocator = (*freeList)->allocator;
	
	currentNode = (*freeList)->tail;
	while(currentNode != NULL)
	{
		nextNode = currentNode->prev;
		QUITE_RETHROW(allocator->free((void**)&currentNode, allocator->data));	
		currentNode = nextNode;
	}

	QUITE_RETHROW(allocator->free((void**)freeList, allocator->data));

	*freeList = NULL;

cleanup:
	return err;
}


THROWS err_t safeQueuePush(safeQueue *freeList, void *newNodeData)
{
	err_t err = NO_ERRORCODE;
    node *newNode = NULL;
	node *oldHead = NULL;


	uint64_t u = 1;
	ssize_t bytesWritten = 0;

    QUITE_CHECK(newNodeData != nullptr);
	QUITE_CHECK(freeList != nullptr);
	QUITE_CHECK(IS_VALID_FREELIST(freeList));

    newNode = (node*)((size_t)newNodeData - offsetof(node, data));

	do {
		oldHead = freeList->head;
	} while(!atomic_compare_exchange_weak((void *_Atomic *)&freeList->head, (void **)&oldHead, newNode));
	

	newNode->isValid = true;
	oldHead->prev = newNode;


	QUITE_RETHROW(safeWrite(freeList->eventFd, &u, sizeof(uint64_t), &bytesWritten));
	
cleanup:
	return err;
}

THROWS err_t safeQueuePop(safeQueue *freeList, [[maybe_unused]]void *buf, size_t bufSize)
{
	err_t err = NO_ERRORCODE;
	node *popedNode = NULL;

	QUITE_CHECK(freeList != nullptr);
	QUITE_CHECK(IS_VALID_FREELIST(freeList));

	QUITE_CHECK(freeList->nodeDataSize == bufSize);
	QUITE_CHECK(freeList->tail->isValid || freeList->tail->prev != NULL);
		if(!freeList->tail->isValid && freeList->tail->prev != NULL)
		{
			freeList->tail = freeList->tail->prev;
		}

		popedNode = freeList->tail;
		if(popedNode->prev != NULL)
		{
			freeList->tail = freeList->tail->prev;
		}
		else
		{
			freeList->tail->isValid = false;
		}

	
	QUITE_CHECK(popedNode != NULL);
	memcpy(buf, &popedNode->data, bufSize);

cleanup:
	return err;
}

THROWS err_t safeQueueIsEmpty(safeQueue *freeList, bool *isEmpty)
{
	err_t err = NO_ERRORCODE;

	QUITE_CHECK(freeList != nullptr);
	QUITE_CHECK(IS_VALID_FREELIST(freeList));
	QUITE_CHECK(isEmpty != NULL);

	*isEmpty = !freeList->tail->isValid && freeList->tail->prev == NULL;

cleanup:
	return err;
}

THROWS err_t safeQueueGetEmptyNode([[maybe_unused]] safeQueue *freeList, [[maybe_unused]]void **res)
{
	err_t err = NO_ERRORCODE;
	node *temp = NULL;

	QUITE_CHECK(IS_VALID_FREELIST(freeList));
	QUITE_CHECK(res != NULL);
	QUITE_CHECK(*res == NULL);

	QUITE_RETHROW( freeList->allocator->alloc((void **)&temp, 1, sizeof(node) + freeList->nodeDataSize, 0,  freeList->allocator->data));
	*res = temp->data;

cleanup:
	return err;
}
