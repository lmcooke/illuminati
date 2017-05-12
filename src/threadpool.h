#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <G3D/G3DAll.h>

class App;

/** A worker thread */
class ThreadPoolThread : public Thread
{
public:
    typedef shared_ptr<ThreadPoolThread> Ref;

    ThreadPoolThread(App *parent, int index, int numThreads);
    virtual ~ThreadPoolThread();

    /** Whether this thread is completing a pass */
    bool isRendering();

    /** Causes this thread to start a new pass */
    void trigger();

    /** Causes this thread's main loop to return */
    void quit();

protected:
    /** Entry point */
    void threadMain();

private:
    App *   m_parent;
    int     m_index;
    int     m_numThreads;
    bool    m_rendering;
    bool    m_quit;
};


/** Thread pool of threads that call App::threadCallback.
  *
  * More or less mimics GThread::runConcurrently2D with one caveat: the same
  * set of threads is reused across multiple passes. Previously we used
  * GThread::runConcurrently2D, but found that after ~200 passes pthread_create
  * would decide it's out of resources and stop creating new threads. 
  */
class ThreadPool
{
public:
    typedef shared_ptr<ThreadPool> Ref;

    ThreadPool(App *parent, int numThreads = Thread::numCores());
    ~ThreadPool();

    /** Starts a new pass */
    void run();

private:
    App *                           m_parent;
    Array<ThreadPoolThread::Ref>    m_threads;
};

#endif
