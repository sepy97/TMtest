#define IBM 0         //INTEL for intel skylake, IBM for power8
#define RELEASE 0       //DEBUG for printing dump

#include <cstdio>
#include <cstdlib>
#include <utility>
#include <thread>
#include <mutex>
#include <atomic>
#include <ctime>

#ifdef INTEL
	#include <immintrin.h>
	#include <x86intrin.h>
#endif
#ifdef IBM
	#include <htmxlintrin.h>
#endif

std::atomic<long long unsigned> tx (0);         //counter of successful transactions
std::atomic<long long unsigned> aborts (0);     //counter of aborts
std::atomic<long long unsigned> conflicts (0);
std::atomic<long long unsigned> retry (0);      //only for Intel
std::atomic<long long unsigned> illegal (0);    //only for IBM
std::atomic<long long unsigned> capacity (0);
std::atomic<long long unsigned> nesting (0);
std::atomic<long long unsigned> userAbort (0);
std::atomic<long long unsigned> persistent (0); //only for IBM
std::atomic<long long unsigned> debug (0);	//only for INTEL

#ifdef IBM
        TM_buff_type TM_buff;			//only for IBM - buffer contains information about currently active transaction
#endif

#define INIT_PUSH 1000
#define MAXTHREADNUM 100
#define MAX_VOLUME 1000000

using namespace std;

class FastRandom {
private:
	unsigned long long rnd;
public:
	FastRandom(unsigned long long seed) { //time + threadnum
		rnd = seed;
	}
	unsigned long long rand() {
		rnd ^= rnd << 21;
		rnd ^= rnd >> 35;
		rnd ^= rnd << 4;
		return rnd;
	}
};

struct node
{
	node *left, *right;
	int key, priority;
	node () : key (0), priority (0), left (nullptr), right (nullptr) { }
	node (int key, int priority) : key (key), priority (priority), left (nullptr), right (nullptr) { }
};
typedef node* treap;

void dumpTreap (treap out, int spacingCounter = 0)
{
	if (out)
	{
		dumpTreap (out->right, spacingCounter + 1);
		for (int i = 0; i < spacingCounter; i++) printf ("_________");
		printf ("(%d.%d)\n", out->key, out->priority);
		dumpTreap (out->left, spacingCounter + 1);
	}
}

void split (treap root, treap& left, treap& right, int key, treap* dupl)
{
	if (root == nullptr)
	{
		left  = nullptr;
		right = nullptr;
		return;
	}
	
	if (root->key < key)
	{
		(*dupl) = nullptr;
		
		split (root->right, root, root->right, key, dupl);
	}
	else if (root->key > key)
	{
		(*dupl) = nullptr;
		
		split (root->left, root->left, root, key, dupl);
	}
	else
	{
		auto volatile v = *root;
		auto volatile dv = *dupl;
		auto volatile vl = left;
		auto volatile vr = right;
		auto volatile vll = root->left;
		auto volatile vlr = root->right;
#ifdef INTEL
                unsigned status = _xbegin ();
		if (status & _XBEGIN_STARTED)
#endif
#ifdef IBM
                if ( __TM_begin (TM_buff) )
#endif
                {

			(*dupl) = root;
			left    = root->left;
			right   = root->right;
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
}

void merge (treap left, treap right, treap& result)
{
	if (left == nullptr || right == nullptr)
	{
		if (right == nullptr) result = left;
		else result = right;
		return;
	}
	
	if (left->key > right->key)
	{
		auto volatile v = *result;
		auto volatile vl = *left;
		auto volatile vr = *right;
#ifdef INTEL
		unsigned status = _xbegin ();
		if (status & _XBEGIN_STARTED)
#endif
#ifdef IBM
		if ( __TM_begin (TM_buff) )
#endif
		{
			std::swap (left, right);
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
		return;
		
	}
	
	if (left->priority > right->priority)
	{
		merge (left->right, right, left->right);
		result = left;
		return;
		
	}
	else
	{
		merge (left, right->left, right->left);
		result = right;
		return;
		
	}
}

void erase (treap& t, int key)
{
	if (t != nullptr)
	{
		if (t->key == key)
		{
			merge (t->left, t->right, t);
		}
		else
		{
			if (key < t->key)
			{
				erase (t->left, key);
			}
			else
			{
				erase (t->right, key);
			}
		}
	}
}

void insert (treap& t, treap toInsert)
{
	if (t == nullptr) t = toInsert;
	else if (toInsert->priority > t->priority)
	{
		treap dupl;
		split (t, toInsert->left, toInsert->right, toInsert->key, &dupl);
		t = toInsert;
	}
	else
	{
		if (toInsert->key < t->key)
		{
			insert (t->left, toInsert);
		}
		else
		{
			insert (t->right, toInsert);
		}
	}
}

treap toTest;

void abortTest (const int volume, int threadNum)
{
	//srand (time (NULL));
	for (int i = 0; i < volume; i++)
        {
                int insOrDel = rand ()%2;
                if (insOrDel)
                {
                        auto toAdd = new node (rand ()%volume, rand ()%volume);
                        insert (toTest, toAdd);
                }
                else
                {
                        int data = rand ()%volume;
                        erase (toTest, data);
                }
        }

}

int main (int argc, char** argv)
{
	int maxThreads = 0;
	if (argc > 1)
	{
		maxThreads = atoi(argv[1]);
	}
	else
	{
		maxThreads = 1;
	}
	
	toTest = new node ();
	srand (time (NULL));

	//Fill the treap with some random information for having something to delete out of there
	for (int i = 0; i < INIT_PUSH; i++)
	{
		auto toAdd = new node (rand()%INIT_PUSH, rand ()%INIT_PUSH);
		insert (toTest, toAdd);
		
	}

	dumpTreap (toTest, 1);	

	std::thread thr[maxThreads];
	
	for (int i = 0; i < maxThreads; i++)
	{
		thr[i] = std::thread (abortTest, MAX_VOLUME/maxThreads, i);
	}
	
	for (int i = 0; i < maxThreads; i++)
	{
		thr[i].join ();
	}
	
        printf ("number of transactions and aborts with %d threads: %llu vs %llu\n\n\n", maxThreads, tx.load(), aborts.load());

#ifdef IBM
        printf ("Conflicts: %lld \nIllegal instructions: %lld \nFootprint exceeded: %lld \nNesting depth exceeded: %lld \nUser aborts: %lld \nPersistent failure: %lld \n******************************************************************************************************\n\n\n", conflicts.load(), illegal.load(), capacity.load(), nesting.load(), userAbort.load(), persistent.load());
#endif
#ifdef INTEL
        printf ("Conflicts: %lld \nRetry is possible: %lld \nCapacity exceeded: %lld \nNesting depth exceeded: %lld \nDebug: %lld \nUser aborts: %lld \n******************************************************************************************************\n\n\n", conflicts.load(), retry.load(), capacity.load(), nesting.load(), debug.load(), userAbort.load());
#endif


	dumpTreap (toTest, 1);
}
