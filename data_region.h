#ifndef DATA_REGION_H
#define DATA_REGION_H
#include <stdbool.h>
#include <assert.h>

typedef struct DataRegion
{
  size_t first_index;
  size_t last_index;
} DataRegion;

bool does_data_region_contain_other_data_region(DataRegion outer, DataRegion inner)
{
  return inner.first_index >= outer.first_index && inner.last_index <= outer.last_index;
}

bool do_data_regions_intersect(DataRegion a, DataRegion b)
{
  return a.last_index >= b.first_index && b.last_index >= a.first_index;
}

bool are_data_regions_adjacent(DataRegion a, DataRegion b)
{
  return b.last_index == a.first_index - 1//'b' is left-adjacent to 'a'
    || b.first_index == a.last_index + 1;//'b' is right-adjacent to 'a'
}

bool can_combine_data_regions(DataRegion a, DataRegion b)
{
  return are_data_regions_adjacent(a, b) || do_data_regions_intersect(a, b);
}

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

size_t count_bounded_data_regions(const DataRegion* src, size_t srcCount, DataRegion boundaryRegion)
{
  return get_bounded_data_regions(NULL/*NULL indicates that we only want to count the DataRegions*/, 0, src, srcCount, boundaryRegion, NULL);
}

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

#endif//DATA_REGION_H
