#define LOCKTYP     1

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

#endif
