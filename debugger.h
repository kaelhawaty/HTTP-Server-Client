#ifndef NETWORK_LAB_DEBUGGER_H
#define NETWORK_LAB_DEBUGGER_H

#include <mutex>

inline std::mutex &sync_mutex() {
    static std::mutex m;
    return m;
}

#define THREAD_SYNC_START   { std::scoped_lock< std::mutex > lock( sync_mutex() );
#define THREAD_SYNC_STOP    }

#define DEBUG_ENABLE

#ifdef DEBUG_ENABLE
#define Debug(fmt, args...)    do { THREAD_SYNC_START \
                                    fprintf(stdout, "DEBUG: %s:%d:%s(): " fmt "\n", \
                                    __FILE__, __LINE__, __func__, ##args); \
                                    THREAD_SYNC_STOP \
                                    } while(0)
#else
#define Debug(fmt, args...)
#endif

#define Err(fmt, args...)     do { THREAD_SYNC_START \
                                 fprintf(stderr, "ERROR: %s:%d:%s(): " fmt "\n", \
                                __FILE__, __LINE__, __func__, ##args); \
                                THREAD_SYNC_STOP \
                                } while(0)

#endif //NETWORK_LAB_DEBUGGER_H
