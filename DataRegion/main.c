#define _CRT_SECURE_NO_WARNINGS
#include "data_region.h"
#include "data_region_test.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main()
{
	test_data_regions();

	printf("Enter the number of DataRegions to allocate: ");
	size_t regionCapacity;
	scanf("%u", &regionCapacity);

	size_t regionCount = 0;

	DataRegion* regions = (DataRegion*)malloc(regionCapacity * sizeof(DataRegion));
	if (regions != NULL)
	{
		printf("Add a DataRegion by writing \"+(FirstIndex,LastIndex)\", remove one by writing \"-(FirstIndex,LastIndex)\"\n");
		while (true) 
		{
			//Read a command
			char buffer[1024];
			scanf("%s", buffer);
			
			char* enter = strchr(buffer, '(');
			if (enter == NULL)
				goto badCommand;
			char* comma = strchr(enter, ',');
			if (comma == NULL)
				goto badCommand;
			char* exit = strchr(comma, ')');
			if (exit == NULL)
				goto badCommand;

			comma[0] = NULL;//Make 'enter' end at the comma
			exit[0] = NULL;//Make 'comma' end at the ')'

			int firstIndex = atoi(enter + 1);
			int lastIndex = atoi(comma + 1);

			DataRegion region = {firstIndex, lastIndex};

			switch (buffer[0])
			{
			case '+':
				//Add the region
				if (!add_data_region(regions, region, &regionCount, regionCapacity))
					printf("Add failed! [Capacity = %i Count = %i]\n", regionCapacity, regionCount);

				break;

			case '-':
				//Remove the region
				if (!remove_data_region(regions, region, &regionCount, regionCapacity))
					printf("Remove failed! [Capacity = %i Count = %i]\n", regionCapacity, regionCount);

				break;

			default:
				printf("Unrecognized command\n");
			}

			for (size_t i = 0; i < regionCount; i++)
				printf("(%i, %i) ", regions[i].first_index, regions[i].last_index);

			if (regionCount == 0)
				printf("[Empty DataRegion set]");

			printf("\n");

			continue;
		badCommand:
			printf("Invalid command syntax\n");
		}
		free(regions);
	}
	else
	{
		printf("Allocation failed!\n");
	}
}