#include "stdafx.h"                             // pre-compiled headers
#include <iostream>                             // cout
#include <iomanip>                              // setprecision
#include "helper.h"                             //

using namespace std;                            // cout

#define COUNTER64
#define NOPS        100
#define NSECONDS    2                           // run each test for NSECONDS

#ifdef COUNTER64
#define VINT    UINT64                          //  64 bit counter
#else
#define VINT    UINT                            //  32 bit counter
#endif

#define ALIGNED_MALLOC(sz, align) _aligned_malloc(sz, align)
#define GINDX(n)    (g+n*lineSz/sizeof(VINT))   //

UINT64 tstart;                                  // start of test in ms
int lineSz = getCacheLineSz();                  // cache line size
int maxThread;                                  // max # of threads

#include "locks.h"
#include "bst.h"

THREADH *threadH;                               // thread handles
UINT64 *ops;                                    // for ops per thread
BST *bst;
const int KEY_RANGE = 4096;


typedef struct {
    int nt;                                     // # threads
    UINT64 rt;                                  // run time (ms)
    UINT64 ops;                                 // ops
} Result;

typedef struct {
    size_t thread;                              // thread ID
    size_t keyRange;
} Parameters;

Result *r;                                      // results
UINT indx;                                      // results index

volatile VINT *g;                               // NB: position of volatile

//
// worker
//
WORKER worker(void *vthread)
{
	Parameters* params = (Parameters*)vthread;
    int thread = (int)(params->thread);
    UINT64 keyRange = (UINT64)(params->keyRange);

    UINT64 n = 0;
    runThreadOnCPU(thread % ncpu);

    UINT64 randomNumber = getWallClockMS();
    randomNumber = rand(randomNumber);
    bool addOperation;
    UINT64 key;
	
    while (true) {
        //
        // do some work
        //
        for(int i=0; i<NOPS ; ++i){
        	randomNumber = rand(randomNumber);
            addOperation = randomNumber & 1;
            key = (randomNumber >> 1) & (keyRange-1); // % keyRange;
            // cout << randomNumber << endl;
            // cout << (randomNumber>>1) << endl;
            if(addOperation){
            	// cout << "Adding " << key << endl;
            	bst->add(new Node(key));
            }else{
            	// cout << "Removing " << key << endl;
            	bst->remove(key);
            }
        }
        n += NOPS;

        //
        // check if runtime exceeded
        //
        if ((getWallClockMS() - tstart) > NSECONDS*1000)
            break;
    }

    ops[thread] = n;
    return 0;
}



//
// main
//
int main()
{
    ncpu = getNumberOfCPUs();   // number of logical CPUs
    maxThread = 2 * ncpu;       // max number of threads

    cout << " NOPS=" << NOPS << " NSECONDS=" << NSECONDS ;
    cout << endl;

    cout << endl << "??????????????????????" << endl;
    if(rtmSupported()){
    	cout << "RTM is supported :)" << endl;
    }
    if(hleSupported()){
    	cout << "HLE is supported :)" << endl;
    }
    cout << "??????????????????????" << endl << endl;

    //
    // allocate global variable
    //
    threadH = (THREADH*) ALIGNED_MALLOC(maxThread*sizeof(THREADH), lineSz);       // thread handles
    
    ops = (UINT64*) ALIGNED_MALLOC(maxThread*sizeof(UINT64), lineSz);             // for ops per thread
    g = (VINT*) ALIGNED_MALLOC((maxThread + 1)*lineSz, lineSz);                         // local and shared global variables

    r = (Result*) ALIGNED_MALLOC(5*maxThread*sizeof(Result), lineSz);             // for results
    memset(r, 0, 5*maxThread*sizeof(Result));                                     // zero

    indx = 0;

    //
    // use thousands comma separator
    //
    setCommaLocale();

    cout << "\n*************************************\nStrategy: " << LOCKSTR << "\n*************************************" << endl << endl;

    //
    // header
    //
    cout << setw(4) << "nt";
    cout << setw(6) << "rt";
    cout << setw(13) << "ops";
    cout << setw(12) << "kr";
    cout << endl;

    cout << setw(4) << "--";        // nt
    cout << setw(6) << "--";        // rt
    cout << setw(13) << "---";      // ops
    cout << setw(12) << "---";      // kr
    cout << endl;

    INIT(); // init lock system

    //
    // run tests
    //
    // maxThread = 2; // for testing
    for (int nt = 1; nt <= maxThread; nt *= 2, indx++) {
    	cout << endl;
    	for(int keyRange=16 ; keyRange < 1048577 ; keyRange *= 16){
    		//
	        // get start time
	        //
	        tstart = getWallClockMS();
		    bst = new BST();
		    bst->prefill(keyRange);

	        //
	        // create worker threads
	        //
	        for (int thread = 0; thread < nt; thread++){
	        	Parameters p;
	        	p.thread = (size_t)thread;
	        	p.keyRange = keyRange;
	            createThread(&threadH[thread], worker, (void*)(&p));
	        }

	        //
	        // wait for ALL worker threads to finish
	        //
	        waitForThreadsToFinish(nt, threadH);
	        UINT64 rt = getWallClockMS() - tstart;

	        //
	        // save results and output summary to console
	        //
	        r[indx].ops = 0;
	        for(int i=0 ; i<nt ; ++i)
	            r[indx].ops += ops[i];

	    	r[indx].nt = nt;
	    	r[indx].rt = rt;
	    	
	        cout << setw(4) << nt;
	        cout << setw(6) << fixed << setprecision(2) << (double) rt / 1000;
	        cout << setw(13) << r[indx].ops;
	        cout << setw(12) << keyRange;

	        //
	        // delete thread handles
	        //
	        for (int thread = 0; thread < nt; thread++)
	            closeThread(threadH[thread]);

		    if(bst->treeIsConsistent()){
		    	cout << "   <---- Congrats, the tree is consistent!" << endl;
		    }else{
		    	cout << "   <---- The tree is not consistent! Keep digging." << endl;
		    }
    	}
    }

    cout << endl;
    END();

    quit();

    return 0;

}
// eof
