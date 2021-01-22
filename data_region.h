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
  int64_t total_length;
} DataRegionSet;

void internal_init_data_region_set(DataRegionSet* set, DataRegion* regions, int64_t capacity)
{
  set->regions = regions;
  set->capacity = capacity;
  set->count = 0;
  set->total_length = 0;
}

DataRegionSet* data_region_set_init_in(void* dst, int64_t dstSize)
{
  if(dst == NULL || dstSize < sizeof(DataRegionSet))
    return NULL;

  int64_t sizeForRegions = dstSize - (int64_t)sizeof(DataRegionSet);
  int64_t numRegions = sizeForRegions / (int64_t)sizeof(DataRegion);
  if(numRegions < 0)
    return NULL;

  DataRegionSet* set = dst;
  internal_init_data_region_set(set, (DataRegion*)((uint8_t*)dst + sizeof(DataRegionSet)), numRegions);
  return set;
}

DataRegionSet* data_region_set_create(int64_t regionCapacity)
{
  if(regionCapacity < 0)
    return NULL;

  size_t requiredSize = sizeof(DataRegionSet) + (sizeof(DataRegion) * regionCapacity);
  return data_region_set_init_in(malloc(requiredSize), requiredSize);
}

int64_t data_region_set_count(const DataRegionSet* set)
{
  if(set == NULL)
    return 0;
  else
    return set->count;
}

int64_t data_region_set_capacity(const DataRegionSet* set)
{
  if(set == NULL)
    return 0;
  else
    return set->capacity;
}

const DataRegion* data_region_set_at(const DataRegionSet* set, int64_t index)
{
  if(set == NULL || index < 0 || index >= set->count)
    return NULL;
  return &set->regions[index];
}

int64_t data_region_set_total_length(const DataRegionSet* set)
{
  if(set == NULL)
    return 0;
  else
    return set->total_length;
}

void data_region_set_free(DataRegionSet* set)
{
  free(set);
}

int64_t data_region_length(DataRegion region)
{
  return (region.last_index - region.first_index) + 1;
}

int data_region_contains(DataRegion outer, DataRegion inner)
{
  return inner.first_index >= outer.first_index && inner.last_index <= outer.last_index;
}

int data_region_intersects(DataRegion a, DataRegion b)
{
  return a.last_index >= b.first_index && b.last_index >= a.first_index;
}

int data_region_is_adjacent(DataRegion a, DataRegion b)
{
  return b.last_index == a.first_index - 1//'b' is left-adjacent to 'a'
    || b.first_index == a.last_index + 1;//'b' is right-adjacent to 'a'
}

int data_region_can_combine(DataRegion a, DataRegion b)
{
  return data_region_is_adjacent(a, b) || data_region_intersects(a, b);
}

int data_region_is_valid(DataRegion region)
{
  return region.first_index <= region.last_index;
}

DataRegion data_region_combine(DataRegion a, DataRegion b)
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

void _data_region_set_remove_at(DataRegionSet* set, int64_t index)
{
  int64_t removeLength = data_region_length(set->regions[index]);
  //Move all values 'up' after the remove index
  for (int64_t i = index; i < set->count - 1; i++)
    set->regions[i] = set->regions[i + 1];

  set->count--;
  set->total_length -= removeLength;
}

void _data_region_set_insert_at(DataRegionSet* set, DataRegion toInsert, int64_t index)
{
  //Move all values 'down' after the insert index
  for (int64_t i = set->count; i > index; i--)
    set->regions[i] = set->regions[i - 1];

  //Insert the DataRegion to the specified index
  set->regions[index] = toInsert;
  set->count++;
  set->total_length += data_region_length(toInsert);
}

typedef enum DataRegionSetResult
{
  DATA_REGION_SET_SUCCESS = 0,
  DATA_REGION_SET_NULL_ARG = -1,
  DATA_REGION_SET_INVALID_REGION = -2,
  DATA_REGION_SET_OUT_OF_SPACE = -3,
} DataRegionSetResult;

DataRegionSetResult data_region_set_add(DataRegionSet* set, DataRegion toAdd)
{
  if(set == NULL)
    return DATA_REGION_SET_NULL_ARG;
  if(!data_region_is_valid(toAdd))
    return DATA_REGION_SET_INVALID_REGION;

  int64_t insertPos = 0;

  //Find where to insert 'toAdd', and also remove any 'combinable' DataRegions
  for (int64_t i = 0; i < set->count; i++)
  {
    DataRegion current = set->regions[i];

    if (data_region_can_combine(current, toAdd))
    {
      //Remove this DataRegion since it will be combined with 'toAdd'
      _data_region_set_remove_at(set, i);
      toAdd = data_region_combine(current, toAdd);
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
    _data_region_set_insert_at(set, toAdd, insertPos);
    return DATA_REGION_SET_SUCCESS;
  }
  else
  {
    //Capacity is full, cannot add
    return DATA_REGION_SET_OUT_OF_SPACE;
  }
}

DataRegionSetResult data_region_set_remove(DataRegionSet* set, DataRegion toRemove)
{
  if(set == NULL)
    return DATA_REGION_SET_NULL_ARG;
  if(!data_region_is_valid(toRemove))
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
      if (data_region_intersects(set->regions[i], toRemove))
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
    if (data_region_intersects(currentRegion, toRemove))
    {
      _data_region_set_remove_at(set, i);//Remove 'currentRegion'

      if (currentRegion.first_index < toRemove.first_index)
      {
        //The left portion of 'currentRegion' will remain (so we will add it to 'regions')
        DataRegion leftPortion = { currentRegion.first_index, toRemove.first_index - 1 };
        _data_region_set_insert_at(set, leftPortion, i++);
      }

      if (currentRegion.last_index > toRemove.last_index)
      {
        //The right portion of 'currentRegion' will remain (so we will add it to 'regions')
        DataRegion rightPortion = { toRemove.last_index + 1, currentRegion.last_index };
        _data_region_set_insert_at(set, rightPortion, i++);
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

int64_t data_region_set_crop(DataRegion* dst, int64_t dstCapacity, const DataRegionSet* src, DataRegion boundaryRegion, int* dstTooSmall)
{
  int dstTooSmallPlaceholder;
  if(dstTooSmall == NULL)
    dstTooSmall = &dstTooSmallPlaceholder;
  *dstTooSmall = 0;

  if(src == NULL)
    return 0;
  if(!data_region_is_valid(boundaryRegion))
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
    if (data_region_contains(boundaryRegion, src->regions[i]))
    {
      //src->regions[i] is completely contained by 'boundaryRegion'
      toYield = src->regions[i];
      doYieldCurrent = 1;
    }
    else if(data_region_intersects(boundaryRegion, src->regions[i]))
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

int64_t data_region_set_count_crop(const DataRegionSet* src, DataRegion boundaryRegion)
{
  if(src == NULL)
    return 0;
  if(!data_region_is_valid(boundaryRegion))
    return 0;

  return data_region_set_crop(NULL/*NULL indicates that we only want to count the DataRegions*/, 0, src, boundaryRegion, NULL);
}

int64_t data_region_set_negative_crop(DataRegion* dst, int64_t dstCapacity, const DataRegionSet* src, DataRegion boundaryRegion, int* dstTooSmall)
{
  int dstTooSmallPlaceholder;
  if (dstTooSmall == NULL)
    dstTooSmall = &dstTooSmallPlaceholder;
  *dstTooSmall = 0;

  if(dst == NULL)
    return 0;
  if(src == NULL)
    return 0;
  if(!data_region_is_valid(boundaryRegion))
    return 0;

  if(dstCapacity < 0)
  {
    *dstTooSmall = 1;
    return 0;
  }

  DataRegionSet dstSet;
  internal_init_data_region_set(&dstSet, dst, dstCapacity);

  if (data_region_set_add(&dstSet, boundaryRegion) == DATA_REGION_SET_SUCCESS)
  {
    for (int64_t i = 0; i < src->count; i++)
    {
      DataRegion toRemove;
      int doRemoveCurrent = 0;
      if (data_region_contains(boundaryRegion, src->regions[i]))
      {
        //src->regions[i] is completely contained by 'boundaryRegion'
        toRemove = src->regions[i];
        doRemoveCurrent = 1;
      }
      else if (data_region_intersects(boundaryRegion, src->regions[i]))
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
        if (data_region_set_remove(&dstSet, toRemove) != DATA_REGION_SET_SUCCESS)
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
