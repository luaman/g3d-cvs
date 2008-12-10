/**
 @file GThread.cpp

 GThread class.

 @created 2005-09-24
 @edited  2005-10-22
 */

#include "G3D/GThread.h"
#include "G3D/System.h"
#include "G3D/debugAssert.h"


namespace G3D {

//GThread implementation
GThread::GThread(const std::string& name, void (*threadProc)(void*), void* param) :
    m_name(name),
    m_state(STATE_CREATED),
    m_threadProc(threadProc),
    m_threadParam(param) {

#ifdef G3D_WIN32
    m_event = NULL;
#endif

    // system-independent clear of handle
    System::memset(&m_handle, 0, sizeof(m_handle));
}

GThread::~GThread() {
#ifdef _MSC_VER
#   pragma warning( push )
#   pragma warning( disable : 4127 )
#endif
    alwaysAssertM(m_state != STATE_RUNNING, "Deleting thread while running.");
#ifdef _MSC_VER
#   pragma warning( pop )
#endif

#ifdef G3D_WIN32
    if (m_event) {
        ::CloseHandle(m_event);
    }
#endif
}

GThreadRef GThread::create(const std::string& name, void (*threadProc)(void*), void* param) {
    return new GThread(name, threadProc, param);
}


bool GThread::started() const {
    return m_state != STATE_CREATED;
}

bool GThread::start() {
    
    debugAssertM(! started(), "Thread has already executed.");
    if (started()) {
        return false;
    }

    m_state = STATE_STARTED;

#ifdef G3D_WIN32
    // create win32 thread and pass in this as parameter
    DWORD threadId;

    m_event = ::CreateEvent(NULL, TRUE, FALSE, NULL);
    debugAssert(m_event);

    m_handle = ::CreateThread(NULL, 0, &entryPoint, this, 0, &threadId);

    if (m_handle == NULL) {
        ::CloseHandle(m_event);
        m_event = NULL;
    }

    return (m_handle != NULL);
#else
    // create pthread thread and pass in this as parameter
    if (!pthread_create(&m_handle, NULL, &entryPoint, this)) {
        return true;
    } else {
        // system-independent clear of handle
        System::memset(&m_handle, 0, sizeof(m_handle));

        return false;
    }
#endif
}

void GThread::terminate() {
    if (m_handle) {

        // stop the thread
#ifdef G3D_WIN32
        ::TerminateThread(m_handle, 0);
#else
        pthread_kill(m_handle, SIGSTOP);
#endif

        // system-independent clear of handle
        System::memset(&m_handle, 0, sizeof(m_handle));
    }
}

bool GThread::running() const{
    return (m_state == STATE_RUNNING);
}

bool GThread::completed() const {
    return (m_state == STATE_COMPLETED);
}

void GThread::waitForCompletion() {
    if (m_state != STATE_COMPLETED)
    {
        // wait on thread completion
#ifdef G3D_WIN32
        debugAssert(m_event);
        ::WaitForSingleObject(m_event, INFINITE);
#else
        debugAssert(m_handle);
        pthread_join(m_handle, NULL);
#endif
    }
}

// wrapper for all GThread functions
#ifdef G3D_WIN32
DWORD WINAPI GThread::entryPoint(LPVOID param) {
    GThread* thread = reinterpret_cast<GThread*>(param);
    debugAssert(thread->m_event);

    thread->m_state = STATE_RUNNING;

    thread->m_threadProc(thread->m_threadParam);

    thread->m_state = STATE_COMPLETED;

    ::SetEvent(thread->m_event);
    return 0;
}
#else
void* GThread::entryPoint(void* param) {
    GThread* v = reinterpret_cast<GThread*>(param);

    thread->m_state = STATE_RUNNING;

    thread->m_threadProc(thread->m_threadParam);

    thread->m_state = STATE_COMPLETED;

    return (void*)NULL;
}
#endif


//GMutex implementation
GMutex::GMutex() {
#ifdef G3D_WIN32
    ::InitializeCriticalSection(&m_handle);
#else
    pthread_mutex_init(&m_handle, NULL);
#endif
}

GMutex::~GMutex() {
    //TODO: Debug check for locked
#ifdef G3D_WIN32
    ::DeleteCriticalSection(&m_handle);
#else
    pthread_mutex_destroy(&m_handle);
#endif
}

bool GMutex::tryLock() {
#ifdef G3D_WIN32
    return (::TryEnterCriticalSection(&m_handle) != 0);
#else
    return (pthread_mutex_trylock(&m_handle) == 0);
#endif
}

void GMutex::lock() {
#ifdef G3D_WIN32
    ::EnterCriticalSection(&m_handle);
#else
    pthread_mutex_lock(&m_handle);
#endif
}

void GMutex::unlock() {
#ifdef G3D_WIN32
    ::LeaveCriticalSection(&m_handle);
#else
    pthread_mutex_unlock(&m_handle);
#endif
}

} // namespace G3D
