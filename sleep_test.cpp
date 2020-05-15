#include <cstdio>
#include <ctime>
#include <cstdlib>

#define MAXRAND50 141	//loop takes 50ms
#define MAXRAND100 199	//loop takes 100ms
#define MAXRAND20 88	//loop takes 20ms
#define MAXRAND5 40	//loop takes 5ms	
#define MAXRAND10 60	//loop takes 10ms

#define RANDSIZE 1000000

using namespace std;

int randArr [RANDSIZE];

int main ()
{
	int randtmp = 0;
	srand (time (NULL));
	for (int i = 0; i < RANDSIZE; i++)
	{
		randArr [i] = rand() % 1000;
	}

	unsigned long time1 = clock ();
		
		for (int j = 0; j < MAXRAND; j++)
                {
                        for (int i = 0; i < MAXRAND; i++)
                        {
                                randtmp += randArr [i*j];
                        }
                        randtmp++;
                }

	unsigned long time2 = clock () - time1;
//	randtmp += 5;
	printf ("%d %lu \n", randtmp, time2);
	
}
