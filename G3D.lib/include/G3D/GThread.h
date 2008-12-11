/** 
  @file GThread.h
 
  @created 2005-09-22
  @edited  2007-01-31

 */

#ifndef G3D_GTHREAD_H
#define G3D_GTHREAD_H

#include "G3D/platform.h"
#include "G3D/ReferenceCount.h"
#include <string>

#ifndef G3D_WIN32
#   include <pthread.h>
#   include <signal.h>
#endif


namespace G3D {
        
typedef ReferenceCountedPointer<class GThread> GThreadRef;

/**
 Platform independent thread implementation.  You can either subclass and 
 override GThread::threadMain or call the create method with a method.

 Beware of reference counting and threads.  If circular references exist between
 GThread subclasses then neither class will ever be deallocated.  Also, 
 dropping all pointers (and causing deallocation) of a GThread does NOT 
 stop the underlying process.

 @sa G3D::GMutex, G3D::AtomicInt32
*/
class GThread : public ReferenceCountedObject {
private:
    enum ThreadState {STATE_CREATED, STATE_STARTED, STATE_RUNNING, STATE_COMPLETED};

    std::string             m_name;
    volatile ThreadState    m_state;

    void (*             m_threadProc)(void*);
    void*               m_threadParam;

#ifdef G3D_WIN32
    HANDLE              m_handle;
    HANDLE              m_event;
    static DWORD WINAPI entryPoint(LPVOID param);
#else
    pthread_t           m_handle;
    static void* entryPoint(void* param);
#endif //G3D_WIN32


    GThread(const std::string& name, void (*threadProc)(void*), void* param);

    // Not implemented on purpose, don't use
    GThread(const GThread &);
    GThread& operator=(const GThread&);
    bool operator==(const GThread&);

public:

    virtual ~GThread();

    /** Creates a GThread that will call the function threadProc with param when started.

        @param threadProc The global or static function called when the thread starts
        and then waits on return to exit. */
    static GThreadRef create(const std::string& name, void (*threadProc)(void*), void* param);

    /** Starts the thread and executes the assigned function.
        Returns false if the thread failed to start
        (either because it was already started or because the OS refused).*/
    bool start();

    /** Terminates the thread without notifying or
        waiting for a cancelation point. */
    void terminate();

    /**
        Returns true if the assigned function is still executing.
        This might not be true immediately after start() is called. */
    bool running() const;

    /** Returns true any time after start() has been called. */
    bool started() const;

    /** Returns true if the thread has exited. */
    bool completed() const;

    /** Waits for the thread to finish executing.
        Returns immediately if already completed. */
    void waitForCompletion();

    /** Returns thread name */
    inline const std::string& name() {
        return m_name;
    }
};


/**
 Mutual exclusion lock used for synchronization.
 @sa G3D::GThread, G3D::AtomicInt32
*/
class GMutex {
private:
#   ifdef G3D_WIN32
    CRITICAL_SECTION                    m_handle;
#   else
    pthread_mutex_t                     m_handle;
    pthread_mutexattr_t                 m_attr;
#   endif

    // Not implemented on purpose, don't use
    GMutex(const GMutex &mlock);
    GMutex &operator=(const GMutex &);
    bool operator==(const GMutex&);

public:
    GMutex();
    ~GMutex();

    /** Locks the mutex or blocks until available. */
    void lock();

    /** Locks the mutex if it not already locked.
        Returns true if lock successful, false otherwise. */
    bool tryLock();

    /** Unlocks the mutex. */
    void unlock();
};


/**
    Automatically locks while in scope.
*/
class GMutexLock {
private:
    GMutex* m;

    // Not implemented on purpose, don't use
    GMutexLock(const GMutexLock &mlock);
    GMutexLock &operator=(const GMutexLock &);
    bool operator==(const GMutexLock&);

public:
    GMutexLock(GMutex* mutex) {
        m = mutex;
        m->lock();
    }

    ~GMutexLock() {
        m->unlock();
    }
};


} // namespace G3D

#endif //G3D_GTHREAD_H
