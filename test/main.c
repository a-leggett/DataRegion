#include "../data_region.h"
#include "gidunit.h"

DataRegionSet* init_test_data_region_set(DataRegionSet* set, int randCount)
{
  for(int i = 0; i < randCount; i++)
  {
    if(add_data_region(set, (DataRegion){.first_index = i*20, .last_index=(i*20)+9}) != DATA_REGION_SET_SUCCESS)
      return NULL;
  }
  return set;
}

#define create_test_data_region_set(capacity, randCount)                   \
  init_test_data_region_set(                                               \
    create_data_region_set_in(                                             \
      gid_malloc(sizeof(DataRegionSet) + (sizeof(DataRegion) * capacity)), \
      sizeof(DataRegionSet) + (sizeof(DataRegion) * capacity)              \
    ),                                                                     \
    randCount                                                              \
  )

#define free_test_data_region_set(set) gid_free(set)

DataRegionSet* clone_data_region_set(const DataRegionSet* set)
{
  DataRegionSet* ret = create_data_region_set(set->capacity);
  ret->count = set->count;
  ret->total_length = get_data_region_set_total_length(set);
  for(int64_t i = 0; i < set->count; i++)
    ret->regions[i] = set->regions[i];
  return ret;
}

#define assert_data_region_array_eq(array, ...)                               \
{                                                                             \
  DataRegion _local_expect[] = {__VA_ARGS__};                                 \
  size_t _local_expect_count = sizeof(_local_expect)/sizeof(_local_expect[0]);\
  for(uint64_t i = 0; i < _local_expect_count; i++)                           \
  {                                                                           \
    assert_message_format(                                                    \
         _local_expect[i].first_index == array[i].first_index                 \
      && _local_expect[i].last_index == array[i].last_index,                  \
      "Expected "#array"[%"PRIu64"] to be (%"PRId64", %"PRId64"), but the "   \
      "actual value was (%"PRId64", %"PRId64").",                             \
      i,                                                                      \
      _local_expect[i].first_index,                                           \
      _local_expect[i].last_index,                                            \
      array[i].first_index,                                                   \
      array[i].last_index);                                                   \
  }                                                                           \
}

#define assert_data_region_set_eq(expected, actual)                           \
{                                                                             \
  const DataRegionSet* _local_exp = (expected);                               \
  const DataRegionSet* _local_act = (actual);                                 \
  assert_not_null(_local_exp);                                                \
  assert_not_null(_local_act);                                                \
                                                                              \
  assert_int_eq(_local_exp->capacity, _local_act->capacity);                  \
  assert_int_eq(_local_exp->count, _local_act->count);                        \
  assert_int_eq(                                                              \
    get_data_region_set_total_length(_local_exp),                             \
    get_data_region_set_total_length(_local_act));                            \
  assert_memory_eq(_local_exp->regions, _local_act->regions, (sizeof(DataRegion)*_local_exp->count));\
}

#define free_clone_data_region_set(set) free_data_region_set(set)

#define assert_add_data_region(dst, firstIndex, lastIndex)                    \
{                                                                             \
  DataRegion _local_region = (DataRegion){.first_index = (firstIndex),        \
    .last_index = (lastIndex)};                                               \
  DataRegionSetResult _local_add_result = add_data_region(dst, _local_region);\
  assert_message_format(_local_add_result == DATA_REGION_SET_SUCCESS,         \
    "Failed to add a data region. Expected %i, but the result was %i.",       \
    DATA_REGION_SET_SUCCESS,                                                  \
    _local_add_result);                                                       \
}

#define assert_remove_data_region(set, firstIndex, lastIndex)                 \
{                                                                             \
  DataRegion _local_region = (DataRegion){.first_index = (firstIndex),        \
    .last_index = (lastIndex)};                                               \
  DataRegionSetResult _local_remove_result = remove_data_region(set,          \
    _local_region);                                                           \
  assert_message_format(_local_remove_result == DATA_REGION_SET_SUCCESS,      \
    "Failed to remove a data region. Expected %i, but the result was %i.",    \
    DATA_REGION_SET_SUCCESS,                                                  \
    _local_remove_result);                                                    \
}

#define DR(firstIndex, lastIndex) ((DataRegion){.first_index = (firstIndex),  \
  .last_index = (lastIndex)})

#define assert_data_region_set_eq_array(set, ...)                             \
{                                                                             \
  DataRegion _local_regions[] = {                                             \
    __VA_ARGS__                                                               \
  };                                                                          \
  size_t _local_count = sizeof(_local_regions)/sizeof(_local_regions[0]);     \
  assert_message_format(_local_count == set->count,                           \
    "Expected the DataRegionSet to contain %"PRIu64" regions, but it contained %"PRIu64".", \
    _local_count, set->count);                                                \
  int64_t _local_total_length = 0;                                            \
  for(int64_t i = 0; i < set->count; i++)                                     \
  {                                                                           \
    assert_message_format(                                                    \
       set->regions[i].first_index == _local_regions[i].first_index           \
    && set->regions[i].last_index == _local_regions[i].last_index,            \
      "Expected the data region at index %"PRId64" to be (%"PRId64", "        \
      "%"PRId64"), but the actual region was (%"PRId64", %"PRId64")",         \
      i,                                                                      \
      _local_regions[i].first_index,                                          \
      _local_regions[i].last_index,                                           \
      set->regions[i].first_index,                                            \
      set->regions[i].last_index                                              \
    );                                                                        \
    _local_total_length += get_data_region_length(_local_regions[i]);         \
  }                                                                           \
  int64_t _local_actual_length = get_data_region_set_total_length(set);       \
  assert_message_format(_local_total_length == _local_actual_length,          \
    "Expected the total length to equal %"PRId64", but it was %"PRId64".",    \
    _local_total_length,                                                      \
    _local_actual_length);                                                    \
}

BEGIN_TEST_SUITE(DataRegionSetInitialization)

  Test(create_data_region_set_in_when_dst_is_null,
    RangeParam(size, 0, sizeof(DataRegionSet)+128))
  {
    DataRegionSet* got = create_data_region_set_in(NULL, size);
    assert_null(got);
  }

  Test(create_data_region_set_in_when_dstSize_is_negative,
    EnumParam(negativeSize, -10000, -100, -10, -1))
  {
    uint8_t* mem = gid_malloc(0);
    DataRegionSet* got = create_data_region_set_in(mem, negativeSize);
    assert_null(got);
    gid_free(mem);
  }

  Test(create_data_region_set_in_when_size_too_small_for_struct,
    RangeParam(deficit, 1, sizeof(DataRegionSet)))
  {
    size_t memSize = sizeof(DataRegionSet) - deficit;
    uint8_t* mem = gid_malloc(memSize);

    DataRegionSet* got = create_data_region_set_in(mem, memSize);
    assert_null(got);//Because the memSize was too small

    gid_free(mem);
  }

  Test(create_data_region_set_in_only_enough_space_for_struct,
    RangeParam(surplus, 0, sizeof(DataRegion) - 1))
  {
    size_t memSize = sizeof(DataRegionSet) + surplus;
    uint8_t* mem = gid_malloc(memSize);

    DataRegionSet* set = create_data_region_set_in(mem, memSize);
    assert_not_null(set);
    assert_int_eq(0, set->count);
    assert_int_eq(0, set->capacity);//Because there wasn't enough space for even one DataRegion
    assert_pointer_eq(mem + sizeof(DataRegionSet), set->regions);

    gid_free(mem);
  }

  Test(create_data_region_set_in_with_enough_space,
    RangeParam(numRegions, 0, 10)
    RangeParam(surplus, 0, sizeof(DataRegion) - 1))
  {
    size_t memSize = sizeof(DataRegionSet) + (sizeof(DataRegion) * numRegions) + surplus;
    uint8_t* mem = gid_malloc(memSize);

    DataRegionSet* set = create_data_region_set_in(mem, memSize);
    assert_not_null(set);
    assert_int_eq(0, set->count);
    assert_int_eq(numRegions, set->capacity);
    assert_pointer_eq(mem + sizeof(DataRegionSet), set->regions);

    gid_free(mem);
  }

  Test(create_data_region_set_works,
    EnumParam(capacity, 0, 1, 2, 3, 4, 10, 1024, 10240))
  {
    DataRegionSet* set = create_data_region_set(capacity);
    assert_not_null(set);
    assert_int_eq(0, set->count);
    assert_int_eq(capacity, set->capacity);
    assert_not_null(set->regions);

    //Write to all of the regions, just to ensure pointer is valid
    for(int64_t i = 0; i < capacity; i++)
      set->regions[i] = (DataRegion){ .first_index = i*5, .last_index=(i+1)*5 };

    free(set);
  }

END_TEST_SUITE()


BEGIN_TEST_SUITE(DataRegionFunctionTests);

  Test(get_data_region_length_works,
    EnumParam(offset, -10000, -100, -2, -1, 0, 1, 2, 3, 100, 100000000)
    EnumParam(len, 1, 2, 3, 4, 100, 1024))
  {
    DataRegion region = (DataRegion){.first_index = offset, .last_index = offset+(len-1)};
    assert_int_eq(len, get_data_region_length(region));
  }

  Test(does_data_region_contain_other_data_region_false_when_far_away,
    EnumParam(offset, -10000, -100, -2, -1, 0, 1, 2, 3, 100, 10000000))
  {
    DataRegion center = (DataRegion){.first_index = offset + 100, .last_index = offset + 199};
    DataRegion farLeft = (DataRegion){.first_index = offset - 1000, .last_index = offset - 900};
    DataRegion farRight = (DataRegion){.first_index = offset + 1000, .last_index = offset + 1099};

    assert(does_data_region_contain_other_data_region(center, farLeft) == 0);
    assert(does_data_region_contain_other_data_region(farLeft, center) == 0);
    assert(does_data_region_contain_other_data_region(center, farRight) == 0);
    assert(does_data_region_contain_other_data_region(farLeft, center) == 0);
  }

  Test(does_data_region_contain_other_data_region_false_when_outer_overlap,
    EnumParam(offset, -10000, -100, -2, -1, 0, 1, 2, 3, 100, 10000000)
    EnumParam(overlapPad, 1, 2, 3, 50))
  {
    DataRegion center = (DataRegion){.first_index = offset + 100, .last_index = offset + 199};
    DataRegion leftOverlap = (DataRegion){.first_index = center.first_index - overlapPad, .last_index = center.first_index + overlapPad};
    DataRegion rightOverlap = (DataRegion){.first_index = center.last_index - overlapPad, .last_index=center.last_index + overlapPad};

    assert(does_data_region_contain_other_data_region(center, leftOverlap) == 0);
    assert(does_data_region_contain_other_data_region(leftOverlap, center) == 0);
    assert(does_data_region_contain_other_data_region(center, rightOverlap) == 0);
    assert(does_data_region_contain_other_data_region(rightOverlap, center) == 0);
  }

  Test(does_data_region_contain_other_data_region_true_when_all_contained,
    EnumParam(offset, -10000, -100, -2, -1, 0, 1, 2, 3, 100, 100000000))
  {
    DataRegion outer = (DataRegion){.first_index = offset + 100, .last_index = offset + 199};
    DataRegion inner = (DataRegion){.first_index = outer.first_index + 10, .last_index = outer.last_index - 10};

    assert(does_data_region_contain_other_data_region(outer, inner) != 0);
  }

  Test(does_data_region_contain_other_data_region_false_when_proper_superset,
    EnumParam(offset, -10000, -100, -2, -1, 0, 1, 2, 3, 100, 100000000))
  {
    DataRegion outer = (DataRegion){.first_index = offset + 100, .last_index = offset + 199};
    DataRegion inner = (DataRegion){.first_index = outer.first_index + 10, .last_index = outer.last_index - 10};

    assert(does_data_region_contain_other_data_region(inner, outer) == 0);
  }

  Test(does_data_region_contain_other_data_region_when_both_equal,
    EnumParam(offset, -10000, -100, -2, -1, 0, 1, 2, 3, 100, 100000000))
  {
    DataRegion a = (DataRegion){.first_index = offset + 100, .last_index = offset + 199};
    DataRegion b = (DataRegion){.first_index = offset + 100, .last_index = offset + 199};

    assert(does_data_region_contain_other_data_region(a, b) != 0);
    assert(does_data_region_contain_other_data_region(b, a) != 0);
  }

  Test(does_data_region_contain_other_data_region_single_byte_tests,
    EnumParam(offset, -10000, -100, -2, -1, 0, 1, 2, 3, 100, 100000000))
  {
    DataRegion a = (DataRegion){.first_index = offset + 100, .last_index = offset + 100};
    DataRegion leftAdj = (DataRegion){a.first_index - 1, a.first_index - 1};
    DataRegion leftFar = (DataRegion){a.first_index - 100, a.first_index - 100};
    DataRegion rightAdj = (DataRegion){a.last_index + 1, a.last_index + 1};
    DataRegion rightFar = (DataRegion){a.last_index + 100, a.last_index + 100};

    assert(does_data_region_contain_other_data_region(a, a) != 0);
    assert(does_data_region_contain_other_data_region(a, leftAdj) == 0);
    assert(does_data_region_contain_other_data_region(a, leftFar) == 0);
    assert(does_data_region_contain_other_data_region(leftAdj, a) == 0);
    assert(does_data_region_contain_other_data_region(leftFar, a) == 0);
    assert(does_data_region_contain_other_data_region(a, rightAdj) == 0);
    assert(does_data_region_contain_other_data_region(a, rightFar) == 0);
    assert(does_data_region_contain_other_data_region(rightAdj, a) == 0);
    assert(does_data_region_contain_other_data_region(rightFar, a) == 0);
  }

  Test(do_data_regions_intersect_false_when_far,
    EnumParam(offset, -10000, -100, -2, -1, 0, 1, 2, 3, 100, 100000000))
  {
    DataRegion a = (DataRegion){.first_index = offset + 100, .last_index = offset + 199};
    DataRegion left = (DataRegion){.first_index = a.first_index - 10000, .last_index = a.first_index - 9000};
    DataRegion right = (DataRegion){.first_index = a.last_index + 10000, .last_index = a.last_index + 10099};

    assert(do_data_regions_intersect(a, left) == 0);
    assert(do_data_regions_intersect(left, a) == 0);
    assert(do_data_regions_intersect(a, right) == 0);
    assert(do_data_regions_intersect(right, a) == 0);
  }

  Test(do_data_regions_intersect_true_when_partial_overlap,
    EnumParam(offset, -10000, -100, -2, -1, 0, 1, 2, 3, 100, 100000000)
    EnumParam(overlapPad, 1, 2, 3, 50))
  {
    DataRegion inner = (DataRegion){.first_index = offset + 100, .last_index = offset + 199};
    DataRegion left = (DataRegion){.first_index = inner.first_index - overlapPad, .last_index = inner.first_index + overlapPad};
    DataRegion right = (DataRegion){.first_index = inner.last_index - overlapPad, .last_index = inner.last_index + overlapPad};

    assert(do_data_regions_intersect(inner, left) != 0);
    assert(do_data_regions_intersect(left, inner) != 0);
    assert(do_data_regions_intersect(inner, right) != 0);
    assert(do_data_regions_intersect(right, inner) != 0);
  }

  Test(do_data_regions_intersect_true_when_total_overlap,
    EnumParam(offset, -10000, -100, -2, -1, 0, 1, 2, 3, 100, 100000000))
  {
    DataRegion outer = (DataRegion){.first_index = offset + 100, .last_index = offset + 199};
    DataRegion inner = (DataRegion){outer.first_index + 10, outer.last_index - 10};

    assert(do_data_regions_intersect(outer, inner) != 0);
    assert(do_data_regions_intersect(inner, outer) != 0);
  }

  Test(do_data_regions_intersect_true_when_equal,
    EnumParam(offset, -10000, -100, -2, -1, 0, 1, 2, 3, 100, 100000000))
  {
    DataRegion a = (DataRegion){.first_index = offset + 100, .last_index = offset + 199};
    DataRegion b = (DataRegion){.first_index = a.first_index, .last_index = a.last_index};

    assert(do_data_regions_intersect(a, b) != 0);
    assert(do_data_regions_intersect(b, a) != 0);
  }

  Test(are_data_regions_adjacent_false_when_far,
    EnumParam(offset, -10000, -100, -2, -1, 0, 1, 2, 3, 100, 100000000))
  {
    DataRegion a = (DataRegion){.first_index = offset + 100, .last_index = offset + 199};
    DataRegion left = (DataRegion){.first_index = a.first_index - 1000, .last_index = a.first_index - 900};
    DataRegion right = (DataRegion){.first_index = a.last_index + 1000, .last_index = a.last_index + 1900};

    assert(are_data_regions_adjacent(a, left) == 0);
    assert(are_data_regions_adjacent(left, a) == 0);
    assert(are_data_regions_adjacent(a, right) == 0);
    assert(are_data_regions_adjacent(right, a) == 0);
  }

  Test(are_data_regions_adjacent_false_when_overlap,
    EnumParam(offset, -10000, -100, -2, -1, 0, 1, 2, 3, 100, 100000000))
  {
    DataRegion inner = (DataRegion){.first_index = offset + 100, .last_index = offset + 199};
    DataRegion leftOverlap = (DataRegion){.first_index = inner.first_index - 10, .last_index = inner.first_index + 10};
    DataRegion rightOverlap = (DataRegion){.first_index = inner.last_index - 10, .last_index = inner.last_index + 10};
    DataRegion leftInner = (DataRegion){.first_index = inner.first_index+1, .last_index = inner.first_index + 5};
    DataRegion rightInner = (DataRegion){.first_index = inner.last_index - 5, .last_index = inner.last_index - 1};

    assert(are_data_regions_adjacent(inner, leftOverlap) == 0);
    assert(are_data_regions_adjacent(leftOverlap, inner) == 0);
    assert(are_data_regions_adjacent(inner, rightOverlap) == 0);
    assert(are_data_regions_adjacent(rightOverlap, inner) == 0);
    assert(are_data_regions_adjacent(inner, leftInner) == 0);
    assert(are_data_regions_adjacent(leftInner, inner) == 0);
    assert(are_data_regions_adjacent(inner, rightInner) == 0);
    assert(are_data_regions_adjacent(rightInner, inner) == 0);
  }

  Test(are_data_regions_adjacent_false_when_both_equal,
    EnumParam(offset, -10000, -100, -2, -1, 0, 1, 2, 3, 100, 100000000))
  {
    DataRegion a = (DataRegion){.first_index = offset + 100, .last_index = offset + 199};
    DataRegion b = (DataRegion){.first_index = offset + 100, .last_index = offset + 199};

    assert(are_data_regions_adjacent(a, b) == 0);
    assert(are_data_regions_adjacent(b, a) == 0);
  }

  Test(are_data_regions_adjacent_true_when_adjacent,
    EnumParam(offset, -10000, -100, -2, -1, 0, 1, 2, 3, 100, 100000000)
    EnumParam(adjLen, 1, 2, 3, 100))
  {
    DataRegion inner = (DataRegion){.first_index = offset + 100, .last_index = offset + 199};
    DataRegion left = (DataRegion){.first_index = inner.first_index - adjLen, .last_index = inner.first_index - 1};
    DataRegion right = (DataRegion){.first_index = inner.last_index + 1, .last_index = inner.last_index + adjLen};

    assert(are_data_regions_adjacent(inner, left) != 0);
    assert(are_data_regions_adjacent(left, inner) != 0);
    assert(are_data_regions_adjacent(inner, right) != 0);
    assert(are_data_regions_adjacent(right, inner) != 0);
  }

  Test(can_combine_data_regions_false_when_far,
    EnumParam(offset, -10000, -100, -2, -1, 0, 1, 2, 3, 100, 100000000))
  {
    DataRegion inner = (DataRegion){.first_index = offset + 100, .last_index = offset + 199};
    DataRegion left = (DataRegion){.first_index = inner.first_index - 1000, .last_index = inner.first_index - 900};
    DataRegion right = (DataRegion){.first_index = inner.last_index + 1000, .last_index = inner.last_index + 1900};

    assert(can_combine_data_regions(inner, left) == 0);
    assert(can_combine_data_regions(left, inner) == 0);
    assert(can_combine_data_regions(inner, right) == 0);
    assert(can_combine_data_regions(right, inner) == 0);
  }

  Test(can_combine_data_regions_true_when_partial_overlap,
    EnumParam(offset, -10000, -100, -2, -1, 0, 1, 2, 3, 100, 100000000))
  {
    DataRegion inner = (DataRegion){.first_index = offset + 100, .last_index = offset + 199};
    DataRegion left = (DataRegion){.first_index = inner.first_index - 10, .last_index = inner.first_index + 10};
    DataRegion right = (DataRegion){.first_index = inner.last_index - 10, .last_index = inner.last_index + 10};

    assert(can_combine_data_regions(inner, left) != 0);
    assert(can_combine_data_regions(left, inner) != 0);
    assert(can_combine_data_regions(inner, right) != 0);
    assert(can_combine_data_regions(right, inner) != 0);

    DataRegion innerLeftCombo = combine_data_regions(inner, left);
    DataRegion leftInnerCombo = combine_data_regions(left, inner);
    DataRegion expInnerLeftCombo = (DataRegion){.first_index = left.first_index, .last_index=inner.last_index};
    assert_memory_eq(&expInnerLeftCombo, &innerLeftCombo, sizeof(DataRegion));
    assert_memory_eq(&expInnerLeftCombo, &leftInnerCombo, sizeof(DataRegion));

    DataRegion innerRightCombo = combine_data_regions(inner, right);
    DataRegion rightInnerCombo = combine_data_regions(right, inner);
    DataRegion expInnerRightCombo = (DataRegion){.first_index = inner.first_index, .last_index = right.last_index};
    assert_memory_eq(&expInnerRightCombo, &innerRightCombo, sizeof(DataRegion));
    assert_memory_eq(&expInnerRightCombo, &rightInnerCombo, sizeof(DataRegion));
  }

  Test(can_combine_data_regions_true_when_total_overlap,
    EnumParam(offset, -10000, -100, -2, -1, 0, 1, 2, 3, 100, 100000000))
  {
    DataRegion a = (DataRegion){.first_index = offset + 100, .last_index = offset + 199};
    DataRegion inner = (DataRegion){.first_index = a.first_index + 10, .last_index = a.last_index - 10};
    DataRegion innerLeft = (DataRegion){.first_index = a.first_index + 1, .last_index = a.first_index + 10};
    DataRegion innerLeftLarge = (DataRegion){.first_index = a.first_index + 1, .last_index = a.last_index + 1000};
    DataRegion innerRight = (DataRegion){.first_index = a.last_index - 10, a.last_index - 1};
    DataRegion innerRightLarge = (DataRegion){.first_index = a.first_index - 1000, .last_index = a.last_index - 1};

    assert(can_combine_data_regions(a, inner));
    assert(can_combine_data_regions(inner, a));
    assert(can_combine_data_regions(a, innerLeft));
    assert(can_combine_data_regions(innerLeft, a));
    assert(can_combine_data_regions(a, innerLeftLarge));
    assert(can_combine_data_regions(innerLeftLarge, a));

    assert(can_combine_data_regions(a, innerRight));
    assert(can_combine_data_regions(innerRight, a));
    assert(can_combine_data_regions(a, innerRightLarge));
    assert(can_combine_data_regions(innerRightLarge, a));

    DataRegion a_inner = combine_data_regions(a, inner);
    DataRegion inner_a = combine_data_regions(inner, a);
    DataRegion a_innerLeft = combine_data_regions(a, innerLeft);
    DataRegion innerLeft_a = combine_data_regions(innerLeft, a);
    DataRegion a_innerLeftLarge = combine_data_regions(a, innerLeftLarge);
    DataRegion innerLeftLarge_a = combine_data_regions(innerLeftLarge, a);

    DataRegion a_innerRight = combine_data_regions(a, innerRight);
    DataRegion innerRight_a = combine_data_regions(innerRight, a);
    DataRegion a_innerRightLarge = combine_data_regions(a, innerRightLarge);
    DataRegion innerRightLarge_a = combine_data_regions(innerRightLarge, a);

    //Expect a_inner, a_innerLeft, and a_innerRight to all combine to just 'a'
    assert_memory_eq(&a, &a_inner, sizeof(DataRegion));
    assert_memory_eq(&a, &inner_a, sizeof(DataRegion));
    assert_memory_eq(&a, &a_innerLeft, sizeof(DataRegion));
    assert_memory_eq(&a, &innerLeft_a, sizeof(DataRegion));
    assert_memory_eq(&a, &a_innerRight, sizeof(DataRegion));
    assert_memory_eq(&a, &innerRight_a, sizeof(DataRegion));

    //Expect a_innerLeftLarge and a_innerRightLarge to combine to 'max bounds'
    DataRegion exp_a_innerLeftLarge = (DataRegion){.first_index = a.first_index, .last_index = innerLeftLarge.last_index};
    DataRegion exp_a_innerRightLarge = (DataRegion){.first_index = innerRightLarge.first_index, .last_index = a.last_index};
    assert_memory_eq(&exp_a_innerLeftLarge, &a_innerLeftLarge, sizeof(DataRegion));
    assert_memory_eq(&exp_a_innerLeftLarge, &innerLeftLarge_a, sizeof(DataRegion));
    assert_memory_eq(&exp_a_innerRightLarge, &a_innerRightLarge, sizeof(DataRegion));
    assert_memory_eq(&exp_a_innerRightLarge, &innerRightLarge_a, sizeof(DataRegion));
  }

  Test(can_combine_data_regions_true_when_equivalent,
    EnumParam(offset, -10000, -100, -2, -1, 0, 1, 2, 3, 100, 100000000))
  {
    DataRegion a = (DataRegion){.first_index = offset + 100, .last_index = offset + 199};
    DataRegion b = (DataRegion){.first_index = offset + 100, .last_index = offset + 199};

    assert(can_combine_data_regions(a, b));
    assert(can_combine_data_regions(b, a));

    DataRegion ab = combine_data_regions(a, b);
    DataRegion ba = combine_data_regions(b, a);
    assert_memory_eq(&a, &ab, sizeof(DataRegion));
    assert_memory_eq(&a, &ba, sizeof(DataRegion));
  }

  Test(is_data_region_valid_false_when_order_is_wrong,
    EnumParam(offset, -10000, -100, -2, -1, 0, 1, 2, 3, 100, 100000000)
    EnumParam(distance, 1, 2, 3, 10))
  {
    DataRegion invalid = (DataRegion){.first_index = offset + distance, .last_index = offset};
    assert(is_data_region_valid(invalid) == 0);
  }

  Test(is_data_region_valid_true_when_first_equals_last,
    EnumParam(offset, -10000, -100, -2, -1, 0, 1, 2, 3, 100, 100000000))
  {
    DataRegion region = (DataRegion){.first_index = offset, .last_index = offset};
    assert(is_data_region_valid(region) != 0);
  }

  Test(is_data_region_valid_true_when_first_less_than_last,
    EnumParam(offset, -10000, -100, -2, -1, 0, 1, 2, 3, 100, 100000000))
  {
    DataRegion region = (DataRegion){.first_index = offset, .last_index = offset + 100};
    assert(is_data_region_valid(region) != 0);
  }

END_TEST_SUITE()


BEGIN_TEST_SUITE(DataRegionSetFunctionTests)

  Test(get_data_region_set_total_length_zero_when_NULL)
  {
    assert_int_eq(0, get_data_region_set_total_length(NULL));
  }

  Test(get_data_region_set_total_length_zero_after_initialization)
  {
    const int capacity = 100;
    DataRegionSet* set = create_test_data_region_set(capacity, 0);
    assert_int_eq(0, set->count);
    assert_int_eq(capacity, set->capacity);
    free_test_data_region_set(set);
  }

  Test(get_data_region_set_total_length_after_one_add)
  {
    const int capacity = 100;
    DataRegionSet* set = create_test_data_region_set(capacity, 0);
    DataRegion toAdd = (DataRegion){.first_index = 100, .last_index = 199};
    assert(add_data_region(set, toAdd) == DATA_REGION_SET_SUCCESS);
    assert_int_eq(100, get_data_region_set_total_length(set));

    free_test_data_region_set(set);
  }

  Test(get_data_region_set_total_length_after_multi_add)
  {
    const int capacity = 100;
    DataRegionSet* set = create_test_data_region_set(capacity, 0);

    assert(add_data_region(set, (DataRegion){.first_index = 0, .last_index = 9}) == DATA_REGION_SET_SUCCESS);
    assert(add_data_region(set, (DataRegion){.first_index = 10, .last_index = 19}) == DATA_REGION_SET_SUCCESS);
    assert_int_eq(20, get_data_region_set_total_length(set));

    assert(add_data_region(set, (DataRegion){.first_index = 100, .last_index = 100}) == DATA_REGION_SET_SUCCESS);
    assert_int_eq(21, get_data_region_set_total_length(set));

    assert(add_data_region(set, (DataRegion){.first_index = 1000, .last_index = 1999}) == DATA_REGION_SET_SUCCESS);
    assert_int_eq(1021, get_data_region_set_total_length(set));

    free_test_data_region_set(set);
  }

  Test(get_data_region_set_total_length_after_add_and_remove)
  {
    const int capacity = 100;
    DataRegionSet* set = create_test_data_region_set(capacity, 0);

    assert(add_data_region(set, (DataRegion){.first_index = 0, .last_index = 9}) == DATA_REGION_SET_SUCCESS);
    assert(add_data_region(set, (DataRegion){.first_index = 10, .last_index = 19}) == DATA_REGION_SET_SUCCESS);
    assert(add_data_region(set, (DataRegion){.first_index = 100, .last_index = 100}) == DATA_REGION_SET_SUCCESS);
    assert(add_data_region(set, (DataRegion){.first_index = 1000, .last_index = 1999}) == DATA_REGION_SET_SUCCESS);

    assert(remove_data_region(set, (DataRegion){.first_index = 1, .last_index = 1}) == DATA_REGION_SET_SUCCESS);
    assert_int_eq(1020, get_data_region_set_total_length(set));

    assert(remove_data_region(set, (DataRegion){.first_index = 0, .last_index = 4}) == DATA_REGION_SET_SUCCESS);
    assert_int_eq(1016, get_data_region_set_total_length(set));

    assert(remove_data_region(set, (DataRegion){.first_index = 0, .last_index = 4}) == DATA_REGION_SET_SUCCESS);
    assert_int_eq(1016, get_data_region_set_total_length(set));

    assert(remove_data_region(set, (DataRegion){.first_index = 0, .last_index = 1099}) == DATA_REGION_SET_SUCCESS);
    assert_int_eq(900, get_data_region_set_total_length(set));

    free_test_data_region_set(set);
  }

  Test(internal_remove_data_region_at_single_region)
  {
    const int capacity = 100;
    DataRegionSet* set = create_test_data_region_set(capacity, 0);
    assert(add_data_region(set, (DataRegion){.first_index = 0, .last_index = 9}) == DATA_REGION_SET_SUCCESS);
    assert_int_eq(1, set->count);
    internal_remove_data_region_at(set, 0);
    assert_int_eq(0, set->count);
    assert_int_eq(capacity, set->capacity);
    assert_int_eq(0, get_data_region_set_total_length(set));

    free_test_data_region_set(set);
  }

  Test(internal_remove_data_region_at_first_region_of_many)
  {
    const int capacity = 100;
    DataRegionSet* set = create_test_data_region_set(capacity, 0);

    DataRegion a = (DataRegion){.first_index = 0, .last_index = 1};
    DataRegion b = (DataRegion){.first_index = 10, .last_index = 19};
    DataRegion c = (DataRegion){.first_index = 100, .last_index = 199};

    //Add three regions
    assert(add_data_region(set, a) == DATA_REGION_SET_SUCCESS);
    assert(add_data_region(set, b) == DATA_REGION_SET_SUCCESS);
    assert(add_data_region(set, c) == DATA_REGION_SET_SUCCESS);

    //Remove the first one
    internal_remove_data_region_at(set, 0);
    assert_int_eq(2, set->count);
    assert_int_eq(capacity, set->capacity);
    assert_int_eq(10+100, get_data_region_set_total_length(set));
    assert_memory_eq(&b, &set->regions[0], sizeof(DataRegion));
    assert_memory_eq(&c, &set->regions[1], sizeof(DataRegion));

    free_test_data_region_set(set);
  }

  Test(internal_remove_data_region_at_middle_of_many)
  {
    const int capacity = 100;
    DataRegionSet* set = create_test_data_region_set(capacity, 0);

    DataRegion a = (DataRegion){.first_index = 0, .last_index = 1};
    DataRegion b = (DataRegion){.first_index = 10, .last_index = 19};
    DataRegion c = (DataRegion){.first_index = 100, .last_index = 199};
    DataRegion d = (DataRegion){.first_index = 500, .last_index = 599};
    DataRegion e = (DataRegion){.first_index = 1000, .last_index = 1999};

    //Add 5 regions
    assert(add_data_region(set, a) == DATA_REGION_SET_SUCCESS);
    assert(add_data_region(set, b) == DATA_REGION_SET_SUCCESS);
    assert(add_data_region(set, c) == DATA_REGION_SET_SUCCESS);
    assert(add_data_region(set, d) == DATA_REGION_SET_SUCCESS);
    assert(add_data_region(set, e) == DATA_REGION_SET_SUCCESS);

    //Remove the middle one (c)
    internal_remove_data_region_at(set, 2);
    assert_int_eq(4, set->count);
    assert_int_eq(capacity, set->capacity);
    assert_int_eq(2+10+100+1000, get_data_region_set_total_length(set));
    assert_memory_eq(&a, &set->regions[0], sizeof(DataRegion));
    assert_memory_eq(&b, &set->regions[1], sizeof(DataRegion));
    assert_memory_eq(&d, &set->regions[2], sizeof(DataRegion));
    assert_memory_eq(&e, &set->regions[3], sizeof(DataRegion));

    free_test_data_region_set(set);
  }

  Test(internal_remove_data_region_at_end_of_many)
  {
    const int capacity = 100;
    DataRegionSet* set = create_test_data_region_set(capacity, 0);

    DataRegion a = (DataRegion){.first_index = 0, .last_index = 1};
    DataRegion b = (DataRegion){.first_index = 10, .last_index = 19};
    DataRegion c = (DataRegion){.first_index = 100, .last_index = 199};
    DataRegion d = (DataRegion){.first_index = 500, .last_index = 599};
    DataRegion e = (DataRegion){.first_index = 1000, .last_index = 1999};

    //Add 5 regions
    assert(add_data_region(set, a) == DATA_REGION_SET_SUCCESS);
    assert(add_data_region(set, b) == DATA_REGION_SET_SUCCESS);
    assert(add_data_region(set, c) == DATA_REGION_SET_SUCCESS);
    assert(add_data_region(set, d) == DATA_REGION_SET_SUCCESS);
    assert(add_data_region(set, e) == DATA_REGION_SET_SUCCESS);

    //Remove the last one (e)
    internal_remove_data_region_at(set, 4);
    assert_int_eq(4, set->count);
    assert_int_eq(capacity, set->capacity);
    assert_int_eq(2+10+100+100, get_data_region_set_total_length(set));
    assert_memory_eq(&a, &set->regions[0], sizeof(DataRegion));
    assert_memory_eq(&b, &set->regions[1], sizeof(DataRegion));
    assert_memory_eq(&c, &set->regions[2], sizeof(DataRegion));
    assert_memory_eq(&d, &set->regions[3], sizeof(DataRegion));

    free_test_data_region_set(set);
  }

  Test(insert_data_region_at_empty)
  {
    const int capacity = 100;
    DataRegionSet* set = create_test_data_region_set(capacity, 0);

    DataRegion toInsert = (DataRegion){.first_index = 1, .last_index = 5};
    internal_insert_data_region_at(set, toInsert, 0);
    assert_int_eq(1, set->count);
    assert_int_eq(capacity, set->capacity);
    assert_memory_eq(&toInsert, &set->regions[0], sizeof(DataRegion));
    assert_int_eq(5, get_data_region_set_total_length(set));

    free_test_data_region_set(set);
  }

  Test(insert_data_region_at_before_single_region)
  {
    const int capacity = 100;
    DataRegionSet* set = create_test_data_region_set(capacity, 0);

    DataRegion existing = (DataRegion){.first_index = 10, .last_index = 19};
    internal_insert_data_region_at(set, existing, 0);

    DataRegion toInsert = (DataRegion){.first_index = 100, .last_index = 199};
    internal_insert_data_region_at(set, toInsert, 0);
    assert_int_eq(2, set->count);
    assert_int_eq(capacity, set->capacity);
    assert_memory_eq(&toInsert, &set->regions[0], sizeof(DataRegion));
    assert_memory_eq(&existing, &set->regions[1], sizeof(DataRegion));

    free_test_data_region_set(set);
  }

  Test(insert_data_region_at_after_single_region)
  {
    const int capacity = 100;
    DataRegionSet* set = create_test_data_region_set(capacity, 0);

    DataRegion existing = (DataRegion){.first_index = 10, .last_index = 19};
    internal_insert_data_region_at(set, existing, 0);

    DataRegion toInsert = (DataRegion){.first_index = 100, .last_index = 199};
    internal_insert_data_region_at(set, toInsert, 1);
    assert_int_eq(2, set->count);
    assert_int_eq(capacity, set->capacity);
    assert_memory_eq(&existing, &set->regions[0], sizeof(DataRegion));
    assert_memory_eq(&toInsert, &set->regions[1], sizeof(DataRegion));

    free_test_data_region_set(set);
  }

  Test(insert_data_region_at_before_many_regions)
  {
    const int capacity = 100;
    DataRegionSet* set = create_test_data_region_set(capacity, 0);

    DataRegion a = (DataRegion){.first_index = 0, .last_index = 9};
    DataRegion b = (DataRegion){.first_index = 20, .last_index = 29};
    DataRegion c = (DataRegion){.first_index = 40, .last_index = 49};
    DataRegion d = (DataRegion){.first_index = 60, .last_index = 69};
    DataRegion e = (DataRegion){.first_index = 80, .last_index = 89};

    internal_insert_data_region_at(set, a, 0);
    internal_insert_data_region_at(set, b, 0);
    internal_insert_data_region_at(set, c, 0);
    internal_insert_data_region_at(set, d, 0);
    internal_insert_data_region_at(set, e, 0);

    assert_int_eq(5, set->count);
    assert_int_eq(capacity, set->capacity);
    assert_int_eq(10+10+10+10+10, get_data_region_set_total_length(set));
    assert_memory_eq(&e, &set->regions[0], sizeof(DataRegion));
    assert_memory_eq(&d, &set->regions[1], sizeof(DataRegion));
    assert_memory_eq(&c, &set->regions[2], sizeof(DataRegion));
    assert_memory_eq(&b, &set->regions[3], sizeof(DataRegion));
    assert_memory_eq(&a, &set->regions[4], sizeof(DataRegion));

    free_test_data_region_set(set);
  }

  Test(insert_data_region_at_between_many_regions)
  {
    const int capacity = 100;
    DataRegionSet* set = create_test_data_region_set(capacity, 0);

    DataRegion a = (DataRegion){.first_index = 0, .last_index = 9};
    DataRegion b = (DataRegion){.first_index = 20, .last_index = 29};
    DataRegion c = (DataRegion){.first_index = 40, .last_index = 49};
    DataRegion d = (DataRegion){.first_index = 60, .last_index = 69};
    DataRegion e = (DataRegion){.first_index = 80, .last_index = 89};

    internal_insert_data_region_at(set, a, 0);
    internal_insert_data_region_at(set, b, 0);
    internal_insert_data_region_at(set, d, 0);
    internal_insert_data_region_at(set, e, 0);

    internal_insert_data_region_at(set, c, 2);

    assert_int_eq(5, set->count);
    assert_int_eq(capacity, set->capacity);
    assert_int_eq(10+10+10+10+10, get_data_region_set_total_length(set));
    assert_memory_eq(&e, &set->regions[0], sizeof(DataRegion));
    assert_memory_eq(&d, &set->regions[1], sizeof(DataRegion));
    assert_memory_eq(&c, &set->regions[2], sizeof(DataRegion));
    assert_memory_eq(&b, &set->regions[3], sizeof(DataRegion));
    assert_memory_eq(&a, &set->regions[4], sizeof(DataRegion));

    free_test_data_region_set(set);
  }

  Test(insert_data_region_at_after_many_regions)
  {
    const int capacity = 100;
    DataRegionSet* set = create_test_data_region_set(capacity, 0);

    DataRegion a = (DataRegion){.first_index = 0, .last_index = 9};
    DataRegion b = (DataRegion){.first_index = 20, .last_index = 29};
    DataRegion c = (DataRegion){.first_index = 40, .last_index = 49};
    DataRegion d = (DataRegion){.first_index = 60, .last_index = 69};
    DataRegion e = (DataRegion){.first_index = 80, .last_index = 89};

    internal_insert_data_region_at(set, a, 0);
    internal_insert_data_region_at(set, b, 1);
    internal_insert_data_region_at(set, c, 2);
    internal_insert_data_region_at(set, d, 3);
    internal_insert_data_region_at(set, e, 4);

    assert_int_eq(5, set->count);
    assert_int_eq(capacity, set->capacity);
    assert_int_eq(10+10+10+10+10, get_data_region_set_total_length(set));
    assert_memory_eq(&a, &set->regions[0], sizeof(DataRegion));
    assert_memory_eq(&b, &set->regions[1], sizeof(DataRegion));
    assert_memory_eq(&c, &set->regions[2], sizeof(DataRegion));
    assert_memory_eq(&d, &set->regions[3], sizeof(DataRegion));
    assert_memory_eq(&e, &set->regions[4], sizeof(DataRegion));

    free_test_data_region_set(set);
  }

END_TEST_SUITE()

BEGIN_TEST_SUITE(DataRegionSetAddTests)

  Test(add_data_region_when_dst_is_NULL)
  {
    DataRegion toAdd = (DataRegion){.first_index = 1, .last_index = 7};
    assert_int_eq(DATA_REGION_SET_NULL_ARG, add_data_region(NULL, toAdd));
  }

  Test(add_data_region_when_region_is_invalid,
    EnumParam(initCount, 0, 1, 2, 3, 100))
  {
    const int capacity = 100;
    DataRegionSet* set = create_test_data_region_set(capacity, initCount);
    DataRegionSet* clone = clone_data_region_set(set);

    //Try to add an invalid DataRegion
    DataRegion invalid = (DataRegion){.first_index = 7, .last_index = 1};
    assert_int_eq(DATA_REGION_SET_INVALID_REGION, add_data_region(set, invalid));

    //Check that the set wasn't affected by the failed add
    assert_data_region_set_eq(clone, set);

    free_test_data_region_set(set);
    free_clone_data_region_set(clone);
  }

  Test(add_data_region_when_dst_is_empty,
    EnumParam(capacity, 1, 2, 3, 1000))
  {
    DataRegionSet* set = create_test_data_region_set(capacity, 0);

    DataRegion toAdd = (DataRegion){.first_index = 1, .last_index = 199};
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, toAdd));

    assert_int_eq(capacity, set->capacity);
    assert_int_eq(1, set->count);
    assert_int_eq(199, get_data_region_set_total_length(set));
    assert_memory_eq(&toAdd, &set->regions[0], sizeof(DataRegion));

    free_test_data_region_set(set);
  }

  Test(add_data_region_far_from_single_existing,
    EnumParam(capacity, 2, 3, 1000)
    EnumParam(distance, -10000, -500, 500, 10000))
  {
    DataRegionSet* set = create_test_data_region_set(capacity, 0);
    DataRegion existing = (DataRegion){.first_index = 100, .last_index = 199};
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existing));

    DataRegion toAdd = (DataRegion){.first_index = distance, .last_index = distance+74};
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, toAdd));

    assert_int_eq(capacity, set->capacity);
    assert_int_eq(2, set->count);
    assert_int_eq(100+75, get_data_region_set_total_length(set));

    //Check that regions are added in ascending order
    if(toAdd.last_index < existing.last_index)
    {
      assert_memory_eq(&toAdd, &set->regions[0], sizeof(DataRegion));
      assert_memory_eq(&existing, &set->regions[1], sizeof(DataRegion));
    }
    else
    {
      assert_memory_eq(&existing, &set->regions[0], sizeof(DataRegion));
      assert_memory_eq(&toAdd, &set->regions[1], sizeof(DataRegion));
    }

    free_test_data_region_set(set);
  }

  Test(add_data_region_adjacent_to_single_existing,
    EnumParam(capacity, 1, 2, 3, 1000)
    EnumParam(toLeft, 0, 1)
    EnumParam(size, 1, 2, 3, 100))
  {
    DataRegionSet* set = create_test_data_region_set(capacity, 0);
    DataRegion existing = (DataRegion){.first_index = 100, .last_index = 199};
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existing));

    DataRegion toAdd =
      toLeft ? (DataRegion){.first_index = existing.first_index - size, .last_index = existing.first_index - 1}
             : (DataRegion){.first_index = existing.last_index + 1, .last_index = existing.last_index + size};

    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, toAdd));

    assert_int_eq(capacity, set->capacity);
    assert_int_eq(1, set->count);//1 because 'toAdd' was combined with 'existing'
    assert_int_eq(100+size, get_data_region_set_total_length(set));

    DataRegion expect =
      toLeft ? (DataRegion){.first_index = toAdd.first_index, .last_index = existing.last_index}
             : (DataRegion){.first_index = existing.first_index, .last_index = toAdd.last_index};
    assert_memory_eq(&expect, &set->regions[0], sizeof(DataRegion));

    free_test_data_region_set(set);
  }

  Test(add_data_region_partially_overlapping_single_existing,
    EnumParam(capacity, 1, 2, 3, 1000)
    EnumParam(toLeft, 0, 1))
  {
    DataRegionSet* set = create_test_data_region_set(capacity, 0);
    DataRegion existing = (DataRegion){.first_index = 100, .last_index = 199};
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existing));

    DataRegion toAdd =
      toLeft ? (DataRegion){.first_index = existing.first_index - 10, .last_index = existing.first_index + 10}
             : (DataRegion){.first_index = existing.last_index - 10, .last_index = existing.last_index + 10};

    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, toAdd));

    assert_int_eq(capacity, set->capacity);
    assert_int_eq(1, set->count);//1 because 'toAdd' was combined with 'existing'
    assert_int_eq(100+10, get_data_region_set_total_length(set));

    DataRegion expect =
      toLeft ? (DataRegion){.first_index = toAdd.first_index, .last_index = existing.last_index}
             : (DataRegion){.first_index = existing.first_index, .last_index = toAdd.last_index};
    assert_memory_eq(&expect, &set->regions[0], sizeof(DataRegion));

    free_test_data_region_set(set);
  }

  Test(add_data_region_totally_overlapped_by_single_existing,
    EnumParam(capacity, 1, 2, 3, 1000))
  {
    DataRegionSet* set = create_test_data_region_set(capacity, 0);
    DataRegion existing = (DataRegion){.first_index = 100, .last_index = 199};
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existing));

    DataRegion toAdd = (DataRegion){.first_index = 150, .last_index = 159};
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, toAdd));

    assert_int_eq(capacity, set->capacity);
    assert_int_eq(1, set->count);//1 because 'toAdd' was combined with 'existing'
    assert_int_eq(100, get_data_region_set_total_length(set));
    assert_memory_eq(&existing, &set->regions[0], sizeof(DataRegion));

    free_test_data_region_set(set);
  }

  Test(add_data_region_totally_overlapping_single_existing,
    EnumParam(capacity, 1, 2, 3, 1000))
  {
    DataRegionSet* set = create_test_data_region_set(capacity, 0);
    DataRegion existing = (DataRegion){.first_index = 100, .last_index = 199};
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existing));

    DataRegion toAdd = (DataRegion){.first_index = -1000, .last_index = 999};
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, toAdd));

    assert_int_eq(capacity, set->capacity);
    assert_int_eq(1, set->count);//1 because 'toAdd' was combined with 'existing'
    assert_int_eq(2000, get_data_region_set_total_length(set));
    assert_memory_eq(&toAdd, &set->regions[0], sizeof(DataRegion));

    free_test_data_region_set(set);
  }

  Test(add_data_region_equal_to_single_existing,
    EnumParam(capacity, 1, 2, 3, 1000))
  {
    DataRegionSet* set = create_test_data_region_set(capacity, 0);
    DataRegion existing = (DataRegion){.first_index = 100, .last_index = 199};
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existing));

    DataRegion toAdd = (DataRegion){.first_index = 100, .last_index = 199};
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, toAdd));

    assert_int_eq(capacity, set->capacity);
    assert_int_eq(1, set->count);//Because 'toAdd' was combined with 'existing'
    assert_int_eq(100, get_data_region_set_total_length(set));
    assert_memory_eq(&existing, &set->regions[0], sizeof(DataRegion));

    free_test_data_region_set(set);
  }

  Test(add_data_region_when_full_works_if_combined)
  {
    DataRegionSet* set = create_test_data_region_set(1, 0);
    DataRegion existing = (DataRegion){.first_index = 100, .last_index = 199};
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existing));

    DataRegion toAdd = (DataRegion){.first_index = 90, .last_index = 209};
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, toAdd));

    assert_int_eq(1, set->capacity);
    assert_int_eq(1, set->count);
    assert_int_eq(120, get_data_region_set_total_length(set));
    assert_memory_eq(&toAdd, &set->regions[0], sizeof(DataRegion));

    free_test_data_region_set(set);
  }

  Test(add_data_region_far_from_two_existing,
    EnumParam(capacity, 3, 4, 5, 1000)
    EnumParam(toLeft, 0, 1))
  {
    DataRegionSet* set = create_test_data_region_set(capacity, 0);
    DataRegion existingA = (DataRegion){.first_index = 100, .last_index = 199};
    DataRegion existingB = (DataRegion){.first_index = 300, .last_index = 399};
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingA));
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingB));

    DataRegion toAdd =
      toLeft ? (DataRegion){.first_index = -10999, .last_index = -10000}
             : (DataRegion){.first_index = 10000, .last_index = 10999 };
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, toAdd));

    assert_int_eq(capacity, set->capacity);
    assert_int_eq(3, set->count);
    assert_int_eq(100+100+1000, get_data_region_set_total_length(set));

    if(toLeft)
    {
      assert_memory_eq(&toAdd, &set->regions[0], sizeof(DataRegion));
      assert_memory_eq(&existingA, &set->regions[1], sizeof(DataRegion));
      assert_memory_eq(&existingB, &set->regions[2], sizeof(DataRegion));
    }
    else
    {
      assert_memory_eq(&existingA, &set->regions[0], sizeof(DataRegion));
      assert_memory_eq(&existingB, &set->regions[1], sizeof(DataRegion));
      assert_memory_eq(&toAdd, &set->regions[2], sizeof(DataRegion));
    }

    free_test_data_region_set(set);
  }

  Test(add_data_region_adjacent_to_one_of_two_existing,
    EnumParam(capacity, 2, 3, 4, 5, 1000)
    EnumParam(relativeIndex, 0, 1)
    EnumParam(toLeft, 0, 1))
  {
    DataRegionSet* set = create_test_data_region_set(capacity, 0);
    DataRegion existingA = (DataRegion){.first_index = 100, .last_index = 199};
    DataRegion existingB = (DataRegion){.first_index = 700, .last_index = 799};
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingA));
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingB));

    DataRegion relative = relativeIndex == 0 ? existingA : existingB;
    DataRegion toAdd =
      toLeft ? (DataRegion){.first_index = relative.first_index - 100, .last_index = relative.first_index - 1}
             : (DataRegion){.first_index = relative.last_index + 1, .last_index = relative.last_index + 100};

    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, toAdd));

    assert_int_eq(capacity, set->capacity);
    assert_int_eq(2, set->count);
    assert_int_eq(100+100+100, get_data_region_set_total_length(set));

    if(relativeIndex == 0)
    {
      if(toLeft)
      {
        DataRegion combo = (DataRegion){.first_index = toAdd.first_index, .last_index = existingA.last_index};
        assert_memory_eq(&combo, &set->regions[0], sizeof(DataRegion));
        assert_memory_eq(&existingB, &set->regions[1], sizeof(DataRegion));
      }
      else
      {
        DataRegion combo = (DataRegion){.first_index = existingA.first_index, .last_index = toAdd.last_index};
        assert_memory_eq(&combo, &set->regions[0], sizeof(DataRegion));
        assert_memory_eq(&existingB, &set->regions[1], sizeof(DataRegion));
      }
    }
    else
    {
      if(toLeft)
      {
        DataRegion combo = (DataRegion){.first_index = toAdd.first_index, .last_index = existingB.last_index};
        assert_memory_eq(&existingA, &set->regions[0], sizeof(DataRegion));
        assert_memory_eq(&combo, &set->regions[1], sizeof(DataRegion));
      }
      else
      {
        DataRegion combo = (DataRegion){.first_index = existingB.first_index, .last_index = toAdd.last_index};
        assert_memory_eq(&existingA, &set->regions[0], sizeof(DataRegion));
        assert_memory_eq(&combo, &set->regions[1], sizeof(DataRegion));
      }
    }

    free_test_data_region_set(set);
  }

  Test(add_data_region_between_two_existing_non_adjacent,
    EnumParam(capacity, 3, 4, 5, 1000))
  {
    DataRegionSet* set = create_test_data_region_set(capacity, 0);
    DataRegion existingA = (DataRegion){.first_index = 100, .last_index = 199};
    DataRegion existingB = (DataRegion){.first_index = 700, .last_index = 799};
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingA));
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingB));

    DataRegion toAdd = (DataRegion){.first_index = 300, .last_index = 399};
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, toAdd));

    assert_int_eq(capacity, set->capacity);
    assert_int_eq(3, set->count);
    assert_memory_eq(&existingA, &set->regions[0], sizeof(DataRegion));
    assert_memory_eq(&toAdd, &set->regions[1], sizeof(DataRegion));
    assert_memory_eq(&existingB, &set->regions[2], sizeof(DataRegion));

    free_test_data_region_set(set);
  }

  Test(add_data_region_adjacent_between_two_existing,
    EnumParam(capacity, 2, 3, 4, 1000))
  {
    DataRegionSet* set = create_test_data_region_set(capacity, 0);
    DataRegion existingA = (DataRegion){.first_index = 100, .last_index = 199};
    DataRegion existingB = (DataRegion){.first_index = 700, .last_index = 799};
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingA));
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingB));

    DataRegion toAdd = (DataRegion){.first_index = existingA.last_index + 1, .last_index = existingB.first_index - 1};
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, toAdd));

    assert_int_eq(capacity, set->capacity);
    assert_int_eq(1, set->count);//Because all regions will be combined
    DataRegion combined = (DataRegion){.first_index = existingA.first_index, .last_index = existingB.last_index};
    assert_memory_eq(&combined, &set->regions[0], sizeof(DataRegion));

    free_test_data_region_set(set);
  }

Test(add_data_region_partially_overlapping_one_of_two_existing,
    EnumParam(capacity, 2, 3, 4, 1000)
    EnumParam(relativeIndex, 0, 1)
    EnumParam(toLeft, 0, 1))
  {
    DataRegionSet* set = create_test_data_region_set(capacity, 0);
    DataRegion existingA = (DataRegion){.first_index = 100, .last_index = 199};
    DataRegion existingB = (DataRegion){.first_index = 700, .last_index = 799};
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingA));
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingB));

    DataRegion relative = relativeIndex == 0 ? existingA : existingB;

    DataRegion toAdd =
      toLeft ? (DataRegion){.first_index = relative.first_index - 10, .last_index = relative.first_index + 10}
             : (DataRegion){.first_index = relative.last_index - 10, .last_index = relative.last_index + 10};
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, toAdd));

    assert_int_eq(capacity, set->capacity);
    assert_int_eq(2, set->count);
    assert_int_eq(100+100+10, get_data_region_set_total_length(set));

    DataRegion combined =
      toLeft ? (DataRegion){.first_index = toAdd.first_index, .last_index = relative.last_index}
             : (DataRegion){.first_index = relative.first_index, .last_index = toAdd.last_index};
    if(relativeIndex == 0)
    {
      assert_memory_eq(&combined, &set->regions[0], sizeof(DataRegion));
      assert_memory_eq(&existingB, &set->regions[1], sizeof(DataRegion));
    }
    else
    {
      assert_memory_eq(&existingA, &set->regions[0], sizeof(DataRegion));
      assert_memory_eq(&combined, &set->regions[1], sizeof(DataRegion));
    }

    free_test_data_region_set(set);
  }

  Test(add_data_region_partially_overlapping_both_of_two_existing,
    EnumParam(capacity, 2, 3, 4, 1000))
  {
    DataRegionSet* set = create_test_data_region_set(capacity, 0);
    DataRegion existingA = (DataRegion){.first_index = 100, .last_index = 199};
    DataRegion existingB = (DataRegion){.first_index = 700, .last_index = 799};
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingA));
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingB));

    DataRegion toAdd = (DataRegion){.first_index = existingA.first_index + 10, .last_index = existingB.last_index - 10};
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, toAdd));

    assert_int_eq(capacity, set->capacity);
    assert_int_eq(1, set->count);//1 because all regions were combined
    assert_int_eq(700, get_data_region_set_total_length(set));

    DataRegion combined = (DataRegion){.first_index = existingA.first_index, .last_index = existingB.last_index};
    assert_memory_eq(&combined, &set->regions[0], sizeof(DataRegion));

    free_test_data_region_set(set);
  }

  Test(add_data_region_equal_to_one_of_several,
    EnumParam(capacity, 2, 3, 4, 1000)
    EnumParam(existingIndex, 0, 1))
  {
    DataRegionSet* set = create_test_data_region_set(capacity, 0);
    DataRegion existingA = (DataRegion){.first_index = 100, .last_index = 199};
    DataRegion existingB = (DataRegion){.first_index = 700, .last_index = 799};
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingA));
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingB));

    DataRegion toAdd = existingIndex == 0 ? existingA : existingB;
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, toAdd));

    assert_int_eq(capacity, set->capacity);
    assert_int_eq(2, set->count);
    assert_int_eq(100+100, get_data_region_set_total_length(set));

    assert_memory_eq(&existingA, &set->regions[0], sizeof(DataRegion));
    assert_memory_eq(&existingB, &set->regions[1], sizeof(DataRegion));

    free_test_data_region_set(set);
  }

  Test(add_data_region_totally_overlapped_by_one_of_two_existing,
    EnumParam(capacity, 2, 3, 4, 1000)
    EnumParam(existingIndex, 0, 1))
  {
    DataRegionSet* set = create_test_data_region_set(capacity, 0);
    DataRegion existingA = (DataRegion){.first_index = 100, .last_index = 199};
    DataRegion existingB = (DataRegion){.first_index = 700, .last_index = 799};
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingA));
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingB));

    DataRegion overlappedBy = existingIndex == 0 ? existingA : existingB;
    DataRegion toAdd = (DataRegion){.first_index = overlappedBy.first_index + 10, .last_index = overlappedBy.last_index - 10};
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, toAdd));

    assert_int_eq(capacity, set->capacity);
    assert_int_eq(2, set->count);
    assert_int_eq(100+100, get_data_region_set_total_length(set));

    assert_memory_eq(&existingA, &set->regions[0], sizeof(DataRegion));
    assert_memory_eq(&existingB, &set->regions[1], sizeof(DataRegion));

    free_test_data_region_set(set);
  }

  Test(add_data_region_totally_overlapping_one_of_two_existing,
    EnumParam(capacity, 2, 3, 4, 1000)
    EnumParam(existingIndex, 0, 1))
  {
    DataRegionSet* set = create_test_data_region_set(capacity, 0);
    DataRegion existingA = (DataRegion){.first_index = 100, .last_index = 199};
    DataRegion existingB = (DataRegion){.first_index = 700, .last_index = 799};
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingA));
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingB));

    DataRegion overlappedBy = existingIndex == 0 ? existingA : existingB;
    DataRegion toAdd = (DataRegion){.first_index = overlappedBy.first_index - 10, .last_index = overlappedBy.last_index + 10};
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, toAdd));

    assert_int_eq(capacity, set->capacity);
    assert_int_eq(2, set->count);
    assert_int_eq(100+120, get_data_region_set_total_length(set));

    if(existingIndex == 0)
    {
      assert_memory_eq(&toAdd, &set->regions[0], sizeof(DataRegion));
      assert_memory_eq(&existingB, &set->regions[1], sizeof(DataRegion));
    }
    else
    {
      assert_memory_eq(&existingA, &set->regions[0], sizeof(DataRegion));
      assert_memory_eq(&toAdd, &set->regions[1], sizeof(DataRegion));
    }

    free_test_data_region_set(set);
  }

  Test(add_data_region_totally_overlapping_both_of_two_existing,
    EnumParam(capacity, 2, 3, 1000)
    EnumParam(padding, 0, 1, 2, 3, 1000))
  {
    DataRegionSet* set = create_test_data_region_set(capacity, 0);
    DataRegion existingA = (DataRegion){.first_index = 100, .last_index = 199};
    DataRegion existingB = (DataRegion){.first_index = 700, .last_index = 799};
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingA));
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingB));

    DataRegion toAdd = (DataRegion){.first_index = existingA.first_index - padding, .last_index = existingB.last_index + padding};
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, toAdd));

    assert_int_eq(capacity, set->capacity);
    assert_int_eq(1, set->count);
    assert_int_eq(700+(2*padding), get_data_region_set_total_length(set));
    assert_memory_eq(&toAdd, &set->regions[0], sizeof(DataRegion));

    free_test_data_region_set(set);
  }

  Test(add_data_region_far_from_three_existing,
    EnumParam(capacity, 4, 5, 6, 1000)
    EnumParam(offset, -10000, 10000))
  {
    DataRegionSet* set = create_test_data_region_set(capacity, 0);
    DataRegion existingA = (DataRegion){.first_index = 100, .last_index = 199};
    DataRegion existingB = (DataRegion){.first_index = 300, .last_index = 399};
    DataRegion existingC = (DataRegion){.first_index = 500, .last_index = 599};
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingA));
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingB));
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingC));

    DataRegion toAdd = (DataRegion){.first_index = offset, .last_index = offset + 49};
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, toAdd));

    assert_int_eq(capacity, set->capacity);
    assert_int_eq(4, set->count);
    assert_int_eq(100+100+100+50, get_data_region_set_total_length(set));

    if(offset < existingA.first_index)
    {
      assert_memory_eq(&toAdd, &set->regions[0], sizeof(DataRegion));
      assert_memory_eq(&existingA, &set->regions[1], sizeof(DataRegion));
      assert_memory_eq(&existingB, &set->regions[2], sizeof(DataRegion));
      assert_memory_eq(&existingC, &set->regions[3], sizeof(DataRegion));
    }
    else
    {
      assert_memory_eq(&existingA, &set->regions[0], sizeof(DataRegion));
      assert_memory_eq(&existingB, &set->regions[1], sizeof(DataRegion));
      assert_memory_eq(&existingC, &set->regions[2], sizeof(DataRegion));
      assert_memory_eq(&toAdd, &set->regions[3], sizeof(DataRegion));
    }

    free_test_data_region_set(set);
  }

  Test(add_data_region_many_non_combinable,
    EnumParam(capacity, 5, 6, 7, 1000))
  {
    DataRegionSet* set = create_test_data_region_set(capacity, 0);
    assert_add_data_region(set, -1000, -900);
    assert_add_data_region(set, 0, 100);
    assert_add_data_region(set, 150, 159);
    assert_add_data_region(set, 200, 299);
    assert_add_data_region(set, 400, 449);

    assert_data_region_set_eq_array(set, DR(-1000, -900), DR(0, 100), DR(150, 159), DR(200, 299), DR(400, 449));
  }

  Test(add_data_region_many_scenarios,
    EnumParam(capacity, 5, 6, 7, 1000))
  {
    DataRegionSet* set = create_test_data_region_set(capacity, 0);

    assert_add_data_region(set, 0, 0);
    assert_add_data_region(set, 1, 1);//Should be combined with left region at (0,0)
    assert_data_region_set_eq_array(set, DR(0, 1));

    assert_add_data_region(set, 5, 5);
    assert_add_data_region(set, 4, 4);//Should be combined with right region at (5,5)
    assert_data_region_set_eq_array(set, DR(0, 1), DR(4, 5));

    assert_add_data_region(set, 6, 6);//Should be combined with left region at (4,5)
    assert_data_region_set_eq_array(set, DR(0, 1), DR(4, 6));

    assert_add_data_region(set, 13, 17);
    assert_add_data_region(set, 9, 12);//Should be combined with right region at (13,17)
    assert_data_region_set_eq_array(set, DR(0, 1), DR(4, 6), DR(9, 17));

    assert_add_data_region(set, 9, 17);//Should have no effect since the exact region already exists
    assert_data_region_set_eq_array(set, DR(0, 1), DR(4, 6), DR(9, 17));

    assert_add_data_region(set, 9, 9);//Should have no effect since the region was already contained
    assert_data_region_set_eq_array(set, DR(0, 1), DR(4, 6), DR(9, 17));

    assert_add_data_region(set, 9, 10);//Should have no effect since the region was already contained
    assert_data_region_set_eq_array(set, DR(0, 1), DR(4, 6), DR(9, 17));

    assert_add_data_region(set, 9, 13);//Should have no effect since the region was already contained
    assert_data_region_set_eq_array(set, DR(0, 1), DR(4, 6), DR(9, 17));

    assert_add_data_region(set, 10, 10);//Should have no effect since the region was already contained
    assert_data_region_set_eq_array(set, DR(0, 1), DR(4, 6), DR(9, 17));

    assert_add_data_region(set, 10, 11);//Should have no effect since the region was already contained
    assert_data_region_set_eq_array(set, DR(0, 1), DR(4, 6), DR(9, 17));

    assert_add_data_region(set, 11, 11);//Should have no effect since the region was already contained
    assert_data_region_set_eq_array(set, DR(0, 1), DR(4, 6), DR(9, 17));

    assert_add_data_region(set, 11, 12);//Should have no effect since the region was already contained
    assert_data_region_set_eq_array(set, DR(0, 1), DR(4, 6), DR(9, 17));

    assert_add_data_region(set, 11, 17);//Should have no effect since the region was already contained
    assert_data_region_set_eq_array(set, DR(0, 1), DR(4, 6), DR(9, 17));

    assert_add_data_region(set, 100, 150);
    assert_data_region_set_eq_array(set, DR(0, 1), DR(4, 6), DR(9, 17), DR(100, 150));

    assert_add_data_region(set, 99, 151);//Should be combined with (100, 150) since it contains all of this region
    assert_data_region_set_eq_array(set, DR(0, 1), DR(4, 6), DR(9, 17), DR(99, 151));

    assert_add_data_region(set, 95, 155);//Should be combined with (99, 151) since it contains all of this region
    assert_data_region_set_eq_array(set, DR(0, 1), DR(4, 6), DR(9, 17), DR(95, 155));

    assert_add_data_region(set, 90, 95);//Should be combined with (95, 155) due to intersection at (95, 95)
    assert_data_region_set_eq_array(set, DR(0, 1), DR(4, 6), DR(9, 17), DR(90, 155));

    assert_add_data_region(set, 85, 100);//Should be combined with (90, 155) due to intersection at (90, 100)
    assert_data_region_set_eq_array(set, DR(0, 1), DR(4, 6), DR(9, 17), DR(85, 155));

    assert_add_data_region(set, 100, 200);//Should be combined with (85, 155) due to intersection at (100, 155)
    assert_data_region_set_eq_array(set, DR(0, 1), DR(4, 6), DR(9, 17), DR(85, 200));

    assert_add_data_region(set, 200, 300);//Should be combined with (85, 200) due to intersection at (200, 200)
    assert_data_region_set_eq_array(set, DR(0, 1), DR(4, 6), DR(9, 17), DR(85, 300));

    assert_add_data_region(set, 0, 6);//Should be combined with (0, 1) AND (4, 6)
    assert_data_region_set_eq_array(set, DR(0, 6), DR(9, 17), DR(85, 300));

    assert_add_data_region(set, 7, 8);//Should be combined with (0, 6) AND (9, 17)
    assert_data_region_set_eq_array(set, DR(0, 17), DR(85, 300));

    assert_add_data_region(set, 18, 84);//Should be combined with (0, 17) AND (85, 300)
    assert_data_region_set_eq_array(set, DR(0, 300));

    assert_add_data_region(set, 500, 600);
    assert_add_data_region(set, 900, 1200);
    assert_add_data_region(set, 1500, 2000);
    assert_add_data_region(set, 2300, 2500);

    assert_data_region_set_eq_array(set, DR(0, 300), DR(500, 600), DR(900, 1200), DR(1500, 2000), DR(2300, 2500));

    assert_add_data_region(set, 250, 2400);//Should combine with all
    assert_data_region_set_eq_array(set, DR(0, 2500));

    free_test_data_region_set(set);
  }

  Test(add_data_region_fails_when_full_capacity_and_no_overlap,
    EnumParam(capacity, 0, 1, 2, 3, 4, 1000))
  {
    DataRegionSet* set = create_test_data_region_set(capacity, capacity);
    DataRegionSet* clone = clone_data_region_set(set);

    DataRegion toAdd = (DataRegion){.first_index = -100000, .last_index = -90000};
    assert_int_eq(DATA_REGION_SET_OUT_OF_SPACE, add_data_region(set, toAdd));

    //Check that the set wasn't modified
    assert_data_region_set_eq(clone, set);

    free_test_data_region_set(set);
    free_clone_data_region_set(clone);
  }

  Test(add_data_region_fails_when_capacity_is_zero)
  {
    DataRegionSet* set = create_test_data_region_set(0, 0);

    DataRegion toAdd = (DataRegion){.first_index = 100, .last_index = 199};
    assert_int_eq(DATA_REGION_SET_OUT_OF_SPACE, add_data_region(set, toAdd));

    assert_int_eq(0, set->capacity);
    assert_int_eq(0, set->count);
    assert_int_eq(0, get_data_region_set_total_length(set));

    free_test_data_region_set(set);
  }

END_TEST_SUITE()


BEGIN_TEST_SUITE(DataRegionSetRemoveTests)

  Test(remove_data_region_when_src_NULL)
  {
    DataRegion toRemove = (DataRegion){.first_index = 0, .last_index = 9};
    assert_int_eq(DATA_REGION_SET_NULL_ARG, remove_data_region(NULL, toRemove));
  }

  Test(remove_data_region_when_region_is_invalid,
    EnumParam(count, 0, 1, 2, 1000))
  {
    DataRegionSet* set = create_test_data_region_set(count + 1, count);
    DataRegionSet* clone = clone_data_region_set(set);

    DataRegion invalid = (DataRegion){.first_index = 7, .last_index = 1};
    assert_int_eq(DATA_REGION_SET_INVALID_REGION, remove_data_region(set, invalid));

    //Check that the set wasn't affected
    assert_data_region_set_eq(clone, set);

    free_test_data_region_set(set);
    free_clone_data_region_set(clone);
  }

  Test(remove_data_region_no_effect_when_empty,
    EnumParam(capacity, 0, 1, 2, 3, 1000))
  {
    DataRegionSet* set = create_test_data_region_set(capacity, 0);

    DataRegion toRemove = (DataRegion){.first_index = 0, .last_index = 199};
    assert_int_eq(DATA_REGION_SET_SUCCESS, remove_data_region(set, toRemove));

    assert_int_eq(capacity, set->capacity);
    assert_int_eq(0, set->count);
    assert_int_eq(0, get_data_region_set_total_length(set));

    free_test_data_region_set(set);
  }

  Test(remove_data_region_no_effect_when_region_far_from_single,
    EnumParam(capacity, 1, 2, 3, 1000))
  {
    DataRegionSet* set = create_test_data_region_set(capacity, 0);
    DataRegion existing = (DataRegion){.first_index = 0, .last_index = 99};
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existing));

    DataRegion toRemove = (DataRegion){.first_index = 10000, .last_index = 10999};
    assert_int_eq(DATA_REGION_SET_SUCCESS, remove_data_region(set, toRemove));

    assert_int_eq(capacity, set->capacity);
    assert_int_eq(1, set->count);
    assert_int_eq(100, get_data_region_set_total_length(set));
    assert_memory_eq(&existing, &set->regions[0], sizeof(DataRegion));

    free_test_data_region_set(set);
  }

  Test(remove_data_region_no_effect_when_adjacent_to_single,
    EnumParam(capacity, 1, 2, 3, 1000)
    EnumParam(toLeft, 0, 1))
  {
    DataRegionSet* set = create_test_data_region_set(capacity, 0);
    DataRegion existing = (DataRegion){.first_index = 0, .last_index = 99};
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existing));

    DataRegion toRemove =
      toLeft ? (DataRegion){.first_index = existing.first_index - 100, .last_index = existing.first_index - 1}
             : (DataRegion){.first_index = existing.last_index + 1, .last_index = existing.last_index + 100};
    assert_int_eq(DATA_REGION_SET_SUCCESS, remove_data_region(set, toRemove));

    assert_int_eq(capacity, set->capacity);
    assert_int_eq(1, set->count);
    assert_int_eq(100, get_data_region_set_total_length(set));
    assert_memory_eq(&existing, &set->regions[0], sizeof(DataRegion));

    free_test_data_region_set(set);
  }

  Test(remove_data_region_overlap_of_single,
    EnumParam(capacity, 1, 2, 3, 1000)
    EnumParam(toLeft, 0, 1))
  {
    DataRegionSet* set = create_test_data_region_set(capacity, 0);
    DataRegion existing = (DataRegion){.first_index = 0, .last_index = 99};
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existing));

    DataRegion toRemove =
      toLeft ? (DataRegion){.first_index = existing.first_index - 10, .last_index = existing.first_index + 9}
             : (DataRegion){.first_index = existing.last_index - 9, .last_index = existing.last_index + 10};
    assert_int_eq(DATA_REGION_SET_SUCCESS, remove_data_region(set, toRemove));

    assert_int_eq(capacity, set->capacity);
    assert_int_eq(1, set->count);
    assert_int_eq(100-10, get_data_region_set_total_length(set));

    DataRegion expect =
      toLeft ? (DataRegion){.first_index = toRemove.last_index + 1, .last_index = existing.last_index}
             : (DataRegion){.first_index = existing.first_index, .last_index = toRemove.first_index - 1};
    assert_memory_eq(&expect, &set->regions[0], sizeof(DataRegion));

    free_test_data_region_set(set);
  }

  Test(remove_data_region_overlap_all_of_single,
    EnumParam(capacity, 1, 2, 3, 1000)
    EnumParam(padding, 0, 1, 2, 1000))
  {
    DataRegionSet* set = create_test_data_region_set(capacity, 0);
    DataRegion existing = (DataRegion){.first_index = 0, .last_index = 99};
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existing));

    DataRegion toRemove = (DataRegion){.first_index = existing.first_index - padding, .last_index = existing.last_index + padding};
    assert_int_eq(DATA_REGION_SET_SUCCESS, remove_data_region(set, toRemove));

    assert_int_eq(capacity, set->capacity);
    assert_int_eq(0, set->count);
    assert_int_eq(0, get_data_region_set_total_length(set));

    free_test_data_region_set(set);
  }

  Test(remove_data_region_overlap_equal_to_single,
    EnumParam(capacity, 1, 2, 3, 1000))
  {
    DataRegionSet* set = create_test_data_region_set(capacity, 0);
    DataRegion existing = (DataRegion){.first_index = 0, .last_index = 99};
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existing));

    DataRegion toRemove = (DataRegion){.first_index = existing.first_index, .last_index = existing.last_index};
    assert_int_eq(DATA_REGION_SET_SUCCESS, remove_data_region(set, toRemove));

    assert_int_eq(capacity, set->capacity);
    assert_int_eq(0, set->count);
    assert_int_eq(0, get_data_region_set_total_length(set));

    free_test_data_region_set(set);
  }

  Test(remove_data_region_overlap_middle_of_single,
    EnumParam(capacity, 2, 3, 4, 1000))
  {
    DataRegionSet* set = create_test_data_region_set(capacity, 0);
    DataRegion existing = (DataRegion){.first_index = 0, .last_index = 99};
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existing));

    DataRegion toRemove = (DataRegion){.first_index = existing.first_index + 10, .last_index = existing.last_index - 10};
    assert_int_eq(DATA_REGION_SET_SUCCESS, remove_data_region(set, toRemove));

    assert_int_eq(capacity, set->capacity);
    assert_int_eq(2, set->count);
    assert_int_eq(20, get_data_region_set_total_length(set));

    DataRegion remainLeft = (DataRegion){.first_index = existing.first_index, .last_index = toRemove.first_index - 1};
    DataRegion remainRight = (DataRegion){.first_index = toRemove.last_index + 1, .last_index = existing.last_index};
    assert_memory_eq(&remainLeft, &set->regions[0], sizeof(DataRegion));
    assert_memory_eq(&remainRight, &set->regions[1], sizeof(DataRegion));

    free_test_data_region_set(set);
  }

  Test(remove_data_region_overlap_middle_of_single_fails_when_capacity_exceeded,
    EnumParam(capacity, 1, 2, 3, 1000))
  {
    DataRegionSet* set = create_test_data_region_set(capacity, capacity);
    DataRegionSet* clone = clone_data_region_set(set);

    DataRegion reference = set->regions[set->count / 2];//Arbitrarily remove from middle region
    DataRegion toRemove = (DataRegion){.first_index = reference.first_index + 3, .last_index = reference.last_index - 3};
    assert_int_eq(DATA_REGION_SET_OUT_OF_SPACE, remove_data_region(set, toRemove));

    //Check that the set wasn't modified
    assert_data_region_set_eq(clone, set);

    free_test_data_region_set(set);
    free_clone_data_region_set(clone);
  }

  Test(remove_data_region_no_effect_when_region_far_from_several,
    EnumParam(count, 2, 3, 4, 1000))
  {
    DataRegionSet* set = create_test_data_region_set(count + 3, count);
    DataRegionSet* clone = clone_data_region_set(set);

    DataRegion toRemove = (DataRegion){.first_index = -1000000, .last_index = -900000};
    assert_int_eq(DATA_REGION_SET_SUCCESS, remove_data_region(set, toRemove));

    //Check that the set wasn't modified
    assert_data_region_set_eq(clone, set);

    free_test_data_region_set(set);
    free_clone_data_region_set(clone);
  }

  Test(remove_data_region_overlap_of_one_of_several,
    EnumParam(capacity, 3, 4, 1000)
    EnumParam(toLeft, 0, 1))
  {
    DataRegionSet* set = create_test_data_region_set(capacity, 0);
    DataRegion existingA = (DataRegion){.first_index = 100, .last_index = 199};
    DataRegion existingB = (DataRegion){.first_index = 300, .last_index = 399};
    DataRegion existingC = (DataRegion){.first_index = 500, .last_index = 599};
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingA));
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingB));
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingC));

    DataRegion toRemove =
      toLeft ? (DataRegion){.first_index = existingB.first_index - 10, .last_index = existingB.first_index + 9}
             : (DataRegion){.first_index = existingB.last_index - 9, .last_index = existingB.last_index + 10};
    assert_int_eq(DATA_REGION_SET_SUCCESS, remove_data_region(set, toRemove));

    DataRegion remaining =
      toLeft ? (DataRegion){.first_index = toRemove.last_index + 1, .last_index = existingB.last_index}
             : (DataRegion){.first_index = existingB.first_index, .last_index = toRemove.first_index - 1};

    assert_int_eq(capacity, set->capacity);
    assert_int_eq(3, set->count);
    assert_int_eq(100+100+90, get_data_region_set_total_length(set));
    assert_memory_eq(&existingA, &set->regions[0], sizeof(DataRegion));
    assert_memory_eq(&remaining, &set->regions[1], sizeof(DataRegion));
    assert_memory_eq(&existingC, &set->regions[2], sizeof(DataRegion));

    free_test_data_region_set(set);
  }

  Test(remove_data_region_equal_to_one_of_several,
    EnumParam(capacity, 3, 4, 5, 1000))
  {
    DataRegionSet* set = create_test_data_region_set(capacity, 0);
    DataRegion existingA = (DataRegion){.first_index = 100, .last_index = 199};
    DataRegion existingB = (DataRegion){.first_index = 300, .last_index = 399};
    DataRegion existingC = (DataRegion){.first_index = 500, .last_index = 599};
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingA));
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingB));
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingC));

    DataRegion toRemove = (DataRegion){.first_index = existingB.first_index, .last_index = existingB.last_index};
    assert_int_eq(DATA_REGION_SET_SUCCESS, remove_data_region(set, toRemove));

    assert_int_eq(capacity, set->capacity);
    assert_int_eq(2, set->count);
    assert_int_eq(100+100, get_data_region_set_total_length(set));
    assert_memory_eq(&existingA, &set->regions[0], sizeof(DataRegion));
    assert_memory_eq(&existingC, &set->regions[1], sizeof(DataRegion));

    free_test_data_region_set(set);
  }

  Test(remove_data_region_overlap_partially_two_of_several,
    EnumParam(capacity, 4, 5, 6, 1000))
  {
    DataRegionSet* set = create_test_data_region_set(capacity, 0);
    DataRegion existingA = (DataRegion){.first_index = 100, .last_index = 199};
    DataRegion existingB = (DataRegion){.first_index = 300, .last_index = 399};
    DataRegion existingC = (DataRegion){.first_index = 500, .last_index = 599};
    DataRegion existingD = (DataRegion){.first_index = 700, .last_index = 799};
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingA));
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingB));
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingC));
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingD));

    DataRegion toRemove = (DataRegion){.first_index = existingB.last_index - 9, .last_index = existingC.first_index + 9};
    assert_int_eq(DATA_REGION_SET_SUCCESS, remove_data_region(set, toRemove));

    assert_int_eq(capacity, set->capacity);
    assert_int_eq(4, set->count);
    assert_int_eq(100+90+90+100, get_data_region_set_total_length(set));

    DataRegion cropB = (DataRegion){.first_index = existingB.first_index, .last_index = toRemove.first_index - 1};
    DataRegion cropC = (DataRegion){.first_index = toRemove.last_index + 1, .last_index = existingC.last_index};
    assert_memory_eq(&existingA, &set->regions[0], sizeof(DataRegion));
    assert_memory_eq(&cropB, &set->regions[1], sizeof(DataRegion));
    assert_memory_eq(&cropC, &set->regions[2], sizeof(DataRegion));
    assert_memory_eq(&existingD, &set->regions[3], sizeof(DataRegion));

    free_test_data_region_set(set);
  }

  Test(remove_data_region_overlap_totally_two_of_several,
    EnumParam(capacity, 4, 5, 6, 1000)
    EnumParam(padding, 0, 1, 2, 3, 25))
  {
    DataRegionSet* set = create_test_data_region_set(capacity, 0);
    DataRegion existingA = (DataRegion){.first_index = 100, .last_index = 199};
    DataRegion existingB = (DataRegion){.first_index = 300, .last_index = 399};
    DataRegion existingC = (DataRegion){.first_index = 500, .last_index = 599};
    DataRegion existingD = (DataRegion){.first_index = 700, .last_index = 799};
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingA));
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingB));
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingC));
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingD));

    DataRegion toRemove = (DataRegion){.first_index = existingB.first_index - padding, .last_index = existingC.last_index + padding};
    assert_int_eq(DATA_REGION_SET_SUCCESS, remove_data_region(set, toRemove));

    assert_int_eq(capacity, set->capacity);
    assert_int_eq(2, set->count);
    assert_int_eq(100+100, get_data_region_set_total_length(set));

    assert_memory_eq(&existingA, &set->regions[0], sizeof(DataRegion));
    assert_memory_eq(&existingD, &set->regions[1], sizeof(DataRegion));

    free_test_data_region_set(set);
  }

  Test(remove_data_region_strictly_contain_two_of_several,
    EnumParam(capacity, 4, 5, 6, 1000))
  {
    DataRegionSet* set = create_test_data_region_set(capacity, 0);
    DataRegion existingA = (DataRegion){.first_index = 100, .last_index = 199};
    DataRegion existingB = (DataRegion){.first_index = 300, .last_index = 399};
    DataRegion existingC = (DataRegion){.first_index = 500, .last_index = 599};
    DataRegion existingD = (DataRegion){.first_index = 700, .last_index = 799};
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingA));
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingB));
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingC));
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingD));

    DataRegion toRemove = (DataRegion){.first_index = existingB.first_index, .last_index = existingC.last_index};
    assert_int_eq(DATA_REGION_SET_SUCCESS, remove_data_region(set, toRemove));

    assert_int_eq(capacity, set->capacity);
    assert_int_eq(2, set->count);
    assert_int_eq(100+100, get_data_region_set_total_length(set));

    assert_memory_eq(&existingA, &set->regions[0], sizeof(DataRegion));
    assert_memory_eq(&existingD, &set->regions[1], sizeof(DataRegion));

    free_test_data_region_set(set);
  }

  Test(remove_data_region_totally_contain_all_of_several,
    EnumParam(capacity, 4, 5, 6, 1000)
    EnumParam(padding, 0, 1, 2, 3, 25))
  {
    DataRegionSet* set = create_test_data_region_set(capacity, 0);
    DataRegion existingA = (DataRegion){.first_index = 100, .last_index = 199};
    DataRegion existingB = (DataRegion){.first_index = 300, .last_index = 399};
    DataRegion existingC = (DataRegion){.first_index = 500, .last_index = 599};
    DataRegion existingD = (DataRegion){.first_index = 700, .last_index = 799};
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingA));
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingB));
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingC));
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingD));

    DataRegion toRemove = (DataRegion){.first_index = existingA.first_index - padding, .last_index = existingD.last_index + padding};
    assert_int_eq(DATA_REGION_SET_SUCCESS, remove_data_region(set, toRemove));

    assert_int_eq(capacity, set->capacity);
    assert_int_eq(0, set->count);
    assert_int_eq(0, get_data_region_set_total_length(set));

    free_test_data_region_set(set);
  }

  Test(remove_data_region_overlap_middle_of_one_of_several,
    EnumParam(capacity, 5, 6, 7, 1000))
  {
    DataRegionSet* set = create_test_data_region_set(capacity, 0);
    DataRegion existingA = (DataRegion){.first_index = 100, .last_index = 199};
    DataRegion existingB = (DataRegion){.first_index = 300, .last_index = 399};
    DataRegion existingC = (DataRegion){.first_index = 500, .last_index = 599};
    DataRegion existingD = (DataRegion){.first_index = 700, .last_index = 799};
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingA));
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingB));
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingC));
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingD));

    DataRegion toRemove = (DataRegion){.first_index = existingB.first_index + 10, .last_index = existingB.last_index - 10};
    assert_int_eq(DATA_REGION_SET_SUCCESS, remove_data_region(set, toRemove));

    assert_int_eq(capacity, set->capacity);
    assert_int_eq(5, set->count);
    assert_int_eq(100+10+10+100+100, get_data_region_set_total_length(set));

    DataRegion bLeft = (DataRegion){.first_index = existingB.first_index, .last_index = toRemove.first_index - 1};
    DataRegion bRight = (DataRegion){.first_index = toRemove.last_index + 1, .last_index = existingB.last_index};

    assert_memory_eq(&existingA, &set->regions[0], sizeof(DataRegion));
    assert_memory_eq(&bLeft, &set->regions[1], sizeof(DataRegion));
    assert_memory_eq(&bRight, &set->regions[2], sizeof(DataRegion));
    assert_memory_eq(&existingC, &set->regions[3], sizeof(DataRegion));
    assert_memory_eq(&existingD, &set->regions[4], sizeof(DataRegion));

    free_test_data_region_set(set);
  }

  Test(remove_data_region_overlap_middle_of_one_of_several_fails_when_capacity_exceeded,
    EnumParam(capacity, 4, 5, 6, 1000))
  {
    DataRegionSet* set = create_test_data_region_set(capacity, capacity);
    DataRegionSet* clone = clone_data_region_set(set);

    //Try to remove the middle portion of a region
    DataRegion reference = set->regions[set->count / 2];//Arbitrarily pick middle
    DataRegion toRemove = (DataRegion){.first_index = reference.first_index + 3, .last_index = reference.last_index - 3};
    assert_int_eq(DATA_REGION_SET_OUT_OF_SPACE, remove_data_region(set, toRemove));

    //Check that nothing changed
    assert_data_region_set_eq(clone, set);

    free_test_data_region_set(set);
    free_clone_data_region_set(clone);
  }

  Test(remove_data_region_no_effect_when_adjacent_to_one_of_several,
    EnumParam(capacity, 4, 5, 6, 1000)
    EnumParam(relativeOffset, 0, 1, 2, 3)
    EnumParam(toLeft, 0, 1))
  {
    DataRegionSet* set = create_test_data_region_set(capacity, 4);
    DataRegionSet* clone = clone_data_region_set(set);

    DataRegion relative = set->regions[relativeOffset];
    DataRegion toRemove =
      toLeft ? (DataRegion){.first_index = relative.first_index - 10, .last_index = relative.first_index - 1}
             : (DataRegion){.first_index = relative.last_index + 1, .last_index = relative.last_index + 10};
    assert_int_eq(DATA_REGION_SET_SUCCESS, remove_data_region(set, toRemove));

    //Check that the set wasn't modified
    assert_data_region_set_eq(clone, set);

    free_test_data_region_set(set);
    free_clone_data_region_set(clone);
  }

  Test(remove_data_region_no_effect_when_adjacent_to_two_of_several,
    EnumParam(capacity, 4, 5, 6, 1000))
  {
    DataRegionSet* set = create_test_data_region_set(capacity, 0);
    DataRegion existingA = (DataRegion){.first_index = 100, .last_index = 199};
    DataRegion existingB = (DataRegion){.first_index = 300, .last_index = 399};
    DataRegion existingC = (DataRegion){.first_index = 500, .last_index = 599};
    DataRegion existingD = (DataRegion){.first_index = 700, .last_index = 799};
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingA));
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingB));
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingC));
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingD));

    DataRegion toRemove = (DataRegion){.first_index = existingB.last_index + 1, .last_index = existingC.first_index - 1};
    assert_int_eq(DATA_REGION_SET_SUCCESS, remove_data_region(set, toRemove));

    assert_int_eq(capacity, set->capacity);
    assert_int_eq(4, set->count);
    assert_int_eq(100+100+100+100, get_data_region_set_total_length(set));


    assert_memory_eq(&existingA, &set->regions[0], sizeof(DataRegion));
    assert_memory_eq(&existingB, &set->regions[1], sizeof(DataRegion));
    assert_memory_eq(&existingC, &set->regions[2], sizeof(DataRegion));
    assert_memory_eq(&existingD, &set->regions[3], sizeof(DataRegion));

    free_test_data_region_set(set);
  }

  Test(remove_data_region_overlaps_three_of_several,
    EnumParam(capacity, 4, 5, 6, 1000))
  {
    DataRegionSet* set = create_test_data_region_set(capacity, 0);
    DataRegion existingA = (DataRegion){.first_index = 100, .last_index = 199};
    DataRegion existingB = (DataRegion){.first_index = 300, .last_index = 399};
    DataRegion existingC = (DataRegion){.first_index = 500, .last_index = 599};
    DataRegion existingD = (DataRegion){.first_index = 700, .last_index = 799};
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingA));
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingB));
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingC));
    assert_int_eq(DATA_REGION_SET_SUCCESS, add_data_region(set, existingD));

    DataRegion toRemove = (DataRegion){.first_index = existingB.last_index - 9, .last_index = existingC.first_index + 9};
    assert_int_eq(DATA_REGION_SET_SUCCESS, remove_data_region(set, toRemove));

    assert_int_eq(capacity, set->capacity);
    assert_int_eq(4, set->count);
    assert_int_eq(100+90+90+100, get_data_region_set_total_length(set));

    DataRegion cropB = (DataRegion){.first_index = existingB.first_index, .last_index = toRemove.first_index - 1};
    DataRegion cropC = (DataRegion){.first_index = toRemove.last_index + 1, .last_index = existingC.last_index};
    assert_memory_eq(&existingA, &set->regions[0], sizeof(DataRegion));
    assert_memory_eq(&cropB, &set->regions[1], sizeof(DataRegion));
    assert_memory_eq(&cropC, &set->regions[2], sizeof(DataRegion));
    assert_memory_eq(&existingD, &set->regions[3], sizeof(DataRegion));

    free_test_data_region_set(set);
  }

  Test(remove_data_region_several_scenarios,
    EnumParam(capacity, 4, 5, 6, 1000))
  {
    DataRegionSet* set = create_test_data_region_set(capacity, 0);

    assert_add_data_region(set, 0, 1000);
    assert_remove_data_region(set, 0, 1000);//Should remove the whole thing
    assert_int_eq(0, set->count);

    assert_remove_data_region(set, 0, 1000);//Should have no effect
    assert_int_eq(0, set->count);

    assert_add_data_region(set, 0, 1000);
    assert_remove_data_region(set, 0, 0);//Should remove intersecting region at (0,0)
    assert_data_region_set_eq_array(set, DR(1,1000));

    assert_remove_data_region(set, 1, 1);//Should remove intersecting region at (1,1)
    assert_data_region_set_eq_array(set, DR(2, 1000));

    assert_remove_data_region(set, 1000, 1000);//Should remove intersecting region at (1000,1000)
    assert_data_region_set_eq_array(set, DR(2, 999));

    assert_remove_data_region(set, 0, 5);//Should remove intersecting region at (2,5)
    assert_data_region_set_eq_array(set, DR(6, 999));

    assert_remove_data_region(set, 950, 1500);//Should remove intersecting region at (950, 999)
    assert_data_region_set_eq_array(set, DR(6, 949));

    assert_remove_data_region(set, 12, 12);//Should remove intersecting region at (12,12)
    assert_data_region_set_eq_array(set, DR(6, 11), DR(13, 949));

    assert_remove_data_region(set, 100, 120);//Should remove intersecting region at (100, 120)
    assert_data_region_set_eq_array(set, DR(6, 11), DR(13, 99), DR(121, 949));

    assert_remove_data_region(set, 200, 300);//Should remove intersecting region at (200, 300)
    assert_data_region_set_eq_array(set, DR(6, 11), DR(13, 99), DR(121, 199), DR(301, 949));

    assert_remove_data_region(set, 95, 250);//Should remove intersecting region at (95, 99) AND (121, 199)
    assert_data_region_set_eq_array(set, DR(6, 11), DR(13, 94), DR(301, 949));

    assert_remove_data_region(set, 8, 948);//Should remove intersecting region at (8,11) AND (13, 94) AND (301, 948)
    assert_data_region_set_eq_array(set, DR(6, 7), DR(949, 949));

    assert_remove_data_region(set, 8, 948);//Should not change anything
    assert_data_region_set_eq_array(set, DR(6, 7), DR(949, 949));

    assert_remove_data_region(set, 0, 100000000);
    assert_int_eq(0, set->count);
    assert_int_eq(capacity, set->capacity);

    free_test_data_region_set(set);
  }

END_TEST_SUITE()

BEGIN_TEST_SUITE(DataRegionSetGetBoundedDataRegionsTests)

  Test(get_bounded_data_regions_when_src_NULL,
    EnumParam(nullDstOverflow, 0, 1))
  {
    DataRegion* dst = gid_malloc(sizeof(DataRegion) * 100);
    int dstTooSmall = 5;//Initial garbage value
    int* dstTooSmallPtr = nullDstOverflow ? NULL : &dstTooSmall;
    assert_int_eq(0, get_bounded_data_regions(dst, 100, NULL, (DataRegion){.first_index = 0, .last_index = 100}, dstTooSmallPtr));

    if(!nullDstOverflow)
      assert_int_eq(0, dstTooSmall);

    gid_free(dst);
  }

  Test(get_bounded_data_regions_when_dst_NULL,
    EnumParam(nullDstOverflow, 0, 1))
  {
    DataRegionSet* set = create_test_data_region_set(100, 100);
    DataRegionSet* clone = clone_data_region_set(set);

    int dstTooSmall = 5;//Initial garbage value
    int* dstTooSmallPtr = nullDstOverflow ? NULL : &dstTooSmall;
    assert_int_eq(100, get_bounded_data_regions(NULL, 0, set, DR(0, 1000000), dstTooSmallPtr));
    if(!nullDstOverflow)
      assert_int_eq(0, dstTooSmall);

    //Check that the set wasn't modified
    assert_data_region_set_eq(clone, set);

    free_test_data_region_set(set);
    free_clone_data_region_set(clone);
  }

  Test(get_bounded_data_regions_when_dstCapacity_negative,
    EnumParam(nullDstOverflow, 0, 1)
    EnumParam(dstCapacity, -1, -2, -3, -1000))
  {
    DataRegionSet* set = create_test_data_region_set(100, 100);
    DataRegionSet* clone = clone_data_region_set(set);
    DataRegion* dst = gid_malloc(0);

    int dstTooSmall = 5;//Initial garbage value
    int* dstTooSmallPtr = nullDstOverflow ? NULL : &dstTooSmall;
    assert_int_eq(0, get_bounded_data_regions(dst, dstCapacity, set, DR(0, 1000000), dstTooSmallPtr));
    if(!nullDstOverflow)
      assert_int_eq(1, dstTooSmall);

    //Check that the set wasn't modified
    assert_data_region_set_eq(clone, set);

    gid_free(dst);
    free_test_data_region_set(set);
    free_clone_data_region_set(clone);
  }

  Test(get_bounded_data_regions_when_boundary_is_invalid,
    EnumParam(nullDstOverflow, 0, 1)
    EnumParam(dstCapacity, 0, 1, 2, 100))
  {
    DataRegionSet* set = create_test_data_region_set(100, 100);
    DataRegionSet* clone = clone_data_region_set(set);

    DataRegion* dst = gid_malloc(sizeof(DataRegion) * dstCapacity);

    int dstTooSmall = 5;//Initial garbage value
    int* dstTooSmallPtr = nullDstOverflow ? NULL : &dstTooSmall;

    DataRegion badBounds = DR(1000, 0);
    assert_int_eq(0, get_bounded_data_regions(dst, dstCapacity, set, badBounds, dstTooSmallPtr));
    if(!nullDstOverflow)
      assert_int_eq(0, dstTooSmall);

    //Check that 'count' has the same results
    assert_int_eq(0, count_bounded_data_regions(set, badBounds));

    //Check that the set wasn't modified
    assert_data_region_set_eq(clone, set);

    gid_free(dst);
    free_test_data_region_set(set);
    free_clone_data_region_set(clone);
  }

  Test(get_bounded_data_regions_when_src_is_empty,
    EnumParam(nullDstOverflow, 0, 1)
    EnumParam(dstCapacity, 0, 1, 2, 100)
    EnumParam(srcCapacity, 0, 1, 2, 100))
  {
    DataRegionSet* set = create_test_data_region_set(srcCapacity, 0);
    DataRegionSet* clone = clone_data_region_set(set);
    DataRegion* dst = gid_malloc(sizeof(DataRegion) * dstCapacity);

    int dstTooSmall = 5;//Initial garbage value
    int* dstTooSmallPtr = nullDstOverflow ? NULL : &dstTooSmall;

    DataRegion bounds = DR(0, 1000);
    assert_int_eq(0, get_bounded_data_regions(dst, dstCapacity, set, bounds, dstTooSmallPtr));
    if(!nullDstOverflow)
      assert_int_eq(0, dstTooSmall);

    //Check that 'count' has the same results
    assert_int_eq(0, count_bounded_data_regions(set, bounds));

    //Check that the set wasn't modified
    assert_data_region_set_eq(clone, set);

    gid_free(dst);
    free_test_data_region_set(set);
    free_clone_data_region_set(clone);
  }

  Test(get_bounded_data_regions_when_boundary_contains_nothing,
    EnumParam(nullDstOverflow, 0, 1)
    EnumParam(dstCapacity, 0, 1, 2, 100)
    IntRow(-1000, -900)
    IntRow(-100, -1)
    IntRow(100, 100)
    IntRow(100, 110)
    IntRow(110, 120)
    IntRow(100, 199)
    IntRow(300, 310)
    IntRow(301, 320)
    IntRow(300, 399)
    IntRow(350, 399)
    IntRow(550, 599)
    IntRow(700, 700)
    IntRow(700, 710)
    IntRow(710, 1000))
  {
    DataRegionSet* set = create_test_data_region_set(4, 0);
    assert_add_data_region(set, 0, 99);
    assert_add_data_region(set, 200, 299);
    assert_add_data_region(set, 400, 499);
    assert_add_data_region(set, 600, 699);

    DataRegionSet* clone = clone_data_region_set(set);
    DataRegion* dst = gid_malloc(sizeof(DataRegion) * dstCapacity);

    int dstTooSmall = 5;//Initial garbage value
    int* dstTooSmallPtr = nullDstOverflow ? NULL : &dstTooSmall;

    DataRegion bounds = DR(int_row[0], int_row[1]);
    assert_int_eq(0, get_bounded_data_regions(dst, dstCapacity, set, bounds, dstTooSmallPtr));
    if(!nullDstOverflow)
      assert_int_eq(0, dstTooSmall);

    //Check that 'count' has the same results
    assert_int_eq(0, count_bounded_data_regions(set, bounds));

    //Check that the set wasn't modified
    assert_data_region_set_eq(clone, set);

    gid_free(dst);
    free_test_data_region_set(set);
    free_clone_data_region_set(clone);
  }

  Test(get_bounded_data_regions_when_boundary_partially_overlaps_single,
    EnumParam(nullDstOverflow, 0, 1)
    EnumParam(dstCapacity, 1, 2, 3, 1000)
    EnumParam(toLeft, 0, 1))
  {
    DataRegionSet* set = create_test_data_region_set(1, 0);
    assert_add_data_region(set, 0, 99);
    DataRegion single = set->regions[0];
    DataRegionSet* clone = clone_data_region_set(set);
    DataRegion* dst = gid_malloc(sizeof(DataRegion) * dstCapacity);

    int dstTooSmall = 5;//Initial garbage value
    int* dstTooSmallPtr = nullDstOverflow ? NULL : &dstTooSmall;

    DataRegion bounds =
      toLeft ? DR(single.first_index - 10, single.first_index + 10)
             : DR(single.last_index - 10, single.last_index + 10);

    assert_int_eq(1, get_bounded_data_regions(dst, dstCapacity, set, bounds, dstTooSmallPtr));
    if(!nullDstOverflow)
      assert_int_eq(0, dstTooSmall);

    DataRegion expect =
      toLeft ? DR(single.first_index, bounds.last_index)
             : DR(bounds.first_index, single.last_index);
    assert_memory_eq(&expect, &dst[0], sizeof(DataRegion));

    //Check that 'count' has the same results
    assert_int_eq(1, count_bounded_data_regions(set, bounds));

    //Check that the set wasn't modified
    assert_data_region_set_eq(clone, set);

    gid_free(dst);
    free_test_data_region_set(set);
    free_clone_data_region_set(clone);
  }

  Test(get_bounded_data_regions_when_boundary_partially_overlaps_several,
    EnumParam(nullDstOverflow, 0, 1)
    EnumParam(dstCapacity, 2, 3, 1000))
  {
    DataRegionSet* set = create_test_data_region_set(4, 0);
    assert_add_data_region(set, 0, 99);
    assert_add_data_region(set, 200, 299);
    assert_add_data_region(set, 400, 499);
    assert_add_data_region(set, 600, 699);
    DataRegionSet* clone = clone_data_region_set(set);
    DataRegion* dst = gid_malloc(sizeof(DataRegion) * dstCapacity);

    int dstTooSmall = 5;//Initial garbage value
    int* dstTooSmallPtr = nullDstOverflow ? NULL : &dstTooSmall;

    DataRegion bounds = DR(250, 449);
    assert_int_eq(2, get_bounded_data_regions(dst, dstCapacity, set, bounds, dstTooSmallPtr));
    if(!nullDstOverflow)
      assert_int_eq(0, dstTooSmall);

    assert_memory_eq(&DR(250, 299), &dst[0], sizeof(DataRegion));
    assert_memory_eq(&DR(400, 449), &dst[1], sizeof(DataRegion));

    //Check that 'count' has the same results
    assert_int_eq(2, count_bounded_data_regions(set, bounds));

    //Check that the set wasn't modified
    assert_data_region_set_eq(clone, set);

    gid_free(dst);
    free_test_data_region_set(set);
    free_clone_data_region_set(clone);
  }

  Test(get_bounded_data_regions_when_boundary_totally_overlaps_single,
    EnumParam(nullDstOverflow, 0, 1)
    EnumParam(dstCapacity, 1, 2, 3, 1000))
  {
    DataRegionSet* set = create_test_data_region_set(1, 0);
    assert_add_data_region(set, 0, 99);
    DataRegion single = set->regions[0];
    DataRegionSet* clone = clone_data_region_set(set);
    DataRegion* dst = gid_malloc(sizeof(DataRegion) * dstCapacity);

    int dstTooSmall = 5;//Initial garbage value
    int* dstTooSmallPtr = nullDstOverflow ? NULL : &dstTooSmall;

    DataRegion bounds = DR(-1000, 1000);

    assert_int_eq(1, get_bounded_data_regions(dst, dstCapacity, set, bounds, dstTooSmallPtr));
    if(!nullDstOverflow)
      assert_int_eq(0, dstTooSmall);

    assert_memory_eq(&single, &dst[0], sizeof(DataRegion));

    //Check that 'count' has the same results
    assert_int_eq(1, count_bounded_data_regions(set, bounds));

    //Check that the set wasn't modified
    assert_data_region_set_eq(clone, set);

    gid_free(dst);
    free_test_data_region_set(set);
    free_clone_data_region_set(clone);
  }

  Test(get_bounded_data_regions_when_boundary_totally_overlaps_one_and_partially_overlaps_other_of_several,
    EnumParam(nullDstOverflow, 0, 1)
    EnumParam(dstCapacity, 3, 4, 1000))
  {
    DataRegionSet* set = create_test_data_region_set(4, 0);
    assert_add_data_region(set, 0, 99);
    assert_add_data_region(set, 200, 299);
    assert_add_data_region(set, 400, 499);
    assert_add_data_region(set, 600, 699);
    DataRegionSet* clone = clone_data_region_set(set);
    DataRegion* dst = gid_malloc(sizeof(DataRegion) * dstCapacity);

    int dstTooSmall = 5;//Initial garbage value
    int* dstTooSmallPtr = nullDstOverflow ? NULL : &dstTooSmall;

    DataRegion bounds = DR(50, 449);
    assert_int_eq(3, get_bounded_data_regions(dst, dstCapacity, set, bounds, dstTooSmallPtr));
    if(!nullDstOverflow)
      assert_int_eq(0, dstTooSmall);

    assert_memory_eq(&DR(50, 99), &dst[0], sizeof(DataRegion));
    assert_memory_eq(&DR(200, 299), &dst[1], sizeof(DataRegion));
    assert_memory_eq(&DR(400, 449), &dst[2], sizeof(DataRegion));

    //Check that 'count' has the same results
    assert_int_eq(3, count_bounded_data_regions(set, bounds));

    //Check that the set wasn't modified
    assert_data_region_set_eq(clone, set);

    gid_free(dst);
    free_test_data_region_set(set);
    free_clone_data_region_set(clone);
  }

  Test(get_bounded_data_regions_when_boundary_totally_overlaps_several,
    EnumParam(nullDstOverflow, 0, 1)
    EnumParam(dstCapacity, 3, 4, 1000)
    EnumParam(padding, 0, 1, 2, 10))
  {
    DataRegionSet* set = create_test_data_region_set(4, 0);
    assert_add_data_region(set, 0, 99);
    assert_add_data_region(set, 200, 299);
    assert_add_data_region(set, 400, 499);
    assert_add_data_region(set, 600, 699);
    DataRegionSet* clone = clone_data_region_set(set);
    DataRegion* dst = gid_malloc(sizeof(DataRegion) * dstCapacity);

    int dstTooSmall = 5;//Initial garbage value
    int* dstTooSmallPtr = nullDstOverflow ? NULL : &dstTooSmall;

    DataRegion bounds = DR(set->regions[0].first_index - padding, set->regions[2].last_index + padding);
    assert_int_eq(3, get_bounded_data_regions(dst, dstCapacity, set, bounds, dstTooSmallPtr));
    if(!nullDstOverflow)
      assert_int_eq(0, dstTooSmall);
    assert_data_region_array_eq(dst, set->regions[0], set->regions[1], set->regions[2]);

    //Check that 'count' has the same results
    assert_int_eq(3, count_bounded_data_regions(set, bounds));

    //Check that the set wasn't modified
    assert_data_region_set_eq(clone, set);

    gid_free(dst);
    free_test_data_region_set(set);
    free_clone_data_region_set(clone);
  }

  Test(get_bounded_data_regions_when_boundary_is_subset_of_single,
    EnumParam(nullDstOverflow, 0, 1)
    EnumParam(dstCapacity, 1, 2, 3, 100)
    EnumParam(padding, 0, 1, 2, 10))
  {
    DataRegionSet* set = create_test_data_region_set(1, 0);
    assert_add_data_region(set, 0, 99);
    DataRegionSet* clone = clone_data_region_set(set);
    DataRegion* dst = gid_malloc(sizeof(DataRegion) * dstCapacity);

    int dstTooSmall = 5;//Initial garbage value
    int* dstTooSmallPtr = nullDstOverflow ? NULL : &dstTooSmall;

    DataRegion bounds = DR(set->regions[0].first_index + padding, set->regions[0].last_index - padding);
    assert_int_eq(1, get_bounded_data_regions(dst, dstCapacity, set, bounds, dstTooSmallPtr));
    if(!nullDstOverflow)
      assert_int_eq(0, dstTooSmall);
    assert_data_region_array_eq(dst, bounds);

    //Check that 'count' has the same results
    assert_int_eq(1, count_bounded_data_regions(set, bounds));

    //Check that the set wasn't modified
    assert_data_region_set_eq(clone, set);

    gid_free(dst);
    free_test_data_region_set(set);
    free_clone_data_region_set(clone);
  }

  Test(get_bounded_data_regions_when_boundary_is_subset_of_one_of_several,
    EnumParam(nullDstOverflow, 0, 1)
    EnumParam(dstCapacity, 1, 2, 3, 100)
    EnumParam(padding, 0, 1, 2, 10))
  {
    DataRegionSet* set = create_test_data_region_set(4, 0);
    assert_add_data_region(set, 0, 99);
    assert_add_data_region(set, 200, 299);
    assert_add_data_region(set, 400, 499);
    assert_add_data_region(set, 600, 699);
    DataRegionSet* clone = clone_data_region_set(set);
    DataRegion* dst = gid_malloc(sizeof(DataRegion) * dstCapacity);

    int dstTooSmall = 5;//Initial garbage value
    int* dstTooSmallPtr = nullDstOverflow ? NULL : &dstTooSmall;

    DataRegion bounds = DR(set->regions[1].first_index + padding, set->regions[1].last_index - padding);
    assert_int_eq(1, get_bounded_data_regions(dst, dstCapacity, set, bounds, dstTooSmallPtr));
    if(!nullDstOverflow)
      assert_int_eq(0, dstTooSmall);
    assert_data_region_array_eq(dst, bounds);

    //Check that 'count' has the same results
    assert_int_eq(1, count_bounded_data_regions(set, bounds));

    //Check that the set wasn't modified
    assert_data_region_set_eq(clone, set);

    gid_free(dst);
    free_test_data_region_set(set);
    free_clone_data_region_set(clone);
  }

  Test(get_bounded_data_regions_fails_when_capacity_exceeded_by_superset_boundary_of_single,
    EnumParam(nullDstOverflow, 0, 1))
  {
    DataRegionSet* set = create_test_data_region_set(1, 0);
    assert_add_data_region(set, 0, 99);
    DataRegionSet* clone = clone_data_region_set(set);
    DataRegion* dst = gid_malloc(0);

    int dstTooSmall = 5;//Initial garbage value
    int* dstTooSmallPtr = nullDstOverflow ? NULL : &dstTooSmall;

    DataRegion bounds = DR(set->regions[0].first_index - 10, set->regions[0].last_index + 10);
    assert_int_eq(0, get_bounded_data_regions(dst, 0, set, bounds, dstTooSmallPtr));
    if(!nullDstOverflow)
      assert_int_eq(1, dstTooSmall);

    //Check that 'count' has the correct number
    assert_int_eq(1, count_bounded_data_regions(set, bounds));

    //Check that the set wasn't modified
    assert_data_region_set_eq(clone, set);

    gid_free(dst);
    free_test_data_region_set(set);
    free_clone_data_region_set(clone);
  }

  Test(get_bounded_data_regions_fails_when_capacity_exceeded_by_superset_boundary_of_several,
    EnumParam(nullDstOverflow, 0, 1)
    RangeParam(deficit, 1, 4))
  {
    DataRegionSet* set = create_test_data_region_set(4, 0);
    assert_add_data_region(set, 0, 99);
    assert_add_data_region(set, 200, 299);
    assert_add_data_region(set, 400, 499);
    assert_add_data_region(set, 600, 699);
    DataRegionSet* clone = clone_data_region_set(set);
    int64_t dstCapacity = 4 - deficit;
    DataRegion* dst = gid_malloc(sizeof(DataRegion) * dstCapacity);

    int dstTooSmall = 5;//Initial garbage value
    int* dstTooSmallPtr = nullDstOverflow ? NULL : &dstTooSmall;

    DataRegion bounds = DR(set->regions[0].first_index - 10, set->regions[3].last_index + 10);
    assert_int_eq(dstCapacity, get_bounded_data_regions(dst, dstCapacity, set, bounds, dstTooSmallPtr));
    if(!nullDstOverflow)
      assert_int_eq(1, dstTooSmall);
    assert_memory_eq(dst, set->regions, sizeof(DataRegion) * dstCapacity);

    //Check that 'count' has the correct results
    assert_int_eq(4, count_bounded_data_regions(set, bounds));

    //Check that the set wasn't modified
    assert_data_region_set_eq(clone, set);

    gid_free(dst);
    free_test_data_region_set(set);
    free_clone_data_region_set(clone);
  }

  Test(get_bounded_data_regions_fails_when_capacity_exceeded_by_overlap_boundary_of_single,
    EnumParam(nullDstOverflow, 0, 1)
    EnumParam(padding, 0, 1, 2, 10))
  {
    DataRegionSet* set = create_test_data_region_set(1, 0);
    assert_add_data_region(set, 0, 99);
    DataRegionSet* clone = clone_data_region_set(set);
    DataRegion* dst = gid_malloc(0);

    int dstTooSmall = 5;//Initial garbage value
    int* dstTooSmallPtr = nullDstOverflow ? NULL : &dstTooSmall;

    DataRegion bounds = DR(set->regions[0].first_index - padding, set->regions[0].last_index + padding);
    assert_int_eq(0, get_bounded_data_regions(dst, 0, set, bounds, dstTooSmallPtr));
    if(!nullDstOverflow)
      assert_int_eq(1, dstTooSmall);

    //Check that 'count' has the correct number
    assert_int_eq(1, count_bounded_data_regions(set, bounds));

    //Check that the set wasn't modified
    assert_data_region_set_eq(clone, set);

    gid_free(dst);
    free_test_data_region_set(set);
    free_clone_data_region_set(clone);
  }

  Test(get_bounded_data_regions_fails_when_capacity_exceeded_by_overlap_boundary_of_several,
    EnumParam(nullDstOverflow, 0, 1)
    EnumParam(padding, 0, 1, 2, 10)
    RangeParam(deficit, 1, 4))
  {
    DataRegionSet* set = create_test_data_region_set(4, 0);
    assert_add_data_region(set, 0, 99);
    assert_add_data_region(set, 200, 299);
    assert_add_data_region(set, 400, 499);
    assert_add_data_region(set, 600, 699);
    DataRegionSet* clone = clone_data_region_set(set);
    int64_t dstCapacity = 4 - deficit;
    DataRegion* dst = gid_malloc(sizeof(DataRegion) * dstCapacity);

    int dstTooSmall = 5;//Initial garbage value
    int* dstTooSmallPtr = nullDstOverflow ? NULL : &dstTooSmall;

    DataRegion bounds = DR(set->regions[0].first_index - padding, set->regions[3].last_index + padding);
    assert_int_eq(dstCapacity, get_bounded_data_regions(dst, dstCapacity, set, bounds, dstTooSmallPtr));
    if(!nullDstOverflow)
      assert_int_eq(1, dstTooSmall);
    assert_memory_eq(dst, set->regions, sizeof(DataRegion) * dstCapacity);

    //Check that 'count' has the correct results
    assert_int_eq(4, count_bounded_data_regions(set, bounds));

    //Check that the set wasn't modified
    assert_data_region_set_eq(clone, set);

    gid_free(dst);
    free_test_data_region_set(set);
    free_clone_data_region_set(clone);
  }

  Test(get_bounded_data_regions_several_scenarios)
  {
    DataRegionSet* set = create_test_data_region_set(3, 0);
    int dstCapacity = 3;
    DataRegion* dst = gid_malloc(sizeof(DataRegion) * dstCapacity);

    assert_add_data_region(set, 100, 100);
    assert_add_data_region(set, 200, 300);
    assert_add_data_region(set, 400, 450);

    int dstTooSmall = 5;//Initial garbage value
    assert_int_eq(3, get_bounded_data_regions(dst, dstCapacity, set, DR(0, 500), &dstTooSmall));
    assert_data_region_array_eq(dst, DR(100, 100), DR(200, 300), DR(400, 450));
    assert_int_eq(0, dstTooSmall);

    assert_int_eq(1, get_bounded_data_regions(dst, dstCapacity, set, DR(100, 100), &dstTooSmall));
    assert_data_region_array_eq(dst, DR(100, 100));
    assert_int_eq(0, dstTooSmall);

    assert_int_eq(1, get_bounded_data_regions(dst, dstCapacity, set, DR(99, 101), &dstTooSmall));
    assert_data_region_array_eq(dst, DR(100, 100));
    assert_int_eq(0, dstTooSmall);

    assert_int_eq(2, get_bounded_data_regions(dst, dstCapacity, set, DR(100, 200), &dstTooSmall));
    assert_data_region_array_eq(dst, DR(100, 100), DR(200, 200));
    assert_int_eq(0, dstTooSmall);

    assert_int_eq(2, get_bounded_data_regions(dst, dstCapacity, set, DR(100, 202), &dstTooSmall));
    assert_data_region_array_eq(dst, DR(100, 100), DR(200, 202));
    assert_int_eq(0, dstTooSmall);

    assert_int_eq(2, get_bounded_data_regions(dst, dstCapacity, set, DR(99, 200), &dstTooSmall));
    assert_data_region_array_eq(dst, DR(100, 100), DR(200, 200));
    assert_int_eq(0, dstTooSmall);

    assert_int_eq(2, get_bounded_data_regions(dst, dstCapacity, set, DR(98, 202), &dstTooSmall));
    assert_data_region_array_eq(dst, DR(100, 100), DR(200, 202));
    assert_int_eq(0, dstTooSmall);

    assert_int_eq(1, get_bounded_data_regions(dst, dstCapacity, set, DR(200, 300), &dstTooSmall));
    assert_data_region_array_eq(dst, DR(200, 300));
    assert_int_eq(0, dstTooSmall);

    assert_int_eq(1, get_bounded_data_regions(dst, dstCapacity, set, DR(201, 299), &dstTooSmall));
    assert_data_region_array_eq(dst, DR(201, 299));
    assert_int_eq(0, dstTooSmall);

    assert_int_eq(1, get_bounded_data_regions(dst, dstCapacity, set, DR(210, 250), &dstTooSmall));
    assert_data_region_array_eq(dst, DR(210, 250));
    assert_int_eq(0, dstTooSmall);

    assert_int_eq(0, get_bounded_data_regions(dst, dstCapacity, set, DR(350, 375), &dstTooSmall));
    assert_int_eq(0, dstTooSmall);

    assert_int_eq(1, get_bounded_data_regions(dst, dstCapacity, set, DR(350, 410), &dstTooSmall));
    assert_data_region_array_eq(dst, DR(400, 410));
    assert_int_eq(0, dstTooSmall);

    assert_int_eq(1, get_bounded_data_regions(dst, dstCapacity, set, DR(350, 1500), &dstTooSmall));
    assert_data_region_array_eq(dst, DR(400, 450));
    assert_int_eq(0, dstTooSmall);

    gid_free(dst);
    free_test_data_region_set(set);
  }

END_TEST_SUITE()


BEGIN_TEST_SUITE(DataRegionSetGetMissingDataRegionsTests)

  Test(get_missing_data_regions_when_dst_NULL,
    EnumParam(nullDstOverflow, 0, 1)
    EnumParam(count, 1, 2, 3, 1000)
    EnumParam(dstCapacity, 1, 2, 1000))
  {
    DataRegionSet* set = create_test_data_region_set(count, count);
    DataRegionSet* clone = clone_data_region_set(set);

    int dstTooSmall = 5;//Initial garbage value
    int* dstTooSmallPtr = nullDstOverflow ? NULL : &dstTooSmall;

    assert_int_eq(0, get_missing_data_regions(NULL, dstCapacity, set, DR(-1000000, 1000000), dstTooSmallPtr));
    if(!nullDstOverflow)
      assert_int_eq(0, dstTooSmall);

    //Check that the set wasn't modified
    assert_data_region_set_eq(clone, set);

    free_clone_data_region_set(clone);
    free_test_data_region_set(set);
  }

  Test(get_missing_data_regions_when_dstCapacity_negative,
    EnumParam(nullDstOverflow, 0, 1)
    EnumParam(count, 1, 2, 3, 1000)
    EnumParam(dstCapacity, -1, -2, -3, -10000))
  {
    DataRegionSet* set = create_test_data_region_set(count, count);
    DataRegionSet* clone = clone_data_region_set(set);
    DataRegion* dst = gid_malloc(0);

    int dstTooSmall = 5;//Initial garbage value
    int* dstTooSmallPtr = nullDstOverflow ? NULL : &dstTooSmall;

    assert_int_eq(0, get_missing_data_regions(dst, dstCapacity, set, DR(-1000000, 1000000), dstTooSmallPtr));
    if(!nullDstOverflow)
      assert_int_eq(1, dstTooSmall);

    //Check that the set wasn't modified
    assert_data_region_set_eq(clone, set);

    gid_free(dst);
    free_clone_data_region_set(clone);
    free_test_data_region_set(set);
  }

  Test(get_missing_data_regions_when_src_NULL,
    EnumParam(nullDstOverflow, 0, 1)
    EnumParam(dstCapacity, 0, 1, 2, 3, 1000))
  {
    int dstTooSmall = 5;//Initial garbage value
    int* dstTooSmallPtr = nullDstOverflow ? NULL : &dstTooSmall;

    DataRegion* dst = gid_malloc(0);//Allocate nothing because we expect it to fail
    assert_int_eq(0, get_missing_data_regions(dst, dstCapacity, NULL, DR(-1000000, 1000000), dstTooSmallPtr));
    if(!nullDstOverflow)
      assert_int_eq(0, dstTooSmall);

    gid_free(dst);
  }

  Test(get_missing_data_regions_when_boundary_invalid,
    EnumParam(nullDstOverflow, 0, 1)
    EnumParam(count, 0, 1, 2, 3, 1000)
    EnumParam(dstCapacity, 0, 1, 2, 1000))
  {
    DataRegionSet* set = create_test_data_region_set(count, count);
    DataRegionSet* clone = clone_data_region_set(set);
    DataRegion* dst = gid_malloc(0);

    int dstTooSmall = 5;//Initial garbage value
    int* dstTooSmallPtr = nullDstOverflow ? NULL : &dstTooSmall;

    assert_int_eq(0, get_missing_data_regions(dst, dstCapacity, set, DR(1000000, 1), dstTooSmallPtr));
    if(!nullDstOverflow)
      assert_int_eq(0, dstTooSmall);

    //Check that the set wasn't modified
    assert_data_region_set_eq(clone, set);

    gid_free(dst);
    free_clone_data_region_set(clone);
    free_test_data_region_set(set);
  }

  Test(get_missing_data_regions_when_src_empty,
    EnumParam(nullDstOverflow, 0, 1)
    EnumParam(dstCapacity, 1, 2, 3, 1000)
    EnumParam(srcCapacity, 0, 1, 2, 1000)
    IntRow(0, 0)
    IntRow(0, 1)
    IntRow(-1, 0)
    IntRow(-1, 1)
    IntRow(1, 100))
  {
    DataRegionSet* set = create_test_data_region_set(srcCapacity, 0);
    DataRegionSet* clone = clone_data_region_set(set);

    DataRegion* dst = gid_malloc(0);

    int dstTooSmall = 5;//Initial garbage value
    int* dstTooSmallPtr = nullDstOverflow ? NULL : &dstTooSmall;

    DataRegion bounds = DR(int_row[0], int_row[1]);
    assert_int_eq(1, get_missing_data_regions(dst, dstCapacity, set, bounds, dstTooSmallPtr));
    if(!nullDstOverflow)
      assert_int_eq(0, dstTooSmall);
    assert_data_region_array_eq(dst, bounds);

    //Check that the set wasn't modified
    assert_data_region_set_eq(clone, set);

    free_test_data_region_set(set);
    free_clone_data_region_set(clone);
  }

  Test(get_missing_data_regions_when_dst_capacity_zero_nothing_missing_in_bounds,
    EnumParam(nullDstOverflow, 0, 1))
  {
    DataRegionSet* set = create_test_data_region_set(10, 0);
    assert_add_data_region(set, 0, 1000000);
    DataRegionSet* clone = clone_data_region_set(set);
    DataRegion* dst = gid_malloc(0);

    int dstTooSmall = 5;//Initial garbage value
    int* dstTooSmallPtr = nullDstOverflow ? NULL : &dstTooSmall;

    DataRegion bounds = DR(1, 5);//Nothing missing in (1,5), so expect return 0
    assert_int_eq(0, get_missing_data_regions(dst, 0, set, bounds, dstTooSmallPtr));
    if(!nullDstOverflow)
      assert_int_eq(1, dstTooSmall);
    //dstTooSmall is only true because the implementation needs at least one DataRegion

    //Check that the set wasn't modified
    assert_data_region_set_eq(clone, set);

    gid_free(dst);
    free_test_data_region_set(set);
    free_clone_data_region_set(clone);
  }

  Test(get_missing_data_regions_when_dst_capacity_zero_nothing_in_bounds,
    EnumParam(nullDstOverflow, 0, 1))
  {
    DataRegionSet* set = create_test_data_region_set(10, 0);
    DataRegionSet* clone = clone_data_region_set(set);
    DataRegion* dst = gid_malloc(0);

    int dstTooSmall = 5;//Initial garbage value
    int* dstTooSmallPtr = nullDstOverflow ? NULL : &dstTooSmall;

    DataRegion bounds = DR(1, 5);//Nothing contained in (1,5)
    assert_int_eq(0, get_missing_data_regions(dst, 0, set, bounds, dstTooSmallPtr));
    if(!nullDstOverflow)
      assert_int_eq(1, dstTooSmall);

    //Check that the set wasn't modified
    assert_data_region_set_eq(clone, set);

    gid_free(dst);
    free_test_data_region_set(set);
    free_clone_data_region_set(clone);
  }

  Test(get_missing_data_regions_when_boundary_contains_nothing,
    EnumParam(nullDstOverflow, 0, 1)
    EnumParam(dstCapacity, 1, 2, 3, 1000))
  {
    DataRegionSet* set = create_test_data_region_set(10, 0);
    DataRegionSet* clone = clone_data_region_set(set);
    DataRegion* dst = gid_malloc(sizeof(DataRegion) * dstCapacity);

    int dstTooSmall = 5;//Initial garbage value
    int* dstTooSmallPtr = nullDstOverflow ? NULL : &dstTooSmall;

    DataRegion bounds = DR(1, 5);//Nothing contained in (1,5)
    assert_int_eq(1, get_missing_data_regions(dst, dstCapacity, set, bounds, dstTooSmallPtr));
    if(!nullDstOverflow)
      assert_int_eq(0, dstTooSmall);
    assert_data_region_array_eq(dst, bounds);

    //Check that the set wasn't modified
    assert_data_region_set_eq(clone, set);

    gid_free(dst);
    free_test_data_region_set(set);
    free_clone_data_region_set(clone);
  }

  Test(get_missing_data_regions_when_boundary_partially_contains_single,
    EnumParam(nullDstOverflow, 0, 1)
    EnumParam(dstCapacity, 1, 2, 3, 1000)
    EnumParam(toLeft, 0, 1))
  {
    DataRegionSet* set = create_test_data_region_set(10, 0);
    assert_add_data_region(set, 10, 19);
    DataRegionSet* clone = clone_data_region_set(set);
    DataRegion* dst = gid_malloc(sizeof(DataRegion) * dstCapacity);

    int dstTooSmall = 5;//Initial garbage value
    int* dstTooSmallPtr = nullDstOverflow ? NULL : &dstTooSmall;

    DataRegion bounds = toLeft ? DR(0, 14) : DR(15, 25);
    assert_int_eq(1, get_missing_data_regions(dst, dstCapacity, set, bounds, dstTooSmallPtr));
    if(!nullDstOverflow)
      assert_int_eq(0, dstTooSmall);
    DataRegion expect = toLeft ? DR(0, 9) : DR(20, 25);
    assert_data_region_array_eq(dst, expect);

    //Check that the set wasn't modified
    assert_data_region_set_eq(clone, set);

    gid_free(dst);
    free_test_data_region_set(set);
    free_clone_data_region_set(clone);
  }

  Test(get_missing_data_regions_when_boundary_partially_contains_one_of_several,
    EnumParam(nullDstOverflow, 0, 1)
    EnumParam(dstCapacity, 1, 2, 3, 1000)
    EnumParam(toLeft, 0, 1)
    RangeParam(relativeIndex, 0, 9))
  {
    DataRegionSet* set = create_test_data_region_set(10, 10);
    DataRegion relative = set->regions[relativeIndex];
    DataRegionSet* clone = clone_data_region_set(set);
    DataRegion* dst = gid_malloc(sizeof(DataRegion) * dstCapacity);

    int dstTooSmall = 5;//Initial garbage value
    int* dstTooSmallPtr = nullDstOverflow ? NULL : &dstTooSmall;

    DataRegion bounds = toLeft ? DR(relative.first_index - 5, relative.first_index + 5) : DR(relative.last_index - 5, relative.last_index + 5);
    assert_int_eq(1, get_missing_data_regions(dst, dstCapacity, set, bounds, dstTooSmallPtr));
    if(!nullDstOverflow)
      assert_int_eq(0, dstTooSmall);
    DataRegion expect = toLeft ? DR(bounds.first_index, relative.first_index - 1) : DR(relative.last_index + 1, bounds.last_index);
    assert_data_region_array_eq(dst, expect);

    //Check that the set wasn't modified
    assert_data_region_set_eq(clone, set);

    gid_free(dst);
    free_test_data_region_set(set);
    free_clone_data_region_set(clone);
  }

  Test(get_missing_data_regions_when_boundary_partially_contains_several,
    EnumParam(nullDstOverflow, 0, 1)
    EnumParam(dstCapacity, 2, 3, 100)
    EnumParam(padding, 0, 1, 2, 3, 10))
  {
    DataRegionSet* set = create_test_data_region_set(10, 0);
    assert_add_data_region(set, 0, 99);
    assert_add_data_region(set, 300, 399);
    assert_add_data_region(set, 500, 599);
    assert_add_data_region(set, 700, 799);
    assert_add_data_region(set, 900, 999);
    DataRegionSet* clone = clone_data_region_set(set);
    DataRegion* dst = gid_malloc(sizeof(DataRegion) * dstCapacity);

    int dstTooSmall = 5;//Initial garbage value
    int* dstTooSmallPtr = nullDstOverflow ? NULL : &dstTooSmall;

    DataRegion bounds = DR(300 + padding, 799 - padding);
    assert_int_eq(2, get_missing_data_regions(dst, dstCapacity, set, bounds, dstTooSmallPtr));
    if(!nullDstOverflow)
      assert_int_eq(0, dstTooSmall);

    assert_data_region_array_eq(dst, DR(400, 499), DR(600, 699));

    //Check that the set wasn't modified
    assert_data_region_set_eq(clone, set);

    gid_free(dst);
    free_test_data_region_set(set);
    free_clone_data_region_set(clone);
  }

  Test(get_missing_data_regions_when_boundary_is_proper_subset_of_single,
    EnumParam(nullDstOverflow, 0, 1)
    EnumParam(dstCapacity, 1, 2, 3, 100)
    EnumParam(padding, 0, 1, 2, 4))
  {
    DataRegionSet* set = create_test_data_region_set(10, 0);
    assert_add_data_region(set, 0, 99);
    DataRegionSet* clone = clone_data_region_set(set);
    DataRegion* dst = gid_malloc(sizeof(DataRegion) * dstCapacity);

    int dstTooSmall = 5;//Initial garbage value
    int* dstTooSmallPtr = nullDstOverflow ? NULL : &dstTooSmall;

    DataRegion bounds = DR(set->regions[0].first_index + padding, set->regions[0].last_index - padding);
    assert_int_eq(0, get_missing_data_regions(dst, dstCapacity, set, bounds, dstTooSmallPtr));
    if(!nullDstOverflow)
      assert_int_eq(0, dstTooSmall);

    //Check that the set wasn't modified
    assert_data_region_set_eq(clone, set);

    gid_free(dst);
    free_test_data_region_set(set);
    free_clone_data_region_set(clone);
  }

  Test(get_missing_data_regions_when_boundary_is_proper_subset_of_one_of_several,
    EnumParam(nullDstOverflow, 0, 1)
    EnumParam(dstCapacity, 1, 2, 3, 100)
    EnumParam(padding, 0, 1, 2, 4)
    RangeParam(relativeIndex, 0, 4))
  {
    DataRegionSet* set = create_test_data_region_set(10, 0);
    assert_add_data_region(set, 0, 99);
    assert_add_data_region(set, 300, 399);
    assert_add_data_region(set, 500, 599);
    assert_add_data_region(set, 700, 799);
    assert_add_data_region(set, 900, 999);
    DataRegionSet* clone = clone_data_region_set(set);
    DataRegion* dst = gid_malloc(sizeof(DataRegion) * dstCapacity);

    int dstTooSmall = 5;//Initial garbage value
    int* dstTooSmallPtr = nullDstOverflow ? NULL : &dstTooSmall;

    DataRegion relative = set->regions[relativeIndex];
    DataRegion bounds = DR(relative.first_index + padding, relative.last_index - padding);
    assert_int_eq(0, get_missing_data_regions(dst, dstCapacity, set, bounds, dstTooSmallPtr));
    if(!nullDstOverflow)
      assert_int_eq(0, dstTooSmall);

    //Check that the set wasn't modified
    assert_data_region_set_eq(clone, set);

    gid_free(dst);
    free_test_data_region_set(set);
    free_clone_data_region_set(clone);
  }

  Test(get_missing_data_regions_when_boundary_totally_contains_single,
    EnumParam(nullDstOverflow, 0, 1)
    EnumParam(dstCapacity, 2, 3, 4, 100)
    EnumParam(padding, 1, 2, 10))
  {
    DataRegionSet* set = create_test_data_region_set(10, 0);
    assert_add_data_region(set, 0, 99);
    DataRegionSet* clone = clone_data_region_set(set);
    DataRegion* dst = gid_malloc(sizeof(DataRegion) * dstCapacity);

    int dstTooSmall = 5;//Initial garbage value
    int* dstTooSmallPtr = nullDstOverflow ? NULL : &dstTooSmall;

    DataRegion bounds = DR(set->regions[0].first_index - 10, set->regions[0].last_index + 10);
    assert_int_eq(2, get_missing_data_regions(dst, dstCapacity, set, bounds, dstTooSmallPtr));
    if(!nullDstOverflow)
      assert_int_eq(0, dstTooSmall);
    assert_data_region_array_eq(dst, DR(bounds.first_index, set->regions[0].first_index - 1), DR(set->regions[0].last_index + 1, bounds.last_index));

    //Check that the set wasn't modified
    assert_data_region_set_eq(clone, set);

    gid_free(dst);
    free_test_data_region_set(set);
    free_clone_data_region_set(clone);
  }

  Test(get_missing_data_regions_when_boundary_totally_contains_single_equal_to_bounds,
    EnumParam(nullDstOverflow, 0, 1)
    EnumParam(dstCapacity, 2, 3, 4, 100)
    EnumParam(padding, 1, 2, 10))
  {
    DataRegionSet* set = create_test_data_region_set(10, 0);
    assert_add_data_region(set, 0, 99);
    DataRegionSet* clone = clone_data_region_set(set);
    DataRegion* dst = gid_malloc(sizeof(DataRegion) * dstCapacity);

    int dstTooSmall = 5;//Initial garbage value
    int* dstTooSmallPtr = nullDstOverflow ? NULL : &dstTooSmall;

    DataRegion bounds = set->regions[0];
    assert_int_eq(0, get_missing_data_regions(dst, dstCapacity, set, bounds, dstTooSmallPtr));
    if(!nullDstOverflow)
      assert_int_eq(0, dstTooSmall);

    //Check that the set wasn't modified
    assert_data_region_set_eq(clone, set);

    gid_free(dst);
    free_test_data_region_set(set);
    free_clone_data_region_set(clone);
  }

  Test(get_missing_data_regions_when_boundary_totally_contains_one_of_several,
    EnumParam(nullDstOverflow, 0, 1)
    EnumParam(dstCapacity, 2, 3, 4, 100)
    RangeParam(relativeIndex, 0, 4)
    EnumParam(padding, 1, 2, 3))
  {
    DataRegionSet* set = create_test_data_region_set(10, 0);
    assert_add_data_region(set, 0, 99);
    assert_add_data_region(set, 300, 399);
    assert_add_data_region(set, 500, 599);
    assert_add_data_region(set, 700, 799);
    assert_add_data_region(set, 900, 999);
    DataRegionSet* clone = clone_data_region_set(set);
    DataRegion* dst = gid_malloc(sizeof(DataRegion) * dstCapacity);

    int dstTooSmall = 5;//Initial garbage value
    int* dstTooSmallPtr = nullDstOverflow ? NULL : &dstTooSmall;

    DataRegion relative = set->regions[relativeIndex];
    DataRegion bounds = DR(relative.first_index - padding, relative.last_index + padding);
    assert_int_eq(2, get_missing_data_regions(dst, dstCapacity, set, bounds, dstTooSmallPtr));
    if(!nullDstOverflow)
      assert_int_eq(0, dstTooSmall);
    assert_data_region_array_eq(dst, DR(bounds.first_index, relative.first_index - 1), DR(relative.last_index + 1, bounds.last_index));

    //Check that the set wasn't modified
    assert_data_region_set_eq(clone, set);

    gid_free(dst);
    free_test_data_region_set(set);
    free_clone_data_region_set(clone);
  }

  Test(get_missing_data_regions_when_boundary_totally_contains_several,
    EnumParam(nullDstOverflow, 0, 1)
    EnumParam(dstCapacity, 4, 5, 6, 100)
    EnumParam(padding, 1, 2, 3, 10))
  {
    DataRegionSet* set = create_test_data_region_set(10, 0);
    assert_add_data_region(set, 0, 99);
    assert_add_data_region(set, 300, 399);
    assert_add_data_region(set, 500, 599);
    assert_add_data_region(set, 700, 799);
    assert_add_data_region(set, 900, 999);
    DataRegionSet* clone = clone_data_region_set(set);
    DataRegion* dst = gid_malloc(sizeof(DataRegion) * dstCapacity);

    int dstTooSmall = 5;//Initial garbage value
    int* dstTooSmallPtr = nullDstOverflow ? NULL : &dstTooSmall;

    DataRegion first = set->regions[1];
    DataRegion last = set->regions[3];
    DataRegion bounds = DR(first.first_index - padding, last.last_index + padding);
    assert_int_eq(4, get_missing_data_regions(dst, dstCapacity, set, bounds, dstTooSmallPtr));
    if(!nullDstOverflow)
      assert_int_eq(0, dstTooSmall);
    assert_data_region_array_eq(dst, DR(bounds.first_index, first.first_index - 1), DR(400, 499), DR(600, 699), DR(last.last_index + 1, bounds.last_index));

    //Check that the set wasn't modified
    assert_data_region_set_eq(clone, set);

    gid_free(dst);
    free_test_data_region_set(set);
    free_clone_data_region_set(clone);
  }

  Test(get_missing_data_regions_fails_when_capacity_exceeded,
    EnumParam(nullDstOverflow, 0, 1)
    EnumParam(dstCapacity, 0, 1, 2, 3))
  {
    DataRegionSet* set = create_test_data_region_set(10, 0);
    assert_add_data_region(set, 0, 99);
    assert_add_data_region(set, 300, 399);
    assert_add_data_region(set, 500, 599);
    assert_add_data_region(set, 700, 799);
    assert_add_data_region(set, 900, 999);
    DataRegionSet* clone = clone_data_region_set(set);
    DataRegion* dst = gid_malloc(sizeof(DataRegion) * dstCapacity);

    int dstTooSmall = 5;//Initial garbage value
    int* dstTooSmallPtr = nullDstOverflow ? NULL : &dstTooSmall;

    DataRegion first = set->regions[1];
    DataRegion last = set->regions[3];
    DataRegion bounds = DR(first.first_index - 10, last.last_index + 10);
    assert_int_eq(0, get_missing_data_regions(dst, dstCapacity, set, bounds, dstTooSmallPtr));
    if(!nullDstOverflow)
      assert_int_eq(1, dstTooSmall);

    //Check that the set wasn't modified
    assert_data_region_set_eq(clone, set);

    gid_free(dst);
    free_test_data_region_set(set);
    free_clone_data_region_set(clone);
  }

  Test(get_missing_data_regions_several_scenarios)
  {
    DataRegionSet* set = create_test_data_region_set(100, 0);
    size_t dstCapacity = 5;
    DataRegion* dst = gid_malloc(sizeof(DataRegion) * dstCapacity);
    int dstTooSmall = 5;//Initial garbage value

    assert_int_eq(1, get_missing_data_regions(dst, dstCapacity, set, DR(0, 0), &dstTooSmall));
    assert_int_eq(0, dstTooSmall);
    assert_data_region_array_eq(dst, DR(0, 0));

    assert_int_eq(1, get_missing_data_regions(dst, dstCapacity, set, DR(1, 1), &dstTooSmall));
    assert_int_eq(0, dstTooSmall);
    assert_data_region_array_eq(dst, DR(1, 1));

    assert_int_eq(1, get_missing_data_regions(dst, dstCapacity, set, DR(0, 20), &dstTooSmall));
    assert_int_eq(0, dstTooSmall);
    assert_data_region_array_eq(dst, DR(0, 20));

    assert_add_data_region(set, 0, 20);

    assert_int_eq(0, get_missing_data_regions(dst, dstCapacity, set, DR(0, 0), &dstTooSmall));
    assert_int_eq(0, dstTooSmall);

    assert_int_eq(0, get_missing_data_regions(dst, dstCapacity, set, DR(1, 1), &dstTooSmall));
    assert_int_eq(0, dstTooSmall);

    assert_int_eq(0, get_missing_data_regions(dst, dstCapacity, set, DR(0, 20), &dstTooSmall));
    assert_int_eq(0, dstTooSmall);

    assert_int_eq(1, get_missing_data_regions(dst, dstCapacity, set, DR(0, 21), &dstTooSmall));
    assert_int_eq(0, dstTooSmall);
    assert_data_region_array_eq(dst, DR(21, 21));

    assert_int_eq(1, get_missing_data_regions(dst, dstCapacity, set, DR(20, 21), &dstTooSmall));
    assert_int_eq(0, dstTooSmall);
    assert_data_region_array_eq(dst, DR(21, 21));

    assert_remove_data_region(set, 0, 5);

    assert_int_eq(2, get_missing_data_regions(dst, dstCapacity, set, DR(0, 21), &dstTooSmall));
    assert_int_eq(0, dstTooSmall);
    assert_data_region_array_eq(dst, DR(0, 5), DR(21,21));

    assert_int_eq(1, get_missing_data_regions(dst, dstCapacity, set, DR(1, 19), &dstTooSmall));
    assert_int_eq(0, dstTooSmall);
    assert_data_region_array_eq(dst, DR(1, 5));

    assert_int_eq(2, get_missing_data_regions(dst, dstCapacity, set, DR(1, 29), &dstTooSmall));
    assert_int_eq(0, dstTooSmall);
    assert_data_region_array_eq(dst, DR(1, 5), DR(21, 29));

    assert_remove_data_region(set, 10, 10);

    assert_int_eq(3, get_missing_data_regions(dst, dstCapacity, set, DR(1, 29), &dstTooSmall));
    assert_int_eq(0, dstTooSmall);
    assert_data_region_array_eq(dst, DR(1, 5), DR(10, 10), DR(21, 29));

    assert_int_eq(1, get_missing_data_regions(dst, dstCapacity, set, DR(0, 1), &dstTooSmall));
    assert_int_eq(0, dstTooSmall);
    assert_data_region_array_eq(dst, DR(0, 1));

    assert_int_eq(3, get_missing_data_regions(dst, dstCapacity, set, DR(0, 29), &dstTooSmall));
    assert_int_eq(0, dstTooSmall);
    assert_data_region_array_eq(dst, DR(0, 5), DR(10, 10), DR(21, 29));

    assert_int_eq(2, get_missing_data_regions(dst, dstCapacity, set, DR(0, 15), &dstTooSmall));
    assert_int_eq(0, dstTooSmall);
    assert_data_region_array_eq(dst, DR(0, 5), DR(10, 10));

    assert_int_eq(2, get_missing_data_regions(dst, dstCapacity, set, DR(1, 10), &dstTooSmall));
    assert_int_eq(0, dstTooSmall);
    assert_data_region_array_eq(dst, DR(1, 5), DR(10, 10));

    assert_int_eq(1, get_missing_data_regions(dst, dstCapacity, set, DR(21, 29), &dstTooSmall));
    assert_int_eq(0, dstTooSmall);
    assert_data_region_array_eq(dst, DR(21, 29));

    assert_int_eq(1, get_missing_data_regions(dst, dstCapacity, set, DR(10, 10), &dstTooSmall));
    assert_int_eq(0, dstTooSmall);
    assert_data_region_array_eq(dst, DR(10, 10));

    assert_int_eq(1, get_missing_data_regions(dst, dstCapacity, set, DR(1000, 1000), &dstTooSmall));
    assert_int_eq(0, dstTooSmall);
    assert_data_region_array_eq(dst, DR(1000, 1000));
  }

  Test(get_missing_data_regions_several_failure_scenarios,
    EnumParam(dstCapacity, 0, 1, 2))
  {
    DataRegionSet* set = create_test_data_region_set(100, 0);
    assert_add_data_region(set, 1, 1);
    assert_add_data_region(set, 3, 5);
    assert_add_data_region(set, 9, 10);
    assert_add_data_region(set, 100, 199);
    assert_add_data_region(set, 300, 399);
    assert_add_data_region(set, 500, 599);
    assert_add_data_region(set, 700, 799);
    DataRegion* dst = gid_malloc(sizeof(DataRegion) * dstCapacity);

    int dstTooSmall = 5;//Initial garbage value
    assert_int_eq(0, get_missing_data_regions(dst, dstCapacity, set, DR(0, 11), &dstTooSmall));
    assert_int_eq(1, dstTooSmall);

    assert_int_eq(0, get_missing_data_regions(dst, dstCapacity, set, DR(-1000, 1000), &dstTooSmall));
    assert_int_eq(1, dstTooSmall);

    assert_int_eq(0, get_missing_data_regions(dst, dstCapacity, set, DR(100, 799), &dstTooSmall));
    assert_int_eq(1, dstTooSmall);

    assert_int_eq(0, get_missing_data_regions(dst, dstCapacity, set, DR(99, 800), &dstTooSmall));
    assert_int_eq(1, dstTooSmall);

    assert_int_eq(0, get_missing_data_regions(dst, dstCapacity, set, DR(101, 701), &dstTooSmall));
    assert_int_eq(1, dstTooSmall);

    free_test_data_region_set(set);
    gid_free(dst);
  }

END_TEST_SUITE()



int main()
{
  ADD_TEST_SUITE(DataRegionSetInitialization);
  ADD_TEST_SUITE(DataRegionFunctionTests);
  ADD_TEST_SUITE(DataRegionSetFunctionTests);
  ADD_TEST_SUITE(DataRegionSetAddTests);
  ADD_TEST_SUITE(DataRegionSetRemoveTests);
  ADD_TEST_SUITE(DataRegionSetGetBoundedDataRegionsTests);
  ADD_TEST_SUITE(DataRegionSetGetMissingDataRegionsTests);

  return gidunit();
}
