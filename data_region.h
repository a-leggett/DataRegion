#pragma once
#include <stdbool.h>
#include <assert.h>

///<summary>Stores a start and end index of a data region.</summary>
typedef struct DataRegion
{
	///<summary>The index of the first element in the data region.</summary>
	size_t first_index;

	///<summary>The index of the last element in the data region.</summary>
	size_t last_index;
} DataRegion;

///<summary>Checks whether a <see cref="DataRegion"/> completely contains another.</summary>
///<param name="outer">The 'outer' <see cref="DataRegion"/>.</param>
///<param name="inner">The 'inner' <see cref="DataRegion"/>.</param>
///<returns>True if <paramref name="outer"/> contains every index within <paramref name="inner"/>,
///otherwise false.</returns>
bool does_data_region_contain_other_data_region(DataRegion outer, DataRegion inner)
{
	return inner.first_index >= outer.first_index && inner.last_index <= outer.last_index;
}

///<summary>Checks whether two <see cref="DataRegion"/>s intersect.</summary>
///<param name="a">The first <see cref="DataRegion"/>.</param>
///<param name="b">The second <see cref="DataRegion"/>.</param>
///<returns>True if at least one index of <paramref name="a"/> is also
///contained by <paramref name="b"/>, otherwise false.</returns>
bool do_data_regions_intersect(DataRegion a, DataRegion b)
{
	return a.last_index >= b.first_index && b.last_index >= a.first_index;
}

///<summary>Checks whether two <see cref="DataRegion"/>s are adjacent.</summary>
///<param name="a">The first <see cref="DataRegion"/>.</param>
///<param name="b">The second <see cref="DataRegion"/>.</param>
///<returns>True if <paramref name="a"/> is adjacent to
///<paramref name="b"/>.</returns>
///<remarks>Two <see cref="DataRegion"/>s are considered adjacent only if
///there is no gap between them.</remarks>
bool are_data_regions_adjacent(DataRegion a, DataRegion b)
{
	return b.last_index == a.first_index - 1//'b' is left-adjacent to 'a'
		|| b.first_index == a.last_index + 1;//'b' is right-adjacent to 'a'
}

///<summary>Checks whether two <see cref="DataRegion"/>s can be combined.</summary>
///<param name="a">The first <see cref="DataRegion"/>.</param>
///<param name="b">The second <see cref="DataRegion"/>.</param>
///<returns>True if <paramref name="a"/> and <paramref name="b"/> can be 
///combined, otherwise false.</returns>
///<remarks>Two <see cref="DataRegion"/>s can only be combined if
///they are adjacent or intersecting (see <see cref="do_data_regions_intersect(DataRegion,DataRegion)"/>
///and <see cref="are_data_regions_adjacent(DataRegion,DataRegion)"/>).</remarks>
///<seealso cref="combine_data_regions"/>
///<seealso cref="are_data_regions_adjacent(DataRegion,DataRegion)"/>
///<seealso cref="do_data_regions_intersect(DataRegion,DataRegion)"/>
bool can_combine_data_regions(DataRegion a, DataRegion b)
{
	return are_data_regions_adjacent(a, b) || do_data_regions_intersect(a, b);
}

///<summary>Combines two <see cref="DataRegion"/>s into one.</summary>
///<param name="a">The first <see cref="DataRegion"/>.</param>
///<param name="b">The second <see cref="DataRegion"/>.</param>
///<returns>A <see cref="DataRegion"/> that combines <paramref name="a"/> and
///<paramref name="b"/>.</returns>
///<remarks>The caller is responsible for ensuring that <paramref name="a"/>
///and <paramref name="b"/> can be combined. See <see cref="can_combine_data_regions(DataRegion,DataRegion)"/>.</remarks>
DataRegion combine_data_regions(DataRegion a, DataRegion b)
{
	assert(can_combine_data_regions(a, b));
	DataRegion ret;

	ret.first_index = a.first_index;
	if (b.first_index < ret.first_index)//Take the min index
		ret.first_index = b.first_index;

	ret.last_index = a.last_index;
	if (b.last_index > ret.last_index)//Take the max index
		ret.last_index = b.last_index;

	return ret;
}

void internal_remove_data_region_at(DataRegion* regions, size_t* regionCount, size_t index)
{
	//Move all values 'up' after the remove index
	for (size_t i = index; i < (*regionCount) - 1; i++)
		regions[i] = regions[i + 1];

	(*regionCount)--;
}

void internal_insert_data_region_at(DataRegion* regions, size_t* regionCount, DataRegion toInsert, size_t index, size_t capacity)
{
	//Ensure there is indeed enough capacity
	assert(*regionCount < capacity);

	//Move all values 'down' after the insert index
	for (size_t i = *regionCount; i > index; i--)
		regions[i] = regions[i - 1];

	//Insert the DataRegion to the specified index
	regions[index] = toInsert;
	(*regionCount)++;
}

///<summary>Adds a <see cref="DataRegion"/> to an array of <see cref="DataRegion"/>s.</summary>
///<param name="regions">Array of <see cref="DataRegion"/>s, sorted in ascending order. Must not
///contain any overlapping <see cref="DataRegion"/>s.</param>
///<param name="count">Pointer to a size_t that specifies the number of <see cref="DataRegion"/>s
///in <paramref name="regions"/>. The value must be assigned before input, and it will be re-assigned
///for output.</param>
///<param name="capacity">The maximum number of <see cref="DataRegion"/>s that can fit in the
///<paramref name="regions"/> argument.</param>
///<returns>True if <paramref name="toAdd"/> was successfully added to the <paramref name="regions"/>.
///False indicates insufficient capacity.</returns>
///<remarks>This function will find the appropriate position at which to insert <paramref name="toAdd"/>
///into <paramref name="regions"/>. It will also combine any <see cref="DataRegion"/>s that intersect with or
///are adjacent to <paramref name="toAdd"/> (while keeping them sorted in ascending order). As such, it is 
///possible for <paramref name="count"/> to be decreased after the insertion. However, this function still
///requires that the input <paramref name="count"/> value be one less than the <paramref name="capacity"/> (otherwise
///the <see cref="DataRegion"/> will not be added and false will be returned).</remarks>
bool add_data_region(DataRegion* regions, DataRegion toAdd, size_t* count, size_t capacity)
{
	if (*count < capacity)
	{
		size_t insertPos = 0;

		//Find where to insert 'toAdd', and also remove any 'combinable' DataRegions
		for (size_t i = 0; i < *count; i++)
		{
			DataRegion current = regions[i];

			if (can_combine_data_regions(current, toAdd))
			{
				//Remove this DataRegion since it will be combined with 'toAdd'
				internal_remove_data_region_at(regions, count, i);
				toAdd = combine_data_regions(current, toAdd);
				i--;//Since we removed a DataRegion
			}
			else
			{
				if (current.last_index < toAdd.first_index)
					insertPos = i + 1;

				if (current.first_index > toAdd.last_index)
					break;//Done finding the insert position and removing combinable DataRegions
			}
		}

		//Insert 'toAdd'
		internal_insert_data_region_at(regions, count, toAdd, insertPos, capacity);
		return true;
	}
	else
	{
		//Capacity is full, cannot add
		return false;
	}
}

///<summary>Removes a <see cref="DataRegion"/> from an array of <see cref="DataRegion"/>s.</summary>
///<param name="regions">Array of <see cref="DataRegion"/>s, sorted in ascending order. Must not
///contain any overlapping <see cref="DataRegion"/>s.</param>
///<param name="toRemove">The <see cref="DataRegion"/> to remove.</param>
///<param name="count">Pointer to a size_t that specifies the number of <see cref="DataRegion"/>s
///in <paramref name="regions"/>. The value must be assigned before input, and it will be re-assigned
///for output.</param>
///<param name="capacity">The maximum number of <see cref="DataRegion"/>s that can fit in the
///<paramref name="regions"/> argument.</param>
///<returns>True if <see cref="toRemove"/> was successfully removed from the <paramref name="regions"/>,
///otherwise false.</returns>
///<remarks>This function will scan through all <see cref="DataRegion"/>s within the <paramref name="regions"/>
///argument to find any that intersect <paramref name="toRemove"/>. The intersecting region of each
///<see cref="DataRegion"/> will be removed. Note that it is possible for this function to result in
///the <paramref name="count"/> being increased. For example, this may happen when a <see cref="DataRegion"/>
///is split into two pieces. If there is insufficient capacity, then nothing will be removed and
///false will be returned.</remarks>
bool remove_data_region(DataRegion* regions, DataRegion toRemove, size_t* count, size_t capacity)
{
	if (*count == capacity)
	{
		/* Oh no, we may not have enough memory to complete the remove operation!
		  Consider the following example:

		  Regions Before Remove:       (0, 100) [Count = 1, Capacity = 1]
		  Remove DataRegion Argument:  (25, 50)
		  Regions After Remove :       (0, 24), (51, 100) [Count = 2, Capacity = 1] <<<< Problem: We exceeded the capacity!

		  We have to detect whether the above problem will happen in order to prevent it. */
		size_t removeCount = 0, insertCount = 0;
		for (size_t i = 0; i < *count; i++)
		{
			if (do_data_regions_intersect(regions[i], toRemove))
			{
				removeCount++;

				if (regions[i].first_index < toRemove.first_index)
				{
					//The left portion of regions[i] will remain (so we will add it to 'regions')
					insertCount++;
				}

				if (regions[i].last_index > toRemove.last_index)
				{
					//The right portion of regions[i] will remain (so we will add it to 'regions')
					insertCount++;
				}

				//See above, it is possible for removeCount to increment by one, 
				//but insertCount could theoretically increment by two!
			}
			else
			{
				if (regions[i].first_index > toRemove.last_index)
					break;//Done checking for intersections
			}
		}

		if (insertCount > removeCount)
		{
			//We cannot remove, we need more capacity due to the split DataRegions
			return false;
		}
	}

	//Remove all DataRegions within 'toRemove'
	for (size_t i = 0; i < *count; i++)
	{
		DataRegion currentRegion = regions[i];
		if (do_data_regions_intersect(currentRegion, toRemove))
		{
			internal_remove_data_region_at(regions, count, i);//Remove 'currentRegion'
			
			if (currentRegion.first_index < toRemove.first_index)
			{
				//The left portion of 'currentRegion' will remain (so we will add it to 'regions')
				DataRegion leftPortion = { currentRegion.first_index, toRemove.first_index - 1 };
				internal_insert_data_region_at(regions, count, leftPortion, i++, capacity);
			}

			if (currentRegion.last_index > toRemove.last_index)
			{
				//The right portion of 'currentRegion' will remain (so we will add it to 'regions')
				DataRegion rightPortion = { toRemove.last_index + 1, currentRegion.last_index };
				internal_insert_data_region_at(regions, count, rightPortion, i++, capacity);
			}

			i--;//Since we removed 'currentRegion' (must do this AFTER potentially keeping left/right portions above)
		}
		else
		{
			if (regions[i].first_index > toRemove.last_index)
				break;//Done checking for intersections
		}
	}

	return true;
}

///<summary>Gets all <see cref="DataRegion"/>s within an array, bounded by a specific <see cref="DataRegion"/>.</summary>
///<param name="dst">The destination <see cref="DataRegion"/> buffer. May be NULL, in which case this function will only
///count the number of bounded <see cref="DataRegion"/>s.</param>
///<param name="dstCapacity">The maximum number of <see cref="DataRegion"/>s that can be stored in <paramref name="dst"/>.</param>
///<param name="src">The source <see cref="DataRegion"/>s, in ascending order. Must not have overlapping <see cref="DataRegion"/>s.</param>
///<param name="srcCount">The number of <see cref="DataRegion"/>s in <paramref name="src"/>.</param>
///<param name="boundaryRegion">The boundary <see cref="DataRegion"/>.</param>
///<param name="dstTooSmall">Pointer to a boolean that will be assigned to true if <paramref name="dstCapacity"/> was too small, otherwise false.
///May be NULL, in which case its value will not be assigned.</param>
///<returns>The number of bounded <see cref="DataRegion"/>s. This number is limited if the <paramref name="dstCapacity"/> was too small.</returns>
///<remarks>This function will find all <see cref="DataRegion"/>s in <paramref name="src"/> that intersect with <paramref name="boundaryRegion"/>, and
///assign them (in ascending order) to <paramref name="dst"/> (unless <paramref name="dst"/> is NULL). If <paramref name="dst"/> is NULL, then this function
///will only count the number of bounded <see cref="DataRegion"/>s in <paramref name="src"/>.</remarks>
size_t get_bounded_data_regions(DataRegion* dst, size_t dstCapacity, const DataRegion* src, size_t srcCount, DataRegion boundaryRegion, bool* dstTooSmall)
{
	if(dstTooSmall != NULL)
		*dstTooSmall = false;

	size_t count = 0;
	for (size_t i = 0; i < srcCount; i++)
	{
		DataRegion toYield;
		bool doYieldCurrent = false;
		if (does_data_region_contain_other_data_region(boundaryRegion, src[i]))
		{
			//src[i] is completely contained by 'boundaryRegion'
			toYield = src[i];
			doYieldCurrent = true;
		}
		else if(do_data_regions_intersect(boundaryRegion, src[i]))
		{
			//src[i] is partially contained by 'boundaryRegion'
			size_t firstIndex = src[i].first_index;
			size_t lastIndex = src[i].last_index;

			if (firstIndex < boundaryRegion.first_index)
				firstIndex = boundaryRegion.first_index;
			if (lastIndex > boundaryRegion.last_index)
				lastIndex = boundaryRegion.last_index;

			toYield.first_index = firstIndex;
			toYield.last_index = lastIndex;
			doYieldCurrent = true;
		}

		if (doYieldCurrent)
		{
			if (dst != NULL)
			{
				if (count < dstCapacity)
				{
					dst[count++] = toYield;
				}
				else
				{
					if (dstTooSmall != NULL)
						*dstTooSmall = true;

					break;
				}
			}
			else
			{
				//When 'dst' is NULL, it indicates that we are only counting the DataRegions
				count++;
			}
		}

		if (src[i].first_index > boundaryRegion.last_index)
			break;//Beyond the boundary region, no need to continue iterating
	}

	return count;
}

///<summary>Gets the number of <see cref="DataRegion"/>s in an array that intersect with a specific boundary <see cref="DataRegion"/>.</summary>
///<param name="src">The source <see cref="DataRegion"/>s, in ascending order. Must not have overlapping <see cref="DataRegion"/>s.</param>
///<param name="srcCount">The number of <see cref="DataRegion"/>s in <paramref name="src"/>.</param>
///<param name="boundaryRegion">The boundary <see cref="DataRegion"/>.</param>
///<returns>The number of <see cref="DataRegion"/>s in <paramref name="src"/> that intersect with <paramref name="boundaryRegion"/>.</returns>
size_t count_bounded_data_regions(const DataRegion* src, size_t srcCount, DataRegion boundaryRegion)
{
	return get_bounded_data_regions(NULL/*NULL indicates that we only want to count the DataRegions*/, 0, src, srcCount, boundaryRegion, NULL);
}

///<summary>Gets all 'missing' <see cref="DataRegion"/>s within an array.</summary>
///<param name="dst">The destination buffer.</param>
///<param name="dstCapacity">The maximum number of <see cref="DataRegion"/>s that can be stored in <paramref name="dst"/>.</param>
///<param name="src">The source <see cref="DataRegion"/>s.</param>
///<param name="srcCount">The number of <see cref="DataRegion"/>s in <paramref name="src"/>.</param>
///<param name="boundaryRegion">The boundary <see cref="DataRegion"/>.</param>
///<param name="dstTooSmall">Pointer to a boolean that will be assigned to true if <paramref name="dstCapacity"/> was too small, otherwise false.
///May be NULL, in which case its value will not be assigned.</param>
///<returns>The number of <see cref="DataRegion"/>s that were written to <paramref name="dst"/>.</returns>
///<remarks>This function will scan through all <see cref="DataRegion"/>s in <paramref name="src"/>. Any <see cref="DataRegion"/> within the
///<paramref name="boundaryRegion"/> that does not exist within <paramref name="src"/> will be assigned to <paramref name="dst"/>, in ascending
///order.</remarks>
size_t get_missing_data_regions(DataRegion* dst, size_t dstCapacity, const DataRegion* src, size_t srcCount, DataRegion boundaryRegion, bool* dstTooSmall)
{
	if (dstTooSmall != NULL)
		*dstTooSmall = false;

	size_t count = 0;
	if (add_data_region(dst, boundaryRegion, &count, dstCapacity))
	{
		for (size_t i = 0; i < srcCount; i++)
		{
			DataRegion toRemove;
			bool doRemoveCurrent = false;
			if (does_data_region_contain_other_data_region(boundaryRegion, src[i]))
			{
				//src[i] is completely contained by 'boundaryRegion'
				toRemove = src[i];
				doRemoveCurrent = true;
			}
			else if (do_data_regions_intersect(boundaryRegion, src[i]))
			{
				//src[i] is partially contained by 'boundaryRegion'
				size_t firstIndex = src[i].first_index;
				size_t lastIndex = src[i].last_index;

				if (firstIndex < boundaryRegion.first_index)
					firstIndex = boundaryRegion.first_index;
				if (lastIndex > boundaryRegion.last_index)
					lastIndex = boundaryRegion.last_index;

				toRemove.first_index = firstIndex;
				toRemove.last_index = lastIndex;
				doRemoveCurrent = true;
			}

			if (doRemoveCurrent)
			{
				if (!remove_data_region(dst, toRemove, &count, dstCapacity))
				{
					if (dstTooSmall != NULL)
						*dstTooSmall = true;
					break;
				}
			}
		}

		return count;
	}
	else
	{
		if (dstTooSmall != NULL)
			*dstTooSmall = true;

		return 0;
	}
}