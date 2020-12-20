#include <unistd.h>
#ifdef __linux__
#include <pthread.h>
#include <signal.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <errno.h>
#endif

#include "base/thread/t_resalloc.h"
#include "base/thread/t_thread.h"

Thread::Thread():tid(0),threadStatus(THREAD_STATUS_NEW)
{

}

Thread::~Thread()
{
    if(tid > 0)
    {
        pthread_cancel(tid);
        pthread_join(tid,NULL);
    }
}

void *Thread::run0(void *pVoid)
{
    //允许销毁线程
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);

    Thread *p = (Thread *)pVoid;
    p->run1();
    return p;
}

void *Thread::run1()
{
    threadStatus = THREAD_STATUS_RUNNING;
    tid = pthread_self();

    run();

    threadStatus = THREAD_STATUS_EXIT;
    tid = 0;

    pthread_exit(NULL);
}

bool Thread::Start()
{
    return pthread_create(&tid,NULL,run0,this) == 0;
}

pthread_t Thread::getThreadId()
{
    return tid;
}

void Thread::threadSleep(unsigned long ms)
{
    usleep(ms * 1000);
}

int Thread::getStatus()
{
    return threadStatus;
}

int Thread::Cancle()
{
    return pthread_cancel(tid);
}

int Thread::Kill()
{
    return pthread_kill(tid,SIGKILL);
}

void Thread::Join()
{
    if(tid > 0)
    {
        pthread_join(tid,NULL);
    }
}

void Thread::Join(unsigned long ms)
{
    if(tid == 0)
        return ;

    if(ms == 0)
    {
        Join();
    }
    else
    {
        unsigned long k = 0;
        while(threadStatus != THREAD_STATUS_EXIT && k <= ms)
        {
            usleep(100);
            k++;
        }
    }
}

void Thread::Stop()
{
    pthread_exit(NULL);
}

int Thread::Yield()
{
    return pthread_yield();
}

int Thread::threadDetach(pthread_t tid)
{
    return pthread_detach(tid);
}

int Thread::threadJoin(pthread_t tid, void **retVal)
{
    return pthread_join(tid,retVal);
}

int Thread::threadCancel(pthread_t tid)
{
    return pthread_cancel(tid);
}
