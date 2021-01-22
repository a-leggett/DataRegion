#ifndef DATA_REGION_H
#define DATA_REGION_H
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

/* Representation of a region of data (no payload is stored, just indices). */
typedef struct DataRegion
{
  /* The first index of the region. */
  int64_t first_index;

  /* The last index of the region. */
  int64_t last_index;
} DataRegion;

/* Collection of DataRegions. All DataRegions are stored in ascending order,
 * and no DataRegions are overlapping or immediately adjacent.
 * Initialize this structure via 'data_region_set_init_in' or allocate a new
 * one via 'data_region_set_create'.
 * @see data_region_set_add
 * @see data_region_set_remove
 * @see data_region_set_count
 * @see data_region_set_crop
 * @see data_region_set_count_crop
 * @see data_region_set_negative_crop */
typedef struct DataRegionSet
{
  DataRegion* regions;
  int64_t count;
  int64_t capacity;
  int64_t total_length;
} DataRegionSet;

/* Internal function to initialize a DataRegionSet structure.
 * @param set - Pointer to the DataRegionSet to initialize.
 * @param regions - Pointer to the DataRegion array.
 * @param capacity - The number of DataRegions that can be stored in 'regions'. */
void _data_region_set_init(DataRegionSet* set, DataRegion* regions, int64_t capacity)
{
  set->regions = regions;
  set->capacity = capacity;
  set->count = 0;
  set->total_length = 0;
}

/* Initializes a DataRegionSet in existing memory.
 * @param dst - The memory which will store the DataRegionSet. If this argument
 *            is NULL, then NULL will be returned.
 * @param dstSize - The number of bytes from 'dst' to give to the
 *                DataRegionSet.
 * @returns - Pointer to the DataRegionSet (in 'dst'), or NULL if the
 *          initialization failed.
 * @remarks - This function will try to store as large of a DataRegionSet as
 *          possible in the given 'dstSize'. Note that it's possible for a
 *          DataRegionSet to be initialized with a capacity of zero DataRegions
 *          if 'dstSize' is too small. If 'dstSize' is too small for even the
 *          'DataRegionSet' structure, then NULL will be returned. */
DataRegionSet* data_region_set_init_in(void* dst, int64_t dstSize)
{
  if(dst == NULL || dstSize < sizeof(DataRegionSet))
    return NULL;

  int64_t sizeForRegions = dstSize - (int64_t)sizeof(DataRegionSet);
  int64_t numRegions = sizeForRegions / (int64_t)sizeof(DataRegion);
  if(numRegions < 0)
    return NULL;

  DataRegionSet* set = dst;
  _data_region_set_init(set, (DataRegion*)((uint8_t*)dst + sizeof(DataRegionSet)), numRegions);
  return set;
}

/* Allocates a new DataRegionSet with a specific capacity.
 * @param regionCapacity - The maximum number of DataRegions that can be stored
 *        in the allocated DataRegionSet. If this value is less than zero, then
 *        NULL will be returned.
 * @returns - A pointer to the allocated DataRegionSet, or NULL upon failure.
 * @remarks - The memory will be allocated via 'malloc'. If you prefer to use
 *          already existing memory, call 'data_region_set_init_in'.
 *          If 'malloc' returns NULL, then NULL will be returned.
 *          Be sure to free the returned DataRegionSet by calling the
 *          'data_region_set_free' function.
 * @see data_region_set_init_in
 * @see data_region_set_free  */
DataRegionSet* data_region_set_create(int64_t regionCapacity)
{
  if(regionCapacity < 0)
    return NULL;

  size_t requiredSize = sizeof(DataRegionSet) + (sizeof(DataRegion) * regionCapacity);
  return data_region_set_init_in(malloc(requiredSize), requiredSize);
}

/* Gets the number of DataRegions that are stored in a DataRegionSet.
 * @param set - Pointer to the DataRegionSet. If this is NULL, then
 *        zero will be returned.
 * @returns - The number of DataRegions stored in the DataRegionSet.
 * @see data_region_set_capacity */
int64_t data_region_set_count(const DataRegionSet* set)
{
  if(set == NULL)
    return 0;
  else
    return set->count;
}

/* Gets the maximum number of DataRegions that can possibly be stored in a
 * DataRegionSet.
 * @param set - Pointer to the DataRegionSet. If this is NULL, then zero
 *        will be returned.
 * @returns - The maximum capacity of the DataRegionSet.
 * @see data_region_set_count */
int64_t data_region_set_capacity(const DataRegionSet* set)
{
  if(set == NULL)
    return 0;
  else
    return set->capacity;
}

/* Gets a pointer to a DataRegion stored at a particular index in a
 * DataRegionSet.
 * @param set - Pointer to the DataRegionSet. If this is NULL, then NULL
 *        will be returned.
 * @param index - The zero-based index of the DataRegion. If this is out
 *        of bounds, then NULL will be returned.
 * @returns - The pointer to the DataRegion stored in the 'set' at the
 *          specified 'index'. */
const DataRegion* data_region_set_at(const DataRegionSet* set, int64_t index)
{
  if(set == NULL || index < 0 || index >= set->count)
    return NULL;
  return &set->regions[index];
}

/* Gets the length of the sum of all DataRegions stored in a DataRegionSet.
 * @param set - Poitner to the DataRegionSet. If this is NULL, then zero
 *        will be returned.
 * @returns - The total length of all stored DataRegions. */
int64_t data_region_set_total_length(const DataRegionSet* set)
{
  if(set == NULL)
    return 0;
  else
    return set->total_length;
}

/* Clears all DataRegions from a DataRegionSet.
 * @param set - Pointer to the DataRegionSet to clear.
 *        If this argument is NULL, nothing will happen. */
void data_region_set_clear(DataRegionSet* set)
{
  if(set != NULL)
  {
    set->count = 0;
    set->total_length = 0;
  }
}

/* Frees a DataRegionSet that was allocated by the 'data_region_set_create'
 * function.
 * @param set - Pointer to the DataRegionSet. If this argument is NULL, then
 *        nothing will happen.
 * @remarks - Only use this function to free DataRegionSets that were allocated
 *          by the 'data_region_set_create' function. */
void data_region_set_free(DataRegionSet* set)
{
  if(set != NULL)
    free(set);
}

/* Gets the length of a single DataRegion.
 * @param region - The input DataRegion.
 * @returns - The length of the DataRegion. */
int64_t data_region_length(DataRegion region)
{
  return (region.last_index - region.first_index) + 1;
}

/* Checks whether one DataRegion strictly contains another.
 * @param outer - The outer DataRegion.
 * @param inner - The inner DataRegion.
 * @returns - True (1) if the 'outer' region entirely contains
 *          the 'inner' region, otherwise false (0).
 * @remarks - If the 'outer' region is equal to the 'inner' region, then
 *          'outer' is considered to contain 'inner', so true (1) will be
 *          returned. */
int data_region_contains(DataRegion outer, DataRegion inner)
{
  return inner.first_index >= outer.first_index && inner.last_index <= outer.last_index;
}

/* Checks whether two DataRegions have at least one intersecting byte index.
 * @param a - The first DataRegion.
 * @param b - The second DataRegion.
 * @returns - True (1) if there is at least one index that is contained by
 *          both 'a' and 'b', otherwise false (0). */
int data_region_intersects(DataRegion a, DataRegion b)
{
  return a.last_index >= b.first_index && b.last_index >= a.first_index;
}

/* Checks whether two DataRegions are adjacent.
 * @param a - The first DataRegion.
 * @param b - The second DataRegion.
 * @returns - True (1) if 'a' ends immediatly before 'b' begins, or if
 *          'b' ends immediately before 'a' begins, otherwise false (0). */
int data_region_is_adjacent(DataRegion a, DataRegion b)
{
  return b.last_index == a.first_index - 1//'b' is left-adjacent to 'a'
    || b.first_index == a.last_index + 1;//'b' is right-adjacent to 'a'
}

/* Checks whether two DataRegions can be combined into one.
 * @param 'a' - The first DataRegion.
 * @param 'b' - The second DataRegion.
 * @returns - True (1) if 'a' and 'b' can be combined into one DataRegion,
 *          otherwise false (0).
 * @remarks - Two DataRegions can be combined if they are adjacent or
 *           intersecting.
 * @see data_region_contains
 * @see data_region_intersects */
int data_region_can_combine(DataRegion a, DataRegion b)
{
  return data_region_is_adjacent(a, b) || data_region_intersects(a, b);
}

/* Checks whether a DataRegion is valid.
 * @param region - The input DataRegion.
 * @returns - True (1) if the region is valid, otherwise false (0).
 * @remarks - A DataRegion is considered valid if the first_index
 *          is less than or equal to the last_index. */
int data_region_is_valid(DataRegion region)
{
  return region.first_index <= region.last_index;
}

/* Combines two DataRegions into one.
 * @param a - The first DataRegion. Must be valid, see 'data_region_is_valid'.
 * @param b - The second DataRegion. Must be valid, see 'data_region_is_valid'.
 * @returns - The combination of 'a' and 'b'.
 * @remarks - You must only combine two DataRegions that are conbisdered
 *          to be combinable, see 'data_region_can_combine'. If 'a' and 'b'
 *          cannot be combined (or any one is invalid), then the result is some
 *          undefined DataRegion value. */
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

/* Internal function to remove a DataRegion from a DataRegionSet at a
 * specific index.
 * @param set - Pointer to the DataRegionSet from which to remove.
 * @param index - The zero-based index of the DataRegion to remove.
 * @remarks - Most of the time, the application should not call this function.
 *          Instead, the application should call 'data_region_set_remove'. */
void _data_region_set_remove_at(DataRegionSet* set, int64_t index)
{
  int64_t removeLength = data_region_length(set->regions[index]);
  //Move all values 'up' after the remove index
  for (int64_t i = index; i < set->count - 1; i++)
    set->regions[i] = set->regions[i + 1];

  set->count--;
  set->total_length -= removeLength;
}

/* Internal function to add a DataRegion into a DataRegionSet at a
 * specific index.
 * @param set - Pointer to the DataRegionSet into which to insert.
 * @param toInsert - The DataRegion value to insert.
 * @param index - The index at which to insert the DataRegion.
 * @remarks - Most of the time, the application should not call this function.
 *          Instead, the application should call 'data_region_set_add'. */
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

/* Defines the result of a DataRegionSet operation. */
typedef enum DataRegionSetResult
{
  /* The operation was successful. */
  DATA_REGION_SET_SUCCESS = 0,

  /* The operation failed due to a NULL argument. */
  DATA_REGION_SET_NULL_ARG = -1,

  /* The operation failed due to an invalid DataRegion argument.
   * @see data_region_is_valid  */
  DATA_REGION_SET_INVALID_REGION = -2,

  /* The operation failed because the DataRegionSet's capacity
   * was full and the operation needed to insert at least one
   * more DataRegion. */
  DATA_REGION_SET_OUT_OF_SPACE = -3,
} DataRegionSetResult;

/* Adds a DataRegion to a DataRegionSet.
 * @param set - The destination DataRegionSet. If this is NULL, then
 *        DATA_REGION_SET_NULL_ARG will be returned.
 * @param toAdd - The DataRegion to add. If this is invalid (see
 *        data_region_is_valid), then DATA_REGION_SET_INVALID_REGION will
 *        be returned.
 * @returns - The DataRegionSetResult that defines the result of the add
 *          operation. If all arguments are non-null and valid, then the
 *          result will be either DATA_REGION_SET_SUCCESS or
 *          DATA_REGION_SET_OUT_OF_SPACE.
 * @remarks - The input DataRegion will be 'combined' with any combinable
 *          DataRegions in the set (see 'data_region_can_combine'), so it's
 *          possible for the 'count' of the DataRegionSet to be reduced. If
 *          there is no remaining space in the DataRegionSet, and the input
 *          DataRegion can't be combined with any stored DataRegion, them
 *          DATA_REGION_SET_OUT_OF_SPACE will be returned and nothing will
 *          change. */
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

/* Removes a DataRegion from a DataRegionSet.
 * @param set - Pointer to the DataRegionSet from which to remove the
 *        DataRegion. If this argument is NULL, then
 *        DATA_REGION_SET_NULL_ARG will be returned.
 * @param toRemove - The DataRegion to remove. If this is invalid
 *        (see data_region_is_valid), then DATA_REGION_SET_INVALID_REGION
 *        will be returned.
 * @returns - The DataRegionSetResult that defined the result of the removal
 *          operation. If all arguments are non-null and valid, then the result
 *          is either DATA_REGION_SET_SUCCESS or DATA_REGION_SET_OUT_OF_SPACE.
 * @remarks - This function will 'subtract' from all DataRegions stored in the
 *          set that intersect with 'toRemove'. It's possible for this
 *          'subtraction' to cause an existing DataRegion to be split into two
 *          DataRegions. Note that this means it's possible for the 'count' of
 *          the DataRegionSet to increase after a removal operation. It's also
 *          possible that the removal operation may fail if the DataRegionSet
 *          was full before a split was required, in which case
 *          DATA_REGION_SET_OUT_OF_SPACE will be returned and the DataRegionSet
 *          will remain unchanged. */
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

/* Copies a subset of DataRegions in a DataRegionSet to an array.
 * @param dst - The destination array. This may be NULL if you want to only
 *        count the DataRegions.
 * @param dstCapacity - The maximum number of DataRegions that can be stored in
 *        the 'dst' array. If this is less than zero, then zero is returned.
 * @param src - The source DataRegionSet. If 'dst' is NULL, then this argument
 *        should be zero.
 * @param boundaryRegion - The DataRegion that defines the crop boundary. No
 *        DataRegions will be read from outside of this region, and all
 *        DataRegions that cross this region will be trimmed to fit inside
 *        this region. If this region is invalid (see data_region_is_valid),
 *        then zero will be returned.
 * @param dstTooSmall - Optional pointer to an integer that will be assigned
 *        to true (1) if the destination buffer was too small to contain the
 *        cropped DataRegions, otherwise false (0). This argument may be NULL,
 *        in which case it will not be dereferenced for assignment.
 * @returns - The number of DataRegions that were found within the 'crop'
 *          region, limited to 'dstCapacity' if 'dst' was non-NULL. */
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

/* Counts the number of DataRegions that are at least partially contained
 * within a specific boundary region.
 * @param src - Pointer to the DataRegionSet. If this is NULL, then zero
 *        will be returned.
 * @param boundaryRegion - The DataRegion that defines the boundary.
 *        If this is invalid (see data_region_is_valid), then zero will be
 *        returned.
 * @returns - The number of DataRegions that are either completely contained by, or
 *          intersect with, the 'boundaryRegion'.
 * @remarks - Note that this function is equivalent to calling data_region_set_crop
 *          with a NULL 'dst' argument. */
int64_t data_region_set_count_crop(const DataRegionSet* src, DataRegion boundaryRegion)
{
  if(src == NULL)
    return 0;
  if(!data_region_is_valid(boundaryRegion))
    return 0;

  return data_region_set_crop(NULL/*NULL indicates that we only want to count the DataRegions*/, 0, src, boundaryRegion, NULL);
}

/* Copies a 'negative' of a subset of DataRegions within a DataRegionSet.
 * @param dst - The destination DataRegion array which will contain the results.
 *        If this is NULL, then zero will be returned.
 * @param dstCapacity - The maximum number of DataRegions that can be stored in
 *        the 'dst' array.
 * @param src - Pointer to the source DataRegionSet. If this argument is NULL,
 *        then zero will be returned.
 * @param boundaryRegion - DataRegion that defines the boundary of the negative
 *        crop region. No DataRegions will be read outside of this region, and
 *        DataRegions that intersect it will be trimmed. If this argument is
 *        invalid (see data_region_is_valid), then zero will be returned.
 * @param dstTooSmall - Optional pointer to an integer that will be assigned
 *        to true (1) if the destination buffer was too small to contain the
 *        cropped and negated DataRegions, otherwise false (0). This argument
 *        may be NULL, in which case it will not be dereferenced for assignment.
 * @returns - The number of negative cropped DataRegions that were copied into the
 *          'dst' array, or zero upon failure.
 * @remarks - A 'negative crop' is similar to a normal crop (see
 *          data_region_set_crop), but where DataRegions that are present in the
 *          DataRegionSet will be omitted, and DataRegions that are missing in the
 *          DataRegionSet will be yielded.
 *          If the 'dstCapacity' is too small, then zero will be returned but some
 *          garbage data will have been written to 'dst'. */
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
  _data_region_set_init(&dstSet, dst, dstCapacity);

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
