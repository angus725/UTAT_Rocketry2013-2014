#include <iostream>
#include <cstdlib>
#include <windows.h>
#define C std::cout<<
int main()
{
	for (int i[79], k=0, n=0, m=79;; n = rand())
	{
		if (k++ < m)
		{
			if (!(n % 500))
				i[k] = 25;
		}
		else
		{
			k = 0;
			Sleep(15);
		};

		if (i[k]-->0)
			C char(n % m + 30);
		else
			C " ";
	}
}