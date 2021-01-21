#ifndef DATA_REGION_H
#define DATA_REGION_H
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

typedef struct DataRegion
{
  int64_t first_index;
  int64_t last_index;
} DataRegion;

typedef struct DataRegionSet
{
  DataRegion* regions;
  int64_t count;
  int64_t capacity;
} DataRegionSet;

DataRegionSet* create_data_region_set_in(void* dst, int64_t dstSize)
{
  if(dst == NULL || dstSize < sizeof(DataRegionSet))
    return NULL;

  int64_t sizeForRegions = dstSize - (int64_t)sizeof(DataRegionSet);
  int64_t numRegions = sizeForRegions / (int64_t)sizeof(DataRegion);
  if(numRegions < 0)
    return NULL;

  DataRegionSet* set = dst;
  set->regions = (DataRegion*)((uint8_t*)dst + sizeof(DataRegionSet));
  set->capacity = numRegions;
  set->count = 0;
  return set;
}

DataRegionSet* create_data_region_set(int64_t regionCapacity)
{
  if(regionCapacity < 0)
    return NULL;

  size_t requiredSize = sizeof(DataRegionSet) + (sizeof(DataRegion) * regionCapacity);
  return create_data_region_set_in(malloc(requiredSize), requiredSize);
}

void free_data_region_set(DataRegionSet* set)
{
  free(set);
}

int64_t get_data_region_length(DataRegion region)
{
  return (region.last_index - region.first_index) + 1;
}

int does_data_region_contain_other_data_region(DataRegion outer, DataRegion inner)
{
  return inner.first_index >= outer.first_index && inner.last_index <= outer.last_index;
}

int do_data_regions_intersect(DataRegion a, DataRegion b)
{
  return a.last_index >= b.first_index && b.last_index >= a.first_index;
}

int are_data_regions_adjacent(DataRegion a, DataRegion b)
{
  return b.last_index == a.first_index - 1//'b' is left-adjacent to 'a'
    || b.first_index == a.last_index + 1;//'b' is right-adjacent to 'a'
}

int can_combine_data_regions(DataRegion a, DataRegion b)
{
  return are_data_regions_adjacent(a, b) || do_data_regions_intersect(a, b);
}

int is_data_region_valid(DataRegion region)
{
  return region.first_index <= region.last_index;
}

DataRegion combine_data_regions(DataRegion a, DataRegion b)
{
  DataRegion ret;

  ret.first_index = a.first_index;
  if (b.first_index < ret.first_index)//Take the min index
    ret.first_index = b.first_index;

  ret.last_index = a.last_index;
  if (b.last_index > ret.last_index)//Take the max index
    ret.last_index = b.last_index;

  return ret;
}

int64_t get_data_region_set_total_length(const DataRegionSet* set)
{
  //TODO: Optimize this to be a field of 'DataRegionSet', updated O(1) upon add/remove.
  if(set == NULL)
    return 0;

  int64_t total = 0;
  for(int64_t i = 0; i < set->count; i++)
    total += get_data_region_length(set->regions[i]);
  return total;
}

void internal_remove_data_region_at(DataRegionSet* set, int64_t index)
{
  //Move all values 'up' after the remove index
  for (int64_t i = index; i < set->count - 1; i++)
    set->regions[i] = set->regions[i + 1];

  set->count--;
}

void internal_insert_data_region_at(DataRegionSet* set, DataRegion toInsert, int64_t index)
{
  //Move all values 'down' after the insert index
  for (int64_t i = set->count; i > index; i--)
    set->regions[i] = set->regions[i - 1];

  //Insert the DataRegion to the specified index
  set->regions[index] = toInsert;
  set->count++;
}

typedef enum DataRegionSetResult
{
  DATA_REGION_SET_SUCCESS = 0,
  DATA_REGION_SET_NULL_ARG = -1,
  DATA_REGION_SET_INVALID_REGION = -2,
  DATA_REGION_SET_OUT_OF_SPACE = -3,
} DataRegionSetResult;

DataRegionSetResult add_data_region(DataRegionSet* set, DataRegion toAdd)
{
  if(set == NULL)
    return DATA_REGION_SET_NULL_ARG;
  if(!is_data_region_valid(toAdd))
    return DATA_REGION_SET_INVALID_REGION;

  int64_t insertPos = 0;

  //Find where to insert 'toAdd', and also remove any 'combinable' DataRegions
  for (int64_t i = 0; i < set->count; i++)
  {
    DataRegion current = set->regions[i];

    if (can_combine_data_regions(current, toAdd))
    {
      //Remove this DataRegion since it will be combined with 'toAdd'
      internal_remove_data_region_at(set, i);
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

  if(set->count < set->capacity)
  {
    //Insert 'toAdd'
    internal_insert_data_region_at(set, toAdd, insertPos);
    return DATA_REGION_SET_SUCCESS;
  }
  else
  {
    //Capacity is full, cannot add
    return DATA_REGION_SET_OUT_OF_SPACE;
  }
}

DataRegionSetResult remove_data_region(DataRegionSet* set, DataRegion toRemove)
{
  if(set == NULL)
    return DATA_REGION_SET_NULL_ARG;
  if(!is_data_region_valid(toRemove))
    return DATA_REGION_SET_INVALID_REGION;

  if (set->count == set->capacity)
  {
    /* Oh no, we may not have enough memory to complete the remove operation!
      Consider the following example:

      Regions Before Remove:       (0, 100) [Count = 1, Capacity = 1]
      Remove DataRegion Argument:  (25, 50)
      Regions After Remove :       (0, 24), (51, 100) [Count = 2, Capacity = 1] <<<< Problem: We exceeded the capacity!

      We have to detect whether the above problem will happen in order to prevent it. */
    int64_t removeCount = 0, insertCount = 0;
    for (int64_t i = 0; i < set->count; i++)
    {
      if (do_data_regions_intersect(set->regions[i], toRemove))
      {
        removeCount++;

        if (set->regions[i].first_index < toRemove.first_index)
        {
          //The left portion of regions[i] will remain (so we will add it to 'regions')
          insertCount++;
        }

        if (set->regions[i].last_index > toRemove.last_index)
        {
          //The right portion of regions[i] will remain (so we will add it to 'regions')
          insertCount++;
        }

        //See above, it is possible for removeCount to increment by one,
        //but insertCount could theoretically increment by two!
      }
      else
      {
        if (set->regions[i].first_index > toRemove.last_index)
          break;//Done checking for intersections
      }
    }

    if (insertCount > removeCount)
    {
      //We cannot remove, we need more capacity due to the split DataRegions
      return DATA_REGION_SET_OUT_OF_SPACE;
    }
  }

  //Remove all DataRegions within 'toRemove'
  for (int64_t i = 0; i < set->count; i++)
  {
    DataRegion currentRegion = set->regions[i];
    if (do_data_regions_intersect(currentRegion, toRemove))
    {
      internal_remove_data_region_at(set, i);//Remove 'currentRegion'

      if (currentRegion.first_index < toRemove.first_index)
      {
        //The left portion of 'currentRegion' will remain (so we will add it to 'regions')
        DataRegion leftPortion = { currentRegion.first_index, toRemove.first_index - 1 };
        internal_insert_data_region_at(set, leftPortion, i++);
      }

      if (currentRegion.last_index > toRemove.last_index)
      {
        //The right portion of 'currentRegion' will remain (so we will add it to 'regions')
        DataRegion rightPortion = { toRemove.last_index + 1, currentRegion.last_index };
        internal_insert_data_region_at(set, rightPortion, i++);
      }

      i--;//Since we removed 'currentRegion' (must do this AFTER potentially keeping left/right portions above)
    }
    else
    {
      if (set->regions[i].first_index > toRemove.last_index)
        break;//Done checking for intersections
    }
  }

  return DATA_REGION_SET_SUCCESS;
}

int64_t get_bounded_data_regions(DataRegion* dst, int64_t dstCapacity, const DataRegionSet* src, DataRegion boundaryRegion, int* dstTooSmall)
{
  int dstTooSmallPlaceholder;
  if(dstTooSmall == NULL)
    dstTooSmall = &dstTooSmallPlaceholder;
  *dstTooSmall = 0;

  if(src == NULL)
    return 0;
  if(!is_data_region_valid(boundaryRegion))
    return 0;

  if(dstCapacity < 0)
  {
    //Cannot have a negative destination capacity
    *dstTooSmall = 1;
    return 0;
  }

  int64_t count = 0;
  for (int64_t i = 0; i < src->count; i++)
  {
    DataRegion toYield;
    int doYieldCurrent = 0;
    if (does_data_region_contain_other_data_region(boundaryRegion, src->regions[i]))
    {
      //src->regions[i] is completely contained by 'boundaryRegion'
      toYield = src->regions[i];
      doYieldCurrent = 1;
    }
    else if(do_data_regions_intersect(boundaryRegion, src->regions[i]))
    {
      //src->regions[i] is partially contained by 'boundaryRegion'
      int64_t firstIndex = src->regions[i].first_index;
      int64_t lastIndex = src->regions[i].last_index;

      if (firstIndex < boundaryRegion.first_index)
        firstIndex = boundaryRegion.first_index;
      if (lastIndex > boundaryRegion.last_index)
        lastIndex = boundaryRegion.last_index;

      toYield.first_index = firstIndex;
      toYield.last_index = lastIndex;
      doYieldCurrent = 1;
    }

    if (doYieldCurrent)
    {
      if (dst != NULL)
      {
        if(count < dstCapacity)
        {
          dst[count++] = toYield;
        }
        else
        {
          *dstTooSmall = 1;
          break;
        }
      }
      else
      {
        //When 'dst' is NULL, it indicates that we are only counting the DataRegions
        count++;
      }
    }

    if (src->regions[i].first_index > boundaryRegion.last_index)
      break;//Beyond the boundary region, no need to continue iterating
  }

  return count;
}

int64_t count_bounded_data_regions(const DataRegionSet* src, DataRegion boundaryRegion)
{
  if(src == NULL)
    return 0;
  if(!is_data_region_valid(boundaryRegion))
    return 0;

  return get_bounded_data_regions(NULL/*NULL indicates that we only want to count the DataRegions*/, 0, src, boundaryRegion, NULL);
}

int64_t get_missing_data_regions(DataRegion* dst, int64_t dstCapacity, const DataRegionSet* src, DataRegion boundaryRegion, int* dstTooSmall)
{
  int dstTooSmallPlaceholder;
  if (dstTooSmall == NULL)
    dstTooSmall = &dstTooSmallPlaceholder;
  *dstTooSmall = 0;

  if(dst == NULL)
    return 0;
  if(src == NULL)
    return 0;
  if(!is_data_region_valid(boundaryRegion))
    return 0;

  if(dstCapacity < 0)
  {
    *dstTooSmall = 1;
    return 0;
  }

  DataRegionSet dstSet;
  dstSet.regions = dst;
  dstSet.capacity = dstCapacity;
  dstSet.count = 0;
  //TODO: Make sure this is up to date on initialization (maybe create an init function), as we expect to add a 'total_length' field that should be initialized to zero here.

  if (add_data_region(&dstSet, boundaryRegion) == DATA_REGION_SET_SUCCESS)
  {
    for (int64_t i = 0; i < src->count; i++)
    {
      DataRegion toRemove;
      int doRemoveCurrent = 0;
      if (does_data_region_contain_other_data_region(boundaryRegion, src->regions[i]))
      {
        //src->regions[i] is completely contained by 'boundaryRegion'
        toRemove = src->regions[i];
        doRemoveCurrent = 1;
      }
      else if (do_data_regions_intersect(boundaryRegion, src->regions[i]))
      {
        //src->regions[i] is partially contained by 'boundaryRegion'
        int64_t firstIndex = src->regions[i].first_index;
        int64_t lastIndex = src->regions[i].last_index;

        if (firstIndex < boundaryRegion.first_index)
          firstIndex = boundaryRegion.first_index;
        if (lastIndex > boundaryRegion.last_index)
          lastIndex = boundaryRegion.last_index;

        toRemove.first_index = firstIndex;
        toRemove.last_index = lastIndex;
        doRemoveCurrent = 1;
      }

      if (doRemoveCurrent)
      {
        if (remove_data_region(&dstSet, toRemove) != DATA_REGION_SET_SUCCESS)
        {
          *dstTooSmall = 1;
          //The regions stored in 'dst' are not correct, the latest region needs to be split but there isn't enough capacity.
          //So instead of returning the capacity, return zero to indicate that it's incomplete.
          return 0;
        }
      }
    }

    return dstSet.count;
  }
  else
  {
    *dstTooSmall = 1;
    return 0;
  }
}

#endif//DATA_REGION_H
