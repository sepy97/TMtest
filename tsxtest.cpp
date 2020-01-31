#define INTEL 0		//INTEL for intel skylake, IBM for power8
#define RELEASE 0	//DEBUG for printing dump
#define MEMTEST 0	//FREQTEST for testing transactions with different frequency, MEMTEST for testing transactions with different size of allocated memory

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
long long unsigned conflicts  = 0;
long long unsigned retry      = 0;	//only for Intel
long long unsigned illegal    = 0;	//only for IBM
long long unsigned capacity   = 0;
long long unsigned nesting    = 0;
long long unsigned userAbort  = 0;
long long unsigned persistent = 0;	//only for IBM

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
			if (status == _XABORT_CONFLICT)
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
			else if (status == _XABORT_RETRY)
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
			else if (status == _XABORT_CAPACITY)
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
			else if (status == _XABORT_NESTED)
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
			else if (status == _XABORT_EXPLICIT)
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
			else 
			{
#ifdef DEBUG
				printf ("Unknown reason :( \n");
#endif
			}
			
//				printf ("Failure address: %ld; Failure code: %lld\n", __TM_failure_address (TM_buff), __TM_failure_code (TM_buff));
			
//			printf ("trans aborts %llu \n", aborts);
			aborts++;
		}
	}
	//printf ("Thread #%d is on CPU %d\n", threadNum, sched_getcpu());
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

	printf ("number of transactions and aborts with time delay %d milliseconds between transactions: %llu vs %llu\n\n\n", residue, tx.load(), aborts.load());

#ifdef IBM	
	printf ("Conflicts: %lld \nIllegal instructions: %lld \nFootprint exceeded: %lld \nNesting depth exceeded: %lld \nUser aborts: %lld \nPersistent failure: %lld \n******************************************************************************************************\n\n\n", conflicts, illegal, capacity, nesting, userAbort, persistent);
#endif
#ifdef INTEL
	printf ("Conflicts: %lld \nRetry is possible: %lld \nCapacity exceeded: %lld \nNesting depth exceeded: %lld \nUser aborts: %lld \n******************************************************************************************************\n\n\n", conflicts, retry, capacity, nesting, userAbort);
#endif
}
