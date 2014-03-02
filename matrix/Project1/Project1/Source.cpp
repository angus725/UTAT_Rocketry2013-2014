#include <iostream>
#include <windows.h> 
int main()
{
	for (int i[79], k = 0, n = 0;;)
	{

		(k++ > 78) ? k = 0, Sleep(15) : ((n *= 2 ^ 23) >= 0) ? n += 2 ^ 23 : n *= -1;
		(!(n % 183)) ? i[k] = n % 25 + 5 : 0;//183
		std::cout << ((i[k]-- > 0) ? char(n % 89 + 33) : ' ');
	}
}
//hayden comment