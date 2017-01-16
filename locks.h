#define LOCKTYP     2

#if    LOCKTYP == 0

#define LOCKSTR      "None"
#define INIT()
#define END()
#define ACQUIRE() 
#define RELEASE()

#elif  LOCKTYP == 1

volatile INT64 *lock = (INT64*)ALIGNED_MALLOC(sizeof(INT64), lineSz);

#define LOCKSTR    "TATAS optimistic lock"
#define INIT()     *lock = 0;
#define END()
#define ACQUIRE()  while(InterlockedExchange(lock, 1)){ \
                         while(*lock == 1){_mm_pause();}} 
#define RELEASE()  *lock = 0;

#elif  LOCKTYP == 2

volatile INT64 *lock = (INT64*)ALIGNED_MALLOC(sizeof(INT64), lineSz);

#define LOCKSTR    "HLE TATAS optimistic lock"
#define INIT()     *lock = 0;
#define END()
#define ACQUIRE()  while(_InterlockedExchange_HLEAcquire(lock, 1)){ \
                         while(*lock){_mm_pause();}}
#define RELEASE()  _Store_HLERelease(lock, 0);

#elif LOCKTYP == 3

#define TRANSACTION 0
#define LOCK        1

#define LOCKSTR     "RTM with non-transactionnal path using TATAS lock"
#define INIT()

#define ACQUIRE()   int state = TRANSACTION;int attempt = 1;while (1) { \
                        UINT status =_XBEGIN_STARTED; \
                        if (state == TRANSACTION) { status = _xbegin();} \
                        else { \
                        	while (InterlockedExchange(lock, 1)) { \
                        		do {_mm_pause();}while (*lock); \
                        }} \
                        if (status == _XBEGIN_STARTED) { \
                        	if (state == TRANSACTION && lock){_xabort(0xA0);}
#define RELEASE() 			if (state == TRANSACTION) { _xend(); } \
                        	else{ *lock = 0; }break;
              			}else{ \
              				if(lock) { do { _mm_pause(); }while(lock); } \
              				else{ \
              					volatile UINT64 wait = attempt << 4; \
              					while (wait--); \
              				} \
              				if (++attempt >= MAXATTEMPT) { state = LOCK; } \
              			} \
              		} \

#endif
