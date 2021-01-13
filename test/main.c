#include "gidunit.h"
#include "../data_region.h"

BEGIN_TEST_SUITE(DataRegionSetInitialization)

  Test(create_data_region_set_in_when_dst_is_null,
    RangeParam(size, 0, sizeof(DataRegionSet)+128))
  {
    DataRegionSet* got = create_data_region_set_in(NULL, size);
    assert_null(got);
  }

  Test(create_data_region_set_in_when_size_too_small_for_struct,
    RangeParam(deficit, 1, sizeof(DataRegionSet)))
  {
    size_t memSize = sizeof(DataRegionSet) - deficit;
    uint8_t* mem = malloc(memSize);

    //Write a signature to the memory
    for(int i = 0; i < memSize; i++)
      mem[i] = (uint8_t)i;

    //Try to create a DataRegionSet in too-small memory
    DataRegionSet* got = create_data_region_set_in(mem, memSize);
    assert_null(got);

    //Check that the memory wasn't corrupted by this failed attempt
    for(size_t i = 0; i < memSize; i++)
      assert_int_eq((uint8_t)i, mem[i]);
  }

END_TEST_SUITE()

int main()
{
  ADD_TEST_SUITE(DataRegionSetInitialization);
  return gidunit();
}
