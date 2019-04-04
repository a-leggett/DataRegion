#pragma once
#include <stdio.h>
#include "data_region.h"

void validate_data_regions(const DataRegion* regions, size_t count)
{
	for (size_t i = 1; i < count; i++)
		assert(regions[i].first_index > regions[i - 1].last_index);//Not >= because we want adjacent DataRegions to already have been combined
}

void assert_data_regions_equal(const DataRegion* a, const DataRegion* b, size_t count)
{
	for (size_t i = 0; i < count; i++)
		assert(a[i].first_index == b[i].first_index && a[i].last_index == b[i].last_index);
}

void test_add_non_combinable_data_regions()
{
	DataRegion sourceRegions[] = {
		{0,0},
		{2,2},
		{4,5},
		{7,7},
		{10,15}
	};

	const size_t sourceRegionCount = 5;

	DataRegion dstRegions[5];
	size_t dstRegionCount = 0;

	for (size_t i = 0; i < 5; i++)
	{
		assert(add_data_region(dstRegions, sourceRegions[i], &dstRegionCount, 5));
		validate_data_regions(dstRegions, dstRegionCount);
		assert(dstRegionCount == i + 1);
	}

	assert_data_regions_equal(sourceRegions, dstRegions, 5);

	printf("test_add_non_combinable_data_regions passed\n");
}

#define assert_add_data_region(firstIndex, lastIndex, buffer, countRef, capacity) do{DataRegion toAdd = {firstIndex, lastIndex}; assert(add_data_region(buffer, toAdd, countRef, capacity)); } while(false)

#define assert_remove_data_region(firstIndex, lastIndex, buffer, countRef, capacity) do{DataRegion toRemove = {firstIndex, lastIndex}; assert(remove_data_region(buffer, toRemove, countRef, capacity)); } while(false)

#define assert_data_regions_equal_const(expectedSet, actualSet, expectedCount, actualCount) do{ assert(expectedCount == actualCount); DataRegion exp[] = {expectedSet}; assert_data_regions_equal(exp, actualSet, actualCount); } while(false)

#define DR(firstIndex, lastIndex) {firstIndex, lastIndex},

void test_add_some_combinable_data_regions()
{
	DataRegion buffer[6];
	size_t count = 0;

	assert_add_data_region(0, 0, buffer, &count, 5);
	assert_add_data_region(1, 1, buffer, &count, 5);//Should be combined with left region at (0,0)
	assert_data_regions_equal_const(DR(0, 1), buffer, 1, count);

	assert_add_data_region(5, 5, buffer, &count, 5);
	assert_add_data_region(4, 4, buffer, &count, 5);//Should be combined with right region at (5,5)
	assert_data_regions_equal_const(DR(0, 1) DR(4,5), buffer, 2, count);

	assert_add_data_region(6, 6, buffer, &count, 5);//Should be combined with left region at (4,5)
	assert_data_regions_equal_const(DR(0, 1) DR(4, 6), buffer, 2, count);

	assert_add_data_region(13, 17, buffer, &count, 5);
	assert_add_data_region(9, 12, buffer, &count, 5);//Should be combined with right region at (13,17)
	assert_data_regions_equal_const(DR(0, 1) DR(4, 6) DR(9, 17), buffer, 3, count);

	assert_add_data_region(9, 17, buffer, &count, 5);//Should have no effect since the exact region already exists
	assert_data_regions_equal_const(DR(0, 1) DR(4, 6) DR(9, 17), buffer, 3, count);

	assert_add_data_region(9, 9, buffer, &count, 5);//Should have no effect since the region was already covered
	assert_data_regions_equal_const(DR(0, 1) DR(4, 6) DR(9, 17), buffer, 3, count);

	assert_add_data_region(9, 10, buffer, &count, 5);//Should have no effect since the region was already covered
	assert_data_regions_equal_const(DR(0, 1) DR(4, 6) DR(9, 17), buffer, 3, count);

	assert_add_data_region(9, 13, buffer, &count, 5);//Should have no effect since the region was already covered
	assert_data_regions_equal_const(DR(0, 1) DR(4, 6) DR(9, 17), buffer, 3, count);

	assert_add_data_region(10, 10, buffer, &count, 5);//Should have no effect since the region was already covered
	assert_data_regions_equal_const(DR(0, 1) DR(4, 6) DR(9, 17), buffer, 3, count);

	assert_add_data_region(10, 11, buffer, &count, 5);//Should have no effect since the region was already covered
	assert_data_regions_equal_const(DR(0, 1) DR(4, 6) DR(9, 17), buffer, 3, count);

	assert_add_data_region(11, 11, buffer, &count, 5);//Should have no effect since the region was already covered
	assert_data_regions_equal_const(DR(0, 1) DR(4, 6) DR(9, 17), buffer, 3, count);

	assert_add_data_region(11, 12, buffer, &count, 5);//Should have no effect since the region was already covered
	assert_data_regions_equal_const(DR(0, 1) DR(4, 6) DR(9, 17), buffer, 3, count);

	assert_add_data_region(11, 17, buffer, &count, 5);//Should have no effect since the region was already covered
	assert_data_regions_equal_const(DR(0, 1) DR(4, 6) DR(9, 17), buffer, 3, count);

	assert_add_data_region(100, 150, buffer, &count, 5);
	assert_data_regions_equal_const(DR(0, 1) DR(4, 6) DR(9, 17) DR(100,150), buffer, 4, count);

	assert_add_data_region(99, 151, buffer, &count, 5);//Should be combined with (100,150) since it contains all of this region 
	assert_data_regions_equal_const(DR(0, 1) DR(4, 6) DR(9, 17) DR(99, 151), buffer, 4, count);

	assert_add_data_region(95, 155, buffer, &count, 5);//Should be combined with (99,151) since it contains all of this region 
	assert_data_regions_equal_const(DR(0, 1) DR(4, 6) DR(9, 17) DR(95, 155), buffer, 4, count);

	assert_add_data_region(90, 95, buffer, &count, 5);//Should be combined with (95, 155) due to intersection at (95,95)
	assert_data_regions_equal_const(DR(0, 1) DR(4, 6) DR(9, 17) DR(90, 155), buffer, 4, count);

	assert_add_data_region(85, 100, buffer, &count, 5);//Should be combined with (90, 155) due to intersection at (90,100)
	assert_data_regions_equal_const(DR(0, 1) DR(4, 6) DR(9, 17) DR(85, 155), buffer, 4, count);

	assert_add_data_region(100, 200, buffer, &count, 5);//Should be combined with (85,155) due to intersection at (100,155)
	assert_data_regions_equal_const(DR(0, 1) DR(4, 6) DR(9, 17) DR(85, 200), buffer, 4, count);

	assert_add_data_region(200, 300, buffer, &count, 5);//Should be combined with (85,200) due to intersection at (200,200)
	assert_data_regions_equal_const(DR(0, 1) DR(4, 6) DR(9, 17) DR(85, 300), buffer, 4, count);

	assert_add_data_region(0, 6, buffer, &count, 5);//Should be combined with (0,1) AND (4,6)
	assert_data_regions_equal_const(DR(0, 6) DR(9, 17) DR(85, 300), buffer, 3, count);

	assert_add_data_region(7, 8, buffer, &count, 5);//Should be combined with (0,6) AND (9,17)
	assert_data_regions_equal_const(DR(0, 17) DR(85, 300), buffer, 2, count);

	assert_add_data_region(18, 84, buffer, &count, 5);//Should be combined with (0,17) AND (85,300)
	assert_data_regions_equal_const(DR(0, 300), buffer, 1, count);

	assert_add_data_region(500, 600, buffer, &count, 5);
	assert_add_data_region(900, 1200, buffer, &count, 5);
	assert_add_data_region(1500, 2000, buffer, &count, 5);
	assert_add_data_region(2300, 2500, buffer, &count, 5);

	assert_data_regions_equal_const(DR(0, 300) DR(500, 600) DR(900, 1200) DR(1500, 2000) DR(2300, 2500), buffer, 5, count);

	assert_add_data_region(250, 2400, buffer, &count, 6);//Should combine with all
	assert_data_regions_equal_const(DR(0, 2500), buffer, 1, count);

	printf("test_add_some_combinable_data_regions passed\n");
}

size_t create_data_regions_from_mask(const char* mask, size_t maskSize, DataRegion* buffer, size_t bufferCapacity)
{
	DataRegion currentRegion;
	bool inRegion = false;
	size_t regionCount = 0;
	for (size_t i = 0; i < maskSize; i++)
	{
		if (mask[i] != 0)
		{
			if (!inRegion)
			{
				currentRegion.first_index = i;
				inRegion = true;
			}

			currentRegion.last_index = i;
		}
		else
		{
			if (inRegion)
			{
				//Add the current region
				assert(regionCount < bufferCapacity);
				buffer[regionCount++] = currentRegion;
				inRegion = false;
			}
		}
	}

	if (inRegion)
	{
		//Add the current region
		assert(regionCount < bufferCapacity);
		buffer[regionCount++] = currentRegion;
		inRegion = false;
	}

	return regionCount;
}

void create_mask_from_data_regions(const DataRegion* regions, size_t regionCount, char* mask, size_t maskSize)
{
	for (size_t i = 0; i < maskSize; i++)
		mask[i] = 0;

	for (size_t i = 0; i < regionCount; i++)
	{
		for (size_t j = regions[i].first_index; j <= regions[i].last_index; j++)
		{
			assert(j < maskSize);
			mask[j] = 0xFF;
		}
	}
}

void test_create_data_regions_from_mask()
{
	DataRegion regions[1024];
	size_t count = 0;

	char mask1[] = { 1,1,1,1,1,1,1,1 };
	count = create_data_regions_from_mask(mask1, 8, regions, 1024);
	assert_data_regions_equal_const(DR(0, 7), regions, 1, count);

	char mask2[] = { 0,0,0,0,0,0,0,0 };
	count = create_data_regions_from_mask(mask2, 8, regions, 1024);
	assert_data_regions_equal_const(0, regions, 0, count);

	char mask3[] = { 1,0,0,0,0,0,0,1 };
	count = create_data_regions_from_mask(mask3, 8, regions, 1024);
	assert_data_regions_equal_const(DR(0,0) DR(7,7), regions, 2, count);

	char mask4[] = { 1,1,0,0,0,0,1,1 };
	count = create_data_regions_from_mask(mask4, 8, regions, 1024);
	assert_data_regions_equal_const(DR(0,1) DR(6,7), regions, 2, count);

	char mask5[] = { 0,0,0,1,0,0,0,0 };
	count = create_data_regions_from_mask(mask5, 8, regions, 1024);
	assert_data_regions_equal_const(DR(3,3), regions, 1, count);

	char mask6[] = { 0,0,0,1,1,0,0,0 };
	count = create_data_regions_from_mask(mask6, 8, regions, 1024);
	assert_data_regions_equal_const(DR(3,4), regions, 1, count);

	char mask7[] = { 0,1,1,0,1,1,0,0 };
	count = create_data_regions_from_mask(mask7, 8, regions, 1024);
	assert_data_regions_equal_const(DR(1,2) DR(4,5), regions, 2, count);

	printf("test_create_data_regions_from_mask passed\n");
}

void test_add_brute_force()
{
	int width = 50;
	for (int backgroundOffset = -width; backgroundOffset < width + 1/*+1 to test case with no background (push it all to the side)*/; backgroundOffset++)
	{
		for (int count = 0; count < width; count++)
		{
			for (int startIndex = 0; startIndex < width; startIndex++)
			{
				if (startIndex + count >= width)
					break;

				//Initialize some background DataRegions
				char mask[100];
				DataRegion regions[1024] = { {0,5},  {11,11}, {13,13}, {15,16}, {20, 22}, {26, 29}, {33, 40} };
				size_t bgRegionCount = 7;
				size_t regionCount = 0;
				for (size_t i = 0; i < bgRegionCount; i++)
				{
					regions[i].first_index += backgroundOffset;
					regions[i].last_index += backgroundOffset;

					if (regions[i].last_index < 0)
						continue;//Out of bounds, skip it
					if ((int)regions[i].first_index >= width)
						continue;//Out of bounds, skip it

					if (regions[i].first_index < 0)
						regions[i].first_index = 0;
					if ((int)regions[i].last_index > width - 1)
						regions[i].last_index = width - 1;
				}

				create_mask_from_data_regions(regions, regionCount, mask, width);

				//Add the 'moving'  DataRegion to the background
				DataRegion toAdd = { startIndex, startIndex + count };
				assert(add_data_region(regions, toAdd, &regionCount, 1024));

				//Manually write the mask on top of the background
				for (size_t m = toAdd.first_index; m <= toAdd.last_index; m++)
					mask[m] = 0xFF;

				//Create a DataRegion set from the mask
				DataRegion fromMask[1024];
				size_t fromMaskCount = create_data_regions_from_mask(mask, width, fromMask, 1024);

				assert(fromMaskCount == regionCount);
				assert_data_regions_equal(fromMask, regions, fromMaskCount);
			}
		}
	}

	printf("test_add_brute_force passed\n");
}

void test_remove_data_region()
{
	DataRegion buffer[6];
	size_t count = 0;

	assert_add_data_region(0, 1000, buffer, &count, 5);
	assert_remove_data_region(0, 1000, buffer, &count, 5);//Should remove the whole thing
	assert_data_regions_equal_const(DR(0, 0), buffer, 0, count);

	assert_remove_data_region(0, 1000, buffer, &count, 5);//Should have no effect
	assert_data_regions_equal_const(DR(0, 0), buffer, 0, count);

	assert_add_data_region(0, 1000, buffer, &count, 5);
	assert_remove_data_region(0, 0, buffer, &count, 5);//Should remove intersecting region at (0,0)
	assert_data_regions_equal_const(DR(1, 1000), buffer, 1, count);

	assert_remove_data_region(1, 1, buffer, &count, 5);//Should remove intersecting region at (1,1)
	assert_data_regions_equal_const(DR(2, 1000), buffer, 1, count);

	assert_remove_data_region(1000, 1000, buffer, &count, 5);//Should remove intersecting region at (1000,1000)
	assert_data_regions_equal_const(DR(2, 999), buffer, 1, count);

	assert_remove_data_region(0, 5, buffer, &count, 5);//Should remove intersecting region at (2,5)
	assert_data_regions_equal_const(DR(6, 999), buffer, 1, count);

	assert_remove_data_region(950, 1500, buffer, &count, 5);//Should remove intersecting region at (950,999)
	assert_data_regions_equal_const(DR(6,949), buffer, 1, count);

	assert_remove_data_region(12, 12, buffer, &count, 5);//Should remove intersecting region at (12,12)
	assert_data_regions_equal_const(DR(6, 11) DR(13, 949), buffer, 2, count);

	assert_remove_data_region(100, 120, buffer, &count, 5);//Should remove intersecting region at (100,120)
	assert_data_regions_equal_const(DR(6, 11) DR(13, 99) DR(121, 949), buffer, 3, count);

	assert_remove_data_region(200, 300, buffer, &count, 5);//Should remove intersecting region at (200,300)
	assert_data_regions_equal_const(DR(6, 11) DR(13, 99) DR(121, 199) DR(301, 949), buffer, 4, count);

	assert_remove_data_region(95, 250, buffer, &count, 5);//Should remove intersecting region at (95,99) AND (121,199)
	assert_data_regions_equal_const(DR(6, 11) DR(13, 94) DR(301, 949), buffer, 3, count);

	assert_remove_data_region(8, 948, buffer, &count, 5);//Should remove intersecting region at (8,11) AND (13, 94) AND (301,948)
	assert_data_regions_equal_const(DR(6, 7) DR(949, 949), buffer, 2, count);

	assert_remove_data_region(8, 948, buffer, &count, 5);//Should not change anything
	assert_data_regions_equal_const(DR(6, 7) DR(949, 949), buffer, 2, count);

	assert_remove_data_region(0, 1000000, buffer, &count, 5);//Should remove everything
	assert_data_regions_equal_const(DR(0,0), buffer, 0, count);

	printf("test_remove_data_region passed\n");
}

void test_remove_brute_force()
{
	int width = 50;
	for (int backgroundOffset = -width; backgroundOffset < width + 1/*+1 to test case with no background (push it all to the side)*/; backgroundOffset++)
	{
		for (int count = 0; count < width; count++)
		{
			for (int startIndex = 0; startIndex < width; startIndex++)
			{
				if (startIndex + count >= width)
					break;

				//Initialize some background DataRegions
				char mask[100];
				DataRegion regions[1024] = { {0,5},  {11,11}, {13,13}, {15,16}, {20, 22}, {26, 29}, {33, 40} };
				size_t bgRegionCount = 7;
				size_t regionCount = 0;
				for (size_t i = 0; i < bgRegionCount; i++)
				{
					regions[i].first_index += backgroundOffset;
					regions[i].last_index += backgroundOffset;

					if (regions[i].last_index < 0)
						continue;//Out of bounds, skip it
					if ((int)regions[i].first_index >= width)
						continue;//Out of bounds, skip it

					if (regions[i].first_index < 0)
						regions[i].first_index = 0;
					if ((int)regions[i].last_index > width - 1)
						regions[i].last_index = width - 1;
				}

				create_mask_from_data_regions(regions, regionCount, mask, width);

				//Remove the 'moving'  DataRegion to the background
				DataRegion toRemove = { startIndex, startIndex + count };
				assert(remove_data_region(regions, toRemove, &regionCount, 1024));

				//Manually erase the mask from the background
				for (size_t m = toRemove.first_index; m <= toRemove.last_index; m++)
					mask[m] = 0x00;

				//Create a DataRegion set from the mask
				DataRegion fromMask[1024];
				size_t fromMaskCount = create_data_regions_from_mask(mask, width, fromMask, 1024);

				assert(fromMaskCount == regionCount);
				assert_data_regions_equal(fromMask, regions, fromMaskCount);
			}
		}
	}

	printf("test_remove_brute_force passed\n");
}

void test_remove_fails_when_insufficient_memory()
{
	DataRegion buffer[3];
	size_t count = 0;

	assert_add_data_region(0, 10, buffer, &count, 3);
	assert_add_data_region(20, 80, buffer, &count, 3);
	assert_add_data_region(90, 100, buffer, &count, 3);

	//Try to remove the middle of a DataRegion, which causes it to split into 2
	DataRegion toRemove = { 30,70 };
	assert(remove_data_region(buffer, toRemove, &count, 3) == false);

	//Make sure the buffer remains unchanged
	assert_data_regions_equal_const(DR(0, 10) DR(20,80) DR(90,100), buffer, 3, count);

	printf("test_remove_fails_when_insufficient_memory passed\n");
}

void test_remove_when_capacity_is_full()
{
	DataRegion buffer[3];
	size_t count = 0;

	assert_add_data_region(0, 10, buffer, &count, 3);
	assert_add_data_region(20, 80, buffer, &count, 3);
	assert_add_data_region(90, 100, buffer, &count, 3);

	//Remove an exact DataRegion
	{
		DataRegion toRemove = { 20,80 };
		assert(remove_data_region(buffer, toRemove, &count, 3) == true);
		assert_data_regions_equal_const(DR(0, 10) DR(90, 100), buffer, 2, count);
	}

	//Reset
	count = 0;
	assert_add_data_region(0, 10, buffer, &count, 3);
	assert_add_data_region(20, 80, buffer, &count, 3);
	assert_add_data_region(90, 100, buffer, &count, 3);

	//Remove a DataRegion that completely contains a DataRegion in the set
	{
		DataRegion toRemove = { 19,81 };
		assert(remove_data_region(buffer, toRemove, &count, 3) == true);
		assert_data_regions_equal_const(DR(0, 10) DR(90, 100), buffer, 2, count);
	}

	//Reset
	count = 0;
	assert_add_data_region(0, 10, buffer, &count, 3);
	assert_add_data_region(20, 80, buffer, &count, 3);
	assert_add_data_region(90, 100, buffer, &count, 3);

	//Remove a DataRegion that will leave the left portion of an existing DataRegion
	{
		DataRegion toRemove = { 30,80 };
		assert(remove_data_region(buffer, toRemove, &count, 3) == true);
		assert_data_regions_equal_const(DR(0, 10) DR(20, 29) DR(90, 100), buffer, 3, count);
	}

	//Reset
	count = 0;
	assert_add_data_region(0, 10, buffer, &count, 3);
	assert_add_data_region(20, 80, buffer, &count, 3);
	assert_add_data_region(90, 100, buffer, &count, 3);

	//Remove a DataRegion that will leave the left portion of an existing DataRegion
	{
		DataRegion toRemove = { 30,81 };
		assert(remove_data_region(buffer, toRemove, &count, 3) == true);
		assert_data_regions_equal_const(DR(0, 10) DR(20, 29) DR(90, 100), buffer, 3, count);
	}

	//Reset
	count = 0;
	assert_add_data_region(0, 10, buffer, &count, 3);
	assert_add_data_region(20, 80, buffer, &count, 3);
	assert_add_data_region(90, 100, buffer, &count, 3);

	//Remove a DataRegion that will leave the right portion of an existing DataRegion
	{
		DataRegion toRemove = { 20,70 };
		assert(remove_data_region(buffer, toRemove, &count, 3) == true);
		assert_data_regions_equal_const(DR(0, 10) DR(71, 80) DR(90, 100), buffer, 3, count);
	}

	//Reset
	count = 0;
	assert_add_data_region(0, 10, buffer, &count, 3);
	assert_add_data_region(20, 80, buffer, &count, 3);
	assert_add_data_region(90, 100, buffer, &count, 3);

	//Remove a DataRegion that will leave the right portion of an existing DataRegion
	{
		DataRegion toRemove = { 19,70 };
		assert(remove_data_region(buffer, toRemove, &count, 3) == true);
		assert_data_regions_equal_const(DR(0, 10) DR(71, 80) DR(90, 100), buffer, 3, count);
	}

	printf("test_remove_when_capacity_is_full passed\n");
}

void test_add_fails_when_capacity_is_full()
{
	DataRegion buffer[3];
	size_t count = 0;

	//Fill the buffer to full capacity
	assert_add_data_region(0, 10, buffer, &count, 3);
	assert_add_data_region(20, 80, buffer, &count, 3);
	assert_add_data_region(90, 100, buffer, &count, 3);

	//Try to add (but fail due to full capacity)
	DataRegion toAdd = {200, 300};
	assert(add_data_region(buffer, toAdd, &count, 3) == false);

	//Ensure the buffer remains unchanged
	assert_data_regions_equal_const(DR(0, 10) DR(20, 80) DR(90, 100), buffer, 3, count);

	printf("test_add_fails_when_capacity_is_full passed\n");
}

#define assert_get_bounded_data_regions(firstIndex, lastIndex, src, srcCount, dst, dstCapacity, expectedSet, expectedCount) do{ bool dstTooSmall = false; DataRegion bound = {firstIndex, lastIndex}; size_t gotCount = get_bounded_data_regions(dst, dstCapacity, src, srcCount, bound, &dstTooSmall); assert(count_bounded_data_regions(src, srcCount, bound) == gotCount); assert(dstTooSmall == false); assert_data_regions_equal_const(expectedSet, dst, expectedCount, gotCount); } while(false)

void test_get_bounded_data_regions()
{
	DataRegion src[3];
	size_t srcCount = 0;
	assert_add_data_region(100, 100, src, &srcCount, 3);
	assert_add_data_region(200, 300, src, &srcCount, 3);
	assert_add_data_region(400, 450, src, &srcCount, 3);

	DataRegion dst[3];
	assert_get_bounded_data_regions(0, 500, src, srcCount, dst, 3, DR(100, 100) DR(200, 300) DR(400, 450), 3);
	assert_get_bounded_data_regions(100, 100, src, srcCount, dst, 3, DR(100, 100), 1);
	assert_get_bounded_data_regions(99, 101, src, srcCount, dst, 3, DR(100, 100), 1);
	assert_get_bounded_data_regions(100, 200, src, srcCount, dst, 3, DR(100, 100) DR(200,200), 2);
	assert_get_bounded_data_regions(100, 202, src, srcCount, dst, 3, DR(100, 100) DR(200, 202), 2);
	assert_get_bounded_data_regions(99, 200, src, srcCount, dst, 3, DR(100, 100) DR(200, 200), 2);
	assert_get_bounded_data_regions(98, 202, src, srcCount, dst, 3, DR(100, 100) DR(200, 202), 2);
	assert_get_bounded_data_regions(200, 300, src, srcCount, dst, 3, DR(200, 300), 1);
	assert_get_bounded_data_regions(201, 299, src, srcCount, dst, 3, DR(201, 299), 1);
	assert_get_bounded_data_regions(210, 250, src, srcCount, dst, 3, DR(210, 250), 1);
	assert_get_bounded_data_regions(350, 375, src, srcCount, dst, 3, 0, 0);
	assert_get_bounded_data_regions(350, 410, src, srcCount, dst, 3, DR(400, 410), 1);
	assert_get_bounded_data_regions(350, 1500, src, srcCount, dst, 3, DR(400, 450), 1);

	printf("test_get_bounded_data_regions passed\n");
}

#define assert_get_missing_data_regions(firstIndex, lastIndex, src, srcCount, dst, dstCapacity, expectedSet, expectedCount) do{ bool dstTooSmall = false; DataRegion bound = {firstIndex, lastIndex}; size_t gotCount = get_missing_data_regions(dst, dstCapacity, src, srcCount, bound, &dstTooSmall); assert(dstTooSmall == false); assert_data_regions_equal_const(expectedSet, dst, expectedCount, gotCount); }while(false)

void test_get_mising_data_regions()
{
	DataRegion src[3];
	size_t srcCount = 0;

	DataRegion dst[3];

	assert_get_missing_data_regions(0, 0, src, srcCount, dst, 3, DR(0, 0), 1);
	assert_get_missing_data_regions(1, 1, src, srcCount, dst, 3, DR(1, 1), 1);
	assert_get_missing_data_regions(0, 20, src, srcCount, dst, 3, DR(0, 20), 1);

	assert_add_data_region(0, 20, src, &srcCount, 3);
	assert_get_missing_data_regions(0, 0, src, srcCount, dst, 3, 0, 0);
	assert_get_missing_data_regions(1, 1, src, srcCount, dst, 3, 0, 0);
	assert_get_missing_data_regions(0, 20, src, srcCount, dst, 3, 0, 0);

	assert_get_missing_data_regions(0, 21, src, srcCount, dst, 3, DR(21, 21), 1);
	assert_get_missing_data_regions(20, 21, src, srcCount, dst, 3, DR(21, 21), 1);

	assert_remove_data_region(0, 5, src, &srcCount, 3);

	assert_get_missing_data_regions(0, 21, src, srcCount, dst, 3, DR(0, 5) DR(21, 21), 2);
	assert_get_missing_data_regions(1, 19, src, srcCount, dst, 3, DR(1, 5), 1);
	assert_get_missing_data_regions(1, 29, src, srcCount, dst, 3, DR(1, 5) DR(21,29), 2);

	assert_remove_data_region(10, 10, src, &srcCount, 3);

	assert_get_missing_data_regions(1, 29, src, srcCount, dst, 3, DR(1, 5) DR(10, 10) DR(21, 29), 3);
	assert_get_missing_data_regions(0, 1, src, srcCount, dst, 3, DR(0,1), 1);
	assert_get_missing_data_regions(0, 29, src, srcCount, dst, 3, DR(0, 5) DR(10, 10) DR(21, 29), 3);
	assert_get_missing_data_regions(0, 15, src, srcCount, dst, 3, DR(0, 5) DR(10, 10), 2);
	assert_get_missing_data_regions(1, 10, src, srcCount, dst, 3, DR(1, 5) DR(10,10), 2);
	assert_get_missing_data_regions(11, 29, src, srcCount, dst, 3, DR(21, 29), 1);
	assert_get_missing_data_regions(10, 10, src, srcCount, dst, 3, DR(10, 10), 1);
	assert_get_missing_data_regions(1000, 1000, src, srcCount, dst, 3, DR(1000,1000), 1);

	printf("test_get_mising_data_regions passed\n");
}

void test_get_missing_data_regions_when_buffer_is_too_small()
{
	DataRegion src[3];
	size_t srcCount = 0;
	assert_add_data_region(10, 20, src, &srcCount, 3);
	assert_add_data_region(30, 40, src, &srcCount, 3);
	assert_add_data_region(50, 60, src, &srcCount, 3);

	DataRegion dst[2];
	DataRegion bound = { 0, 70 };
	bool tooSmall = false;
	assert(get_missing_data_regions(dst, 2, src, srcCount, bound, &tooSmall) == 2);
	assert(tooSmall == true);

	printf("test_get_missing_data_regions_when_buffer_is_too_small passed\n");
}

void test_get_bounded_data_regions_when_buffer_is_too_small()
{
	DataRegion src[3];
	size_t srcCount = 0;
	assert_add_data_region(100, 150, src, &srcCount, 3);
	assert_add_data_region(200, 300, src, &srcCount, 3);
	assert_add_data_region(400, 450, src, &srcCount, 3);

	DataRegion dst[1];
	DataRegion bound = {  140, 200 };
	bool tooSmall = false;
	assert(get_bounded_data_regions(dst, 1, src, srcCount, bound, &tooSmall) == 1);
	assert(tooSmall == true);

	printf("test_get_bounded_data_regions_when_buffer_is_too_small passed\n");
}

void test_data_regions()
{
	test_add_some_combinable_data_regions();
	test_add_non_combinable_data_regions();
	test_create_data_regions_from_mask();
	test_add_brute_force();
	test_remove_data_region();
	test_remove_brute_force();
	test_remove_fails_when_insufficient_memory();
	test_remove_when_capacity_is_full();
	test_add_fails_when_capacity_is_full();
	test_get_bounded_data_regions();
	test_get_bounded_data_regions_when_buffer_is_too_small();
	test_get_mising_data_regions();
	test_get_missing_data_regions_when_buffer_is_too_small();

	printf("passed all DataRegion tests\n");
}