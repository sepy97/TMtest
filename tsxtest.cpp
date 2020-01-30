#define INTEL 0

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

std::atomic<long long unsigned> tx(0);//         = 0;
std::atomic<long long unsigned> aborts(0);//     = 0;
long long unsigned conflicts  = 0;
long long unsigned retry      = 0;	//only for Intel
long long unsigned illegal    = 0;	//only for IBM
long long unsigned capacity   = 0;
long long unsigned nesting    = 0;
long long unsigned userAbort  = 0;
long long unsigned persistent = 0;	//only for IBM

int tmp;
//char tmpArray [256];

void test (const int volume, int threadNum, int residue)
{
#ifdef IBM
	TM_buff_type TM_buff;	//@@@@
#endif

	for (int i = 0; i < volume; i++)
	{
		sleep_until(system_clock::now() + milliseconds(residue));
//		auto volatile v = tmp; 
//		auto volatile vt = volume;
#ifdef INTEL
		unsigned status = _xbegin();
		if (status == _XBEGIN_STARTED)
#endif
#ifdef IBM
		if ( __TM_begin (TM_buff) )	//@@@@
#endif
		{
			tmp = threadNum;
//			tmpArray[i%256] = (threadNum*1000)%256;
			//tmp = (threadNum * 2)/10;
			//tmp += threadNum/7;
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
//				printf ("Conflict! \n");
				conflicts++;
			}
#ifdef IBM
			else if (__TM_is_illegal (TM_buff) )
			{
//				printf ("Illegal! \n");
				illegal++;
			}
#endif
#ifdef INTEL
			else if (status == _XABORT_RETRY)
			{
//				printf ("Illegal! \n");
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
//				printf ("Cache Line! \n");
				capacity++;
			}
#ifdef IBM
			else if (__TM_is_nested_too_deep (TM_buff) )
#endif
#ifdef INTEL
			else if (status == _XABORT_NESTED)
#endif
			{
//				printf ("Nested too deep! \n");
				nesting++;
			}
#ifdef IBM
			else if (__TM_is_user_abort (TM_buff) )
#endif
#ifdef INTEL
			else if (status == _XABORT_EXPLICIT)
#endif
			{
//				printf ("User abort! \n");
				userAbort++;
			}
#ifdef IBM
			else if (__TM_is_failure_persistent (TM_buff) )
			{
//				printf ("Persistent failure! \n");
				persistent++;
			}
#endif
			else 
			{
//				printf ("Unknown reason :( \n");
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
	int residue = 10;
	if (argc > 1)
	{
		residue = atoi(argv[1]);
	}
	else
	{
		residue = 10;
	}

	std::thread thr[maxThreads];
	
	for (int i = 0; i < maxThreads; i++)
	{
		thr[i] = std::thread (test, NUMOFTRANS/maxThreads, i, residue);		
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
	printf ("Conflicts: %lld \nIllegal instructions: %lld \nFootprint exceeded: %lld \nNesting depth exceeded: %lld \nUser aborts: %lld \nPersistent failure: %lld \n", conflicts, illegal, footprint, nesting, userAbort, persistent);
#endif
#ifdef INTEL
	printf ("Conflicts: %lld \nRetry is possible: %lld \nCapacity exceeded: %lld \nNesting depth exceeded: %lld \nUser aborts: %lld \n", conflicts, retry, capacity, nesting, userAbort);
#endif
}
