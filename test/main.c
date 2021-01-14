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

int main()
{
  ADD_TEST_SUITE(DataRegionSetInitialization);
  ADD_TEST_SUITE(DataRegionFunctionTests);
  ADD_TEST_SUITE(DataRegionSetFunctionTests);
  return gidunit();
}
