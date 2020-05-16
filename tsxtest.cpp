#define INTEL 0		//INTEL for intel skylake, IBM for power8
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
#define MAXTHREADS 4

#define MAXRAND50 250   //loop takes 50ms
#define MAXRAND100 355  //loop takes 100ms
#define MAXRAND20 155   //loop takes 20ms
#define MAXRAND5 75     //loop takes 5ms        
#define MAXRAND10 110   //loop takes 10ms

#define RANDSIZE 10000000

int thrval[NUMOFTRANS];
int randArr[NUMOFTRANS];

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

std::thread thr[MAXTHREADS];
//pthread_t thr[MAXTHREADS];
cpu_set_t cpuset;
//CPU_ZERO (&cpuset);
unsigned long long Randtmp1[MAXTHREADS];

void test (const int volume, int threadNum, int param)
{
	//int schedThread = sched_getcpu();

	//printf ("@BEGIN: Thread #%d is on CPU %d\n", threadNum, sched_getcpu());
	CPU_ZERO (&cpuset);
	CPU_SET (threadNum*2, &cpuset);
        pthread_setaffinity_np (thr[threadNum].native_handle(), sizeof(cpu_set_t), &cpuset);
	//int schedThread = sched_getcpu();

	int memory = 16;
	int residue = 10;
	int iter = 0;
#ifdef IBM
	TM_buff_type TM_buff;	
#endif

#ifdef FREQTEST
                residue = param;
#endif
#ifdef MEMTEST
                memory = param;
#endif

	unsigned long long randtmp = 0;
	int bound = 0;
                switch (residue)
                {
                case 5:
	                bound = MAXRAND5;
                        break;
                case 10:
                        bound = MAXRAND10;
                        break;
                case 20:
                        bound = MAXRAND20;
                        break;
                case 50:
                        bound = MAXRAND50;
                        break;
                case 100:
                        bound = MAXRAND100;
                        break;
                default:
                        bound = 10000;

                }
	printf ("%d %d \n", residue, bound);

	for (int i = 0; i < volume; i++)
	{
//		iter = rand() % memory;
		//sleep_until(system_clock::now() + milliseconds(residue));	@@@@

//		unsigned long time1 = clock ();	
		
		for (int j = 0; j < bound; j++)
                {
                        for (int k = 0; k < bound; k++)
                        {
                                randtmp += randArr [(i*(threadNum+1)+(k+bound*j))%RANDSIZE];
                        }
                        randtmp*=(i+1);
                }
//		unsigned long time2 = clock () - time1;	
//	        printf ("%lu \n", time2);

		Randtmp1[threadNum] += randtmp;
		iter = randArr [i] % memory;

//		if (threadNum*2 != sched_getcpu()) printf ("Threadnum %d is not equal to sched %d\n", threadNum*2, sched_getcpu());

//		printf ("@@AWAKE: Thread #%d is on CPU %d\n", threadNum, sched_getcpu());
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
	//int maxThreads = 10; //10;
	int param = 10;
	if (argc > 1)
	{
		param = atoi(argv[1]);
	}

//	std::thread thr[maxThreads];

#ifdef FREQTEST
	tmp = (char*) calloc (16, sizeof (char));
#endif
#ifdef MEMTEST
	tmp = (char*) calloc (param, sizeof (char));
#endif
	
	for (int i = 0; i < NUMOFTRANS; i++)
	{
		randArr[i] = rand() % NUMOFTRANS;
	}

	CPU_ZERO (&cpuset);
	for (int i = 0; i < MAXTHREADS; i++)
	{
		thr[i] = std::thread (test, NUMOFTRANS/MAXTHREADS, i, param);	
		
		//thr[i] = 
		//pthread_create(&(thr[i]), NULL, test, NUMOFTRANS/MAXTHREADS, i, param);		
		/*cpu_set_t cpuset;
		CPU_ZERO (&cpuset);
		pthread_setaffinity_np (thr[i].native_handle(), sizeof(cpu_set_t), &cpuset);
		thr[i] = std::thread (test, NUMOFTRANS/maxThreads, i, param);		*/
	}

	void* ret = NULL;
	for (int i = 0; i < MAXTHREADS; i++)
	{
		//pthread_join (thr[i], &ret);
		thr[i].join();
	}

	for (int i = 0; i < MAXTHREADS; i++)
	{
		printf ("%llu \n", Randtmp1[i]);
	}

#ifdef FREQTEST
	printf ("number of transactions and aborts with time delay %d milliseconds between transactions: %llu vs %llu\n\n\n", param, tx.load(), aborts.load());
	FILE* tx_out = fopen ("freq.csv", "a");
#endif
#ifdef MEMTEST
	printf ("number of transactions and aborts with allocated memory of %d bytes before transaction: %llu vs %llu\n\n\n", param, tx.load(), aborts.load());
	FILE* tx_out = fopen ("mem.csv", "a");
#endif
        fprintf (tx_out, "%d ; %llu ; %llu ; \n", param, tx.load(), aborts.load());

#ifdef IBM	
	printf ("Conflicts: %lld \nIllegal instructions: %lld \nFootprint exceeded: %lld \nNesting depth exceeded: %lld \nUser aborts: %lld \nPersistent failure: %lld \n******************************************************************************************************\n\n\n", conflicts.load(), illegal.load(), capacity.load(), nesting.load(), userAbort.load(), persistent.load());
        fprintf (tx_out, " ; %lld ; %lld ; %lld ; %lld ; %lld ; %lld ; \n", conflicts.load(), illegal.load(), capacity.load(), nesting.load(), userAbort.load(), persistent.load());
#endif
#ifdef INTEL
	printf ("Conflicts: %lld \nRetry is possible: %lld \nCapacity exceeded: %lld \nNesting depth exceeded: %lld \nDebug: %lld \nUser aborts: %lld \n******************************************************************************************************\n\n\n",                  conflicts.load(), retry.load(), capacity.load(), nesting.load(), debug.load(), userAbort.load());
        fprintf (tx_out, " ; %lld ; %lld ; %lld ; %lld ; %lld ; %lld ; \n", conflicts.load(), retry.load(), capacity.load(), nesting.load(), debug.load(), userAbort.load());
#endif
        fclose (tx_out);
}
