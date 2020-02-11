#define IBM 0		//INTEL for intel skylake, IBM for power8
#define RELEASE 0	//DEBUG for printing dump
#define FREQTEST 0	//FREQTEST for testing transactions with different frequency, MEMTEST for testing transactions with different size of allocated memory

#include <cstdio>
#include <cstdlib>
#include <utility>
#include <thread>
#include <atomic>

#ifdef INTEL
	#include <immintrin.h>
	#include <x86intrin.h>
#endif
#ifdef IBM
	#include <htmxlintrin.h>
#endif
#include <chrono>

#define NUMOFTRANS 1000000


using namespace std;
using namespace std::chrono;
using namespace std::this_thread;

std::atomic<long long unsigned> tx (0);		//counter of successful transactions
std::atomic<long long unsigned> aborts (0);	//counter of aborts
std::atomic<long long unsigned> conflicts (0);
std::atomic<long long unsigned> retry (0);	//only for Intel
std::atomic<long long unsigned> illegal (0);	//only for IBM
std::atomic<long long unsigned> capacity (0);
std::atomic<long long unsigned> nesting (0);
std::atomic<long long unsigned> userAbort (0);
std::atomic<long long unsigned> persistent (0);	//only for IBM
std::atomic<long long unsigned> debug (0); 	//only for Intel

char* tmp;	//the global variable where we make writes inside of transaction

void test (const int volume, int threadNum, int param)
{
	int memory = 16;
	int residue = 10;
	int iter = 0;
#ifdef IBM
	TM_buff_type TM_buff;	
#endif

	for (int i = 0; i < volume; i++)
	{
#ifdef FREQTEST
		residue = param;
#endif
#ifdef MEMTEST
		memory = param;
#endif
		iter = rand() % memory;
		tmp = (char*) calloc (memory, sizeof(char));
		sleep_until(system_clock::now() + milliseconds(residue));
#ifdef INTEL
		unsigned status = _xbegin();
		if (status == _XBEGIN_STARTED)
#endif
#ifdef IBM
		if ( __TM_begin (TM_buff) )	
#endif
		{
			tmp[iter] = threadNum;
#ifdef INTEL
			_xend();
#endif
#ifdef IBM
			__TM_end();
#endif
			tx++;
		}
		else
		{
#ifdef IBM
			if (__TM_is_conflict (TM_buff) )
#endif
#ifdef INTEL
			if (status & _XABORT_CONFLICT)
#endif
			{
#ifdef DEBUG
				printf ("Conflict! \n");
#endif
				conflicts++;
			}
#ifdef IBM
			else if (__TM_is_illegal (TM_buff) )
			{
#ifdef DEBUG
				printf ("Illegal! \n");
#endif
				illegal++;
			}
#endif
#ifdef INTEL
			else if (status & _XABORT_RETRY)
			{
#ifdef DEBUG
				printf ("Illegal! \n");
#endif
				retry++;
			}
#endif
#ifdef IBM
			else if (__TM_is_footprint_exceeded (TM_buff) )
#endif
#ifdef INTEL
			else if (status & _XABORT_CAPACITY)
#endif
			{
#ifdef DEBUG
				printf ("Capacity! \n");
#endif
				capacity++;
			}
#ifdef IBM
			else if (__TM_is_nested_too_deep (TM_buff) )
#endif
#ifdef INTEL
			else if (status & _XABORT_NESTED)
#endif
			{
#ifdef DEBUG
				printf ("Nested too deep! \n");
#endif
				nesting++;
			}
#ifdef IBM
			else if (__TM_is_user_abort (TM_buff) )
#endif
#ifdef INTEL
			else if (status & _XABORT_EXPLICIT)
#endif
			{
#ifdef DEBUG
				printf ("User abort! \n");
#endif
				userAbort++;
			}
#ifdef IBM
			else if (__TM_is_failure_persistent (TM_buff) )
			{
#ifdef DEBUG
				printf ("Persistent failure! \n");
#endif
				persistent++;
			}
#endif
#ifdef INTEL
                        else if (status & _XABORT_DEBUG)
                        {
#ifdef DEBUG
                                printf ("Debug! \n");
#endif
                                debug++;
                        }
#endif
			else 
			{
#ifdef DEBUG
				printf ("Unknown reason :( \n");
#endif
			}
			aborts++;
		}
	}
	return;
}

int main (int argc, char** argv)
{
	int maxThreads = 10;
	int param = 10;
	if (argc > 1)
	{
		param = atoi(argv[1]);
	}

	std::thread thr[maxThreads];
	
	for (int i = 0; i < maxThreads; i++)
	{
		thr[i] = std::thread (test, NUMOFTRANS/maxThreads, i, param);		
		cpu_set_t cpuset;
		CPU_ZERO (&cpuset);
		pthread_setaffinity_np (thr[i].native_handle(), sizeof(cpu_set_t), &cpuset);
	}

	for (int i = 0; i < maxThreads; i++)
	{
		thr[i].join ();
	}

#ifdef FREQTEST
	printf  ("number of transactions and aborts with time delay %d milliseconds between transactions: %llu vs %llu\n\n\n", param, tx.load(), aborts.load());
	FILE* tx_out = fopen ("freq.csv", "a");
	fprintf (tx_out, "%d ; %llu ; %llu ; \n", param, tx.load(), aborts.load());
	fclose (tx_out);
#endif
#ifdef MEMTEST
	printf ("number of transactions and aborts with allocated memory of %d bytes before transaction: %llu vs %llu\n\n\n", param, tx.load(), aborts.load());
#endif

#ifdef IBM	
	printf ("Conflicts: %lld \nIllegal instructions: %lld \nFootprint exceeded: %lld \nNesting depth exceeded: %lld \nUser aborts: %lld \nPersistent failure: %lld \n******************************************************************************************************\n\n\n", conflicts.load(), illegal.load(), capacity.load(), nesting.load(), userAbort.load(), persistent.load());
#endif
#ifdef INTEL
	printf ("Conflicts: %lld \nRetry is possible: %lld \nCapacity exceeded: %lld \nNesting depth exceeded: %lld \nDebug: %lld \nUser aborts: %lld \n******************************************************************************************************\n\n\n", conflicts.load(), retry.load(), capacity.load(), nesting.load(), debug.load(), userAbort.load());
#endif
}
