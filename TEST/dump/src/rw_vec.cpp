#include <bitset>
#include <ctime>
#include <cstdio>

unsigned char arr[0x200000];
std::bitset<0x200000> bits;

clock_t start, end;
double rwtime;

unsigned char null = 0;

double test_array()
{
	start = clock();
	// write
	for (size_t i = 0; i < 0x200000000; i++) {
		size_t idx = rand() % 0x200000;
		unsigned char val = rand() % 2;
		arr[idx] = val;
	}
	// read
	for (size_t i = 0; i < 0x200000000; i++) {
		size_t idx = rand() % 0x200000;
		null = arr[idx];
	}
	end = clock();
	rwtime = (double)(end - start) / CLOCKS_PER_SEC;
	return rwtime;
}

double test_bitset()
{
	start = clock();
	// write 
	for (size_t i = 0; i < 0x200000000; i++) {
		size_t idx = rand() % 0x200000;
		unsigned char val = rand() % 2;
		bits[idx] = val;
	}
	// read
	for (size_t i = 0; i < 0x200000000; i++) {
		size_t idx = rand() % 0x200000;
		null = bits[idx];
	}
	end = clock();
	rwtime = (double)(end - start) / CLOCKS_PER_SEC;
	return rwtime;
}



int main()
{
	srand(time(NULL));
	double array_time = 0, bitset_time = 0;
	for (int i = 0; i < 5; i++) {
		array_time += test_array();
		bitset_time += test_bitset();
	}
	printf("Test array access time ==> 5 loops average: %fs\n", array_time / 5);
	printf("Test bitset access time ==> 5 loops average: %fs\n", bitset_time / 5);
	return 0;
}