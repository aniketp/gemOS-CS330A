#include <stdio.h>
#include <stdbool.h>
#include <math.h>

extern bool isPrime(int num);

int find_square_root(int num) {
	double sqroot;
	sqroot = sqrt((double)num);

	return (int)(ceil(sqroot));
}

int main() {
	int arr[10] = {123, 54, 1223, 12, 543, 1346, 655, 23, 65, 233};
	int maxprime = 0;

	for (int i = 0; i < 10; i++)
		if (isPrime(arr[i]))
			if (maxprime < arr[i])
				maxprime = arr[i];

	printf("Maximum prime is: %d\n", maxprime);
	return 0;
}
