#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <thread>
#include <utility>

#include <x86intrin.h>

#define MAXRAND50 250	//loop takes 50ms
#define MAXRAND100 355  //loop takes 100ms
#define MAXRAND20 155   //loop takes 20ms
#define MAXRAND5 75	//loop takes 5ms	
#define MAXRAND10 110	//loop takes 10ms

#define RANDSIZE 1000000

using namespace std;
using namespace std::chrono;
using namespace std::this_thread;

int randArr [RANDSIZE];

int main ()
{
	int randtmp = 0;
	srand (time (NULL));
	for (int i = 0; i < RANDSIZE; i++)
	{
		randArr [i] = rand() % 1000;
	}

	uint64_t tick = __rdtsc();
	sleep_until(system_clock::now() + milliseconds(100));		
/*		for (int j = 0; j < MAXRAND50; j++)
                {
                        for (int i = 0; i < MAXRAND50; i++)
                        {
                                randtmp += randArr [i + MAXRAND50*j];
                        }
                        randtmp++;
                }
*/
	uint64_t tick2 = __rdtsc();
//	randtmp += 5;
	printf ("%d %lu \n", randtmp, (tick2 - tick)/2600000);
	
}
