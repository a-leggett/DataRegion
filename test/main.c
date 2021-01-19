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
  for(int64_t i = 0; i < set->count; i++)
    ret->regions[i] = set->regions[i];
  return ret;
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

  Test(add_data_region_many_scenarios)
  {
    assert_fail("Not Implemented");
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

  Test(remove_data_region_several_scenarios)
  {
    assert_fail("Not Implemented");
  }

END_TEST_SUITE()

BEGIN_TEST_SUITE(DataRegionSetGetBoundedDataRegionsTests)

  //TODO: All of these tests need to run case where 'dstTooSmall' is NULL or non-NULL
  //TODO: Also, all of these tests need to call count_bounded_data_regions to verify

  Test(get_bounded_data_regions_when_src_NULL)
  {
    assert_fail("Not Implemented");
  }

  Test(get_bounded_data_regions_when_boundary_is_invalid)
  {
    assert_fail("Not Implemented");
  }

  Test(get_bounded_data_regions_when_boundary_contains_nothing)
  {
    assert_fail("Not Implemented");
  }

  Test(get_bounded_data_regions_when_boundary_adjacent_to_single)
  {
      assert_fail("Not Implemented");
  }

  Test(get_bounded_data_regions_when_boundary_adjacent_to_several)
  {
    assert_fail("Not Implemented");
  }

  Test(get_bounded_data_regions_when_boundary_partially_overlaps_single)
  {
    assert_fail("Not Implemented");
  }

  Test(get_bounded_data_regions_when_boundary_partially_overlaps_several)
  {
    assert_fail("Not Implemented");
  }

  Test(get_bounded_data_regions_when_boundary_totally_overlaps_single)
  {
    assert_fail("Not Implemented");
  }

  Test(get_bounded_data_regions_when_boundary_totally_overlaps_one_and_partially_overlaps_other_of_several)
  {
    assert_fail("Not Implemented");
  }

  Test(get_bounded_data_regions_when_boundary_totally_overlaps_several)
  {
    assert_fail("Not Implemented");
  }

  Test(get_bounded_data_regions_when_boundary_is_subset_of_single)
  {
    assert_fail("Not Implemented");
  }

  Test(get_bounded_data_regions_when_boundary_is_subset_of_one_of_several)
  {
    assert_fail("Not Implemented");
  }

  Test(get_bounded_data_regions_fails_when_capacity_exceeded_by_superset_boundary_of_single)
  {
    assert_fail("Not Implemented");
  }

  Test(get_bounded_data_regions_fails_when_capacity_exceeded_by_superset_boundary_of_several)
  {
    assert_fail("Not Implemented");
  }

  Test(get_bounded_data_regions_fails_when_capacity_exceeded_by_overlap_boundary_of_single)
  {
    assert_fail("Not Implemented");
  }

  Test(get_bounded_data_regions_fails_when_capacity_exceeded_by_overlap_boundary_of_several)
  {
    assert_fail("Not Implemented");
  }

  Test(get_bounded_data_regions_several_scenarios)
  {
    assert_fail("Not Implemented");
  }

END_TEST_SUITE()


BEGIN_TEST_SUITE(DataRegionSetGetMissingDataRegionsTests)

  Test(get_missing_data_regions_when_dst_NULL)
  {
    assert_fail("Not Implemented");
  }

  Test(get_missing_data_regions_when_src_NULL)
  {
    assert_fail("Not Implemented");
  }

  Test(get_missing_data_regions_when_boundary_invalid)
  {
    assert_fail("Not Implemented");
  }

  Test(get_missing_data_regions_when_src_empty)
  {
    assert_fail("Not Implemented");
  }

  Test(get_missing_data_regions_when_dst_capacity_zero)
  {
    assert_fail("Not Implemented");
  }

  Test(get_missing_data_regions_when_boundary_contains_nothing)
  {
    assert_fail("Not Implemented");
  }

  Test(get_missing_data_regions_when_boundary_partially_contains_single)
  {
    assert_fail("Not Implemented");
  }

  Test(get_missing_data_regions_when_boundary_partially_contains_one_of_several)
  {
    assert_fail("Not Implemented");
  }

  Test(get_missing_data_regions_when_boundary_partially_contains_several)
  {
    assert_fail("Not Implemented");
  }

  Test(get_missing_data_regions_when_boundary_is_proper_subset_of_single)
  {
    assert_fail("Not Implemented");
  }

  Test(get_missing_data_regions_when_boundary_is_proper_subset_of_one_of_several)
  {
    assert_fail("Not Implemented");
  }

  Test(get_missing_data_regions_when_boundary_totally_contains_single)
  {
    assert_fail("Not Implemented");
  }

  Test(get_missing_data_regions_when_boundary_totally_contains_one_of_several)
  {
    assert_fail("Not Implemented");
  }

  Test(get_missing_data_regions_when_boundary_totally_contains_several)
  {
    assert_fail("Not Implemented");
  }

  Test(get_missing_data_regions_stops_when_capacity_exceeded)
  {
    assert_fail("Not Implemented");
  }

  //TODO: Also test 'edge' cases (literal edge cases, the endpoints, that is, things can go wrong at the first and last region, test against these!)

  Test(get_missing_data_regions_several_scenarios)
  {
    assert_fail("Not Implemented");
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
