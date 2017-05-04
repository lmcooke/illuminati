
#include "app.h"
#include "threadpool.h"

static void wait()
{
    // Yield for 10 milliseconds
    usleep(10000);
}

ThreadPoolThread::ThreadPoolThread(App *parent, int index, int numThreads)
    : Thread("ThreadPoolThread"),
      m_parent(parent),
      m_index(index),
      m_numThreads(numThreads),
      m_rendering(false),
      m_quit(false)
{ }

ThreadPoolThread::~ThreadPoolThread() { }

bool ThreadPoolThread::isRendering()
{
    return m_rendering;
}

void ThreadPoolThread::trigger()
{
    m_rendering = true;
}

void ThreadPoolThread::quit()
{
    m_quit = true;
}

void ThreadPoolThread::threadMain()
{
    while (!m_quit)
    {
        if (m_rendering)
        {
            int w = m_parent->window()->width(),
                h = m_parent->window()->height();

            for (int y = m_index; y < h; y += m_numThreads)
                for (int x = 0; x < w; ++x)
                    m_parent->traceCallback(x, y);

            m_rendering = false;
        }
        else
        {
            wait();
        }
    }
}


ThreadPool::ThreadPool(App *parent, int numThreads)
    : m_parent(parent)
{
    // ???? This used to do something, I presume
    //if (numThreads == Thread::numCores())
    //    numThreads = Thread::numCores();

    for (int i = 0; i < numThreads; ++i)
    {
        ThreadPoolThread::Ref thr(new ThreadPoolThread(parent, i, numThreads));
        m_threads.append(thr);
        thr->start();
    }
}

ThreadPool::~ThreadPool()
{
    for (int i = 0; i < m_threads.size(); ++i)
        m_threads[i]->quit();

    for (int i = 0; i < m_threads.size(); ++i)
        m_threads[i]->waitForCompletion();
}

void ThreadPool::run()
{
    for (int i = 0; i < m_threads.size(); ++i)
        m_threads[i]->trigger();

    for (int i = 0; i < m_threads.size(); ++i)
        while (m_threads[i]->isRendering())
            wait();
}

