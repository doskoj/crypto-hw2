#include <iostream>

int main(int argc, char * argv[])
{
	int a = atoi(argv[1]);
	int n = atoi(argv[2]);
	int m = 1;
	int k = 1;
	while (abs(k*n - m*a) != 1)
	{
		if (k*n > m*a) m++;
		else if (k*n < m*a) k++;
		else
		{
			std::cout << a << " is not invertable mod " << n << std::endl << "m = " << m << ", k = " << k << std::endl;
			return 0;
		}
	}
	std::cout << a << "^-1 = " << m << " mod " << n << std::endl;
}