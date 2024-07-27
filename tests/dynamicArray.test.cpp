#include "log.h"

#include "err.h"

#include "memoryPool/dummyAllocator.h"
#include "types/dynamicArray.h"

#include <gtest/gtest.h>

TEST(dynamicArray, createAndDeleteArrays)
{
	err_t err = NO_ERRORCODE;
	char arrData[1] = {0};
	char arrData2[100] = {0};
	char arrData3[100][100] = {{0}};
	darray arr = {0, 0, 0, getDummyAllocator(), (void *)arrData};
	darray arr2 = {0, 0, 0, getDummyAllocator(), (void *)arrData2};
	darray arr3 = {0, 0, 0, getDummyAllocator(), (void *)arrData3};
	darray *testArr = &arr;
	darray *testArr2 = &arr2;
	darray *testArr3 = &arr3;

	RETHROW(darrayCreate(1, 1, getDummyAllocator(), &testArr));
	RETHROW(darrayFree(&testArr));

	RETHROW(darrayCreate(1, 100, getDummyAllocator(), &testArr2));
	RETHROW(darrayFree(&testArr2));

	RETHROW(darrayCreate(100, 100, getDummyAllocator(), &testArr3));
	RETHROW(darrayFree(&testArr3));

cleanup:
	REWARN(err);
	ASSERT_TRUE(!IS_ERROR(err));
}

TEST(dynamicArray, getElementsArrays)
{
	err_t err = NO_ERRORCODE;
	uint8_t arrData[1] = {10};
	uint8_t arrData2[100] = {[50] = 1};
	uint8_t arrData3[100][100] = {{1}};
	darray arr = {0, 0, 0, getDummyAllocator(), (void *)arrData};
	darray arr2 = {0, 0, 0, getDummyAllocator(), (void *)arrData2};
	darray arr3 = {0, 0, 0, getDummyAllocator(), (void *)arrData3};
	darray *testArr = &arr;
	darray *testArr2 = &arr2;
	darray *testArr3 = &arr3;

	uint8_t test = 0;
	uint8_t testArrRes[100] = {0};

	RETHROW(darrayCreate(1, 1, getDummyAllocator(), &testArr));
	RETHROW(darrayGet(testArr, 0, (void **)&test, sizeof(uint8_t)));
	CHECK_ERRORCODE(test == 10, test);
	RETHROW(darrayFree(&testArr));

	RETHROW(darrayCreate(100, 1, getDummyAllocator(), &testArr2));
	RETHROW(darrayGet(testArr2, 50, (void **)&test, sizeof(uint8_t)));
	CHECK_ERRORCODE(test == 1, test);
	RETHROW(darrayFree(&testArr2));

	RETHROW(darrayCreate(100, 100, getDummyAllocator(), &testArr3));
	RETHROW(darrayGet(testArr3, 0, (void **)&testArrRes, 100));
	CHECK_ERRORCODE(testArrRes[0] == 1, testArrRes[0]);
	RETHROW(darrayFree(&testArr3));

cleanup:
	REWARN(err);
	ASSERT_TRUE(!IS_ERROR(err));
}

TEST(dynamicArray, setElementsArrays)
{
	err_t err = NO_ERRORCODE;
	uint8_t arrData[1] = {10};
	uint8_t arrData2[100] = {1};

	darray arr = {0, 0, 0, getDummyAllocator(), (void *)arrData};
	darray arr2 = {0, 0, 0, getDummyAllocator(), (void *)arrData2};

	darray *testArr = &arr;
	darray *testArr2 = &arr2;

	uint8_t test = 0;

	RETHROW(darrayCreate(1, 1, getDummyAllocator(), &testArr));

	test = 10;
	RETHROW(darraySet(testArr, 0, (void **)&test, sizeof(uint8_t)));
	test = 0;

	RETHROW(darrayGet(testArr, 0, (void **)&test, sizeof(uint8_t)));
	CHECK_ERRORCODE(test == 10, test);
	RETHROW(darrayFree(&testArr));

	RETHROW(darrayCreate(100, 1, getDummyAllocator(), &testArr2));

	test = 1;
	RETHROW(darraySet(testArr2, 0, (void **)&test, sizeof(uint8_t)));
	test = 0;

	RETHROW(darrayGet(testArr2, 0, (void **)&test, sizeof(uint8_t)));
	CHECK_ERRORCODE(test == 1, test);
	RETHROW(darrayFree(&testArr2));

cleanup:
	REWARN(err);
	ASSERT_TRUE(!IS_ERROR(err));
}
