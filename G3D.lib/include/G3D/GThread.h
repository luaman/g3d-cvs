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

public:

    GThread(const std::string& name);

    virtual ~GThread();

    /** Constructs a basic GThread without requiring a subclass.

        @param proc The global or static function for the threadMain() */
    static GThreadRef create(const std::string& name, void (*proc)(void*), void* param);

    /** @deprecated use overload that accepts void* param */
    static GThreadRef create(const std::string& name, void (*proc)());

    /** Starts the thread and executes threadMain().  Returns false if
       the thread failed to start (either because it was already started
       or because the OS refused).*/
    bool start();

    /** Terminates the thread without notifying or
        waiting for a cancelation point. */
    void terminate();

    /**
        Returns true if threadMain is currently executing. */
    bool running();

    /** Returns completed status of thread. */
    bool completed();

    /** 
        Waits for the thread to finish executing. 
     */
    void waitForCompletion();

    /** Returns thread name */
    inline const std::string& name() {
        return m_name;
    }

    /** Overriden by the thread implementor */
    virtual void threadMain() = 0;

private:
    enum Status {STATUS_CREATED, STATUS_RUNNING, STATUS_COMPLETED};

    // Not implemented on purpose, don't use
    GThread(const GThread &);
    GThread& operator=(const GThread&);
    bool operator==(const GThread&);

#ifdef G3D_WIN32
    static DWORD WINAPI internalThreadProc(LPVOID param);
#else
    static void* internalThreadProc(void* param);
#endif //G3D_WIN32

    Status              m_status;

    // Thread handle to hold HANDLE and pthread_t
#ifdef G3D_WIN32
    HANDLE              m_handle;
    HANDLE              m_event;
#else
    pthread_t           m_handle;
#endif //G3D_WIN32

    std::string         m_name;
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
