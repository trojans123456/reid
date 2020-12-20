#ifndef T_THREAD_H
#define T_THREAD_H

#ifdef __linux__
#include <pthread.h>
#endif

class Thread
{
public:
    Thread();
    virtual ~Thread();
    enum ThreadStatus
    {
        THREAD_STATUS_NEW = 0,//新建
        THREAD_STATUS_RUNNING = 1,//运行
        THREAD_STATUS_EXIT = -1 //结束
    };

    static int threadDetach(pthread_t tid);
    static int threadJoin(pthread_t tid,void **retVal);
    static int threadCancel(pthread_t tid);

    virtual void run() = 0;//运行实体

    bool Start();//启动
    int  Cancle();//取消
    void Stop();
    int Yield();

    int Kill();
    void Join();
    void Join(unsigned long ms);//等待线程退出或超时
    pthread_t getThreadId();//获取线程id
    int getStatus();
    static void threadSleep(unsigned long ms);

private:
    pthread_t tid;
    int threadStatus;
    //线程回调指针
    static void *run0(void *pVoid);
    void *run1();//内部执行方法
};



#endif
