#ifndef DATA_REGION_H
#define DATA_REGION_H
#include <stdbool.h>
#include <assert.h>//TODO: I'm REALLY on the fence about including asserts. It would be nice to catch assert failures in debug (particularly helpful is app code sends wrong args... Maybe have return codes for such cases? But what about things that already have return codes, like combine_data_regions?)
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

bool is_data_region_valid(DataRegion region)
{
  return region.first_index <= region.last_index;
}

DataRegion combine_data_regions(DataRegion a, DataRegion b)
{
  assert(is_data_region_valid(a));
  assert(is_data_region_valid(b));
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
  assert(set != NULL);
  assert(index >= 0);
  assert(index < set->count);

  //Move all values 'up' after the remove index
  for (int64_t i = index; i < set->count - 1; i++)
    set->regions[i] = set->regions[i + 1];

  set->count--;
}

void internal_insert_data_region_at(DataRegionSet* set, DataRegion toInsert, int64_t index)
{
  assert(set != NULL);
  assert(is_data_region_valid(toInsert));
  assert(index >= 0);
  assert(set->count < set->capacity);
  assert(index <= set->count);//index==count when adding to end

  //Move all values 'down' after the insert index
  for (int64_t i = set->count; i > index; i--)
    set->regions[i] = set->regions[i - 1];

  //Insert the DataRegion to the specified index
  set->regions[index] = toInsert;
  set->count++;
}

bool add_data_region(DataRegionSet* set, DataRegion toAdd)
{
  if(set == NULL)
    return false;
  if(!is_data_region_valid(toAdd))
    return false;//TODO: Maybe use enum to define error instead of returning bool

  if (set->count < set->capacity)
  {
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

    //Insert 'toAdd'
    internal_insert_data_region_at(set, toAdd, insertPos);
    return true;
  }
  else
  {
    //Capacity is full, cannot add
    return false;
  }
}

bool remove_data_region(DataRegionSet* set, DataRegion toRemove)
{
  if(set == NULL)
    return false;
  if(!is_data_region_valid(toRemove))
    return false;

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
      return false;
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

  return true;
}

int64_t get_bounded_data_regions(DataRegion* dst, int64_t dstCapacity, const DataRegionSet* src, DataRegion boundaryRegion, bool* dstTooSmall)
{
  if(src == NULL)
    return 0;
  if(!is_data_region_valid(boundaryRegion))
    return 0;

  bool dstTooSmallPlaceholder;
  if(dstTooSmall == NULL)
    dstTooSmall = &dstTooSmallPlaceholder;

  if(dstCapacity < 0)
  {
    //Cannot have a negative destination capacity
    *dstTooSmall = true;
    return 0;
  }

  *dstTooSmall = false;
  int64_t count = 0;
  for (int64_t i = 0; i < src->count; i++)
  {
    DataRegion toYield;
    bool doYieldCurrent = false;
    if (does_data_region_contain_other_data_region(boundaryRegion, src->regions[i]))
    {
      //src->regions[i] is completely contained by 'boundaryRegion'
      toYield = src->regions[i];
      doYieldCurrent = true;
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
      doYieldCurrent = true;
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
          *dstTooSmall = true;//TODO: Definitely test this
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

int64_t get_missing_data_regions(DataRegion* dst, int64_t dstCapacity, const DataRegionSet* src, DataRegion boundaryRegion, bool* dstTooSmall)
{
  if(dst == NULL)
    return 0;
  if(src == NULL)
    return 0;
  if(!is_data_region_valid(boundaryRegion))
    return 0;

  bool dstTooSmallPlaceholder;
  if (dstTooSmall == NULL)
    dstTooSmall = &dstTooSmallPlaceholder;

  if(dstCapacity < 0)
  {
    *dstTooSmall = true;
    return 0;
  }

  DataRegionSet dstSet;
  dstSet.regions = dst;
  dstSet.capacity = dstCapacity;
  dstSet.count = 0;

  *dstTooSmall = false;
  int64_t count = 0;
  if (add_data_region(&dstSet, boundaryRegion))
  {
    for (int64_t i = 0; i < src->count; i++)
    {
      DataRegion toRemove;
      bool doRemoveCurrent = false;
      if (does_data_region_contain_other_data_region(boundaryRegion, src->regions[i]))
      {
        //src->regions[i] is completely contained by 'boundaryRegion'
        toRemove = src->regions[i];
        doRemoveCurrent = true;
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
        doRemoveCurrent = true;
      }

      if (doRemoveCurrent)
      {
        if (!remove_data_region(&dstSet, toRemove))
        {
          *dstTooSmall = true;
          break;
        }
      }
    }

    return count;
  }
  else
  {
    *dstTooSmall = true;
    return 0;
  }
}

#endif//DATA_REGION_H
