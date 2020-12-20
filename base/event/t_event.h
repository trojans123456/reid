#ifndef T_EVENT_H
#define T_EVENT_H

#ifdef __linux__
#include <sys/time.h>
#include <sys/epoll.h>
#endif

/**
用C的形式 提高效率 后期处理的数据也不会太多
*/

#define EVENT_NONE      0
#define EVENT_READABLE  1
#define EVENT_WRITABLE  2
#define EVENT_ERRABLE   4

#define EVENT_FILE_EVENTS       1
#define EVENT_TIME_EVENTS       2
#define EVENT_ALL_EVENTS        (EVENT_FILE_EVENTS | EVENT_TIME_EVENTS)
#define EVENT_DONT_WAIT         4 /* poll立即返回*/

#define EVENT_NOMORE        -1 /* timer事件返回该宏立即删除该timer*/
#define EVENT_NOTUSED(V)    ((void)V)


#define EVENT_MAX       128

/** 处理 fd 事件 */
typedef void evFileProc(int fd,void *clientData,int mask);
/** 返回下次要延迟的时间 */
typedef int evTimeProc(long long id,void *clientData);

/** 当timer事件被删除时调用*/
typedef void evEventFinalizerProc(void *clientData);
/** 进入poll监听前处理的内容*/
typedef void evBeforeSleepProc(void *clientData);

typedef struct evFileEvent
{
    int mask; /* EVENT_READABLE or EVENT_WRITABLE */
    evFileProc *rfileProc;
    evFileProc *wfileProc;
    evFileProc *efileProc;
    void *clientData;
}evFileEvent;

typedef struct evTimeEvent
{
    long long id; /* timer 事件标识符*/
    long when_sec;
    long when_ms;
    evTimeProc *timeProc;
    evEventFinalizerProc *finalizerProc;
    void *clientData;
    struct evTimeEvent *next;
}evTimeEvent;

/* poll时发生了的事件 */
typedef struct evFiredEvent
{
    int fd;
    int mask;
}evFiredEvent;

/** EPOLL */
typedef struct evApiState
{
    int epfd;
    struct epoll_event events[EVENT_MAX];
}evApiState;

class EventLoop
{
public:
    EventLoop();
    ~EventLoop();

    void evStart();
    void evStop();

    void evSetBeforeSleepProc(evBeforeSleepProc *beforesleep,void *clientData);

    int evCreateFileEvent(int fd,int mask,evFileProc *proc,void *clientData);
    void evDeleteFileEvent(int fd,int delmask);

    long long evCreateTimeEvent(long long msec,evTimeProc *proc,void *clientData,evEventFinalizerProc *finalizerProc);
    int evDeleteTimeEvent(long long id);

private:
    void evGetTime(long *seconds,long *milliseconds);
    void evAddMillisecondsToNow(long long milliseconds, long *sec, long *ms);
    evTimeEvent *evSearchNearestTimer();
    int processTimeEvents();

    int evProcessEvents(int flags);

    /** 禁止拷贝构造*/
    EventLoop(EventLoop &) {}
    EventLoop &operator=(EventLoop &) {return *this;}
private:
    int maxfd;
    long long timeEventNextId;
    bool stop;
    evBeforeSleepProc *beforesleepProc;
    void *beforesleepArgs;
    evApiState *state;
    evTimeEvent *timeEventHead;
    struct evFileEvent events[EVENT_MAX];
    struct evFiredEvent fired[EVENT_MAX];
};











#endif
