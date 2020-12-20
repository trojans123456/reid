#include <stdio.h>
#ifdef __linux__
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
//#include <sys/epoll.h>
#endif
#include "base/exception/t_exception.h"
#include "base/event/t_event.h"



EventLoop::EventLoop()
    :maxfd(-1)
    ,timeEventNextId(0)
    ,stop(false)
    ,beforesleepProc(NULL)
    ,beforesleepArgs(NULL)
    ,state(NULL)
    ,timeEventHead(NULL)
{
    int i;

    if(!state)
    {
        state = (evApiState *)malloc(sizeof(evApiState));
        if(!state)
        {
            throw TError("evApiState malloc failed");
        }

        state->epfd = epoll_create(EVENT_MAX);
        if(state->epfd < 0)
            throw TError("evApiState create failed");
    }

    for(i = 0;i <EVENT_MAX;i++)
    {
        events[i].mask = EVENT_NONE;
    }
}

EventLoop::~EventLoop()
{
    if(state)
    {
        close(state->epfd);
        free(state);
    }
}



void EventLoop::evStop()
{
    stop = true;
}

int EventLoop::evCreateFileEvent(int fd,int mask,evFileProc *proc,void *clientData)
{
    if(fd >= EVENT_MAX)
        return -1;
    evFileEvent *fe = &events[fd];

    /** add event */
    struct epoll_event ee;
    int op = events[fd].mask == EVENT_NONE ? EPOLL_CTL_ADD : EPOLL_CTL_MOD;

    ee.events = 0;
    mask |= events[fd].mask;
    if(mask & EVENT_READABLE)
        ee.events |= EPOLLIN;
    if(mask & EVENT_WRITABLE)
        ee.events |= EPOLLOUT;
    if(mask & EVENT_ERRABLE)
        ee.events |= EPOLLERR | EPOLLHUP;

    ee.data.u64 = 0;
    ee.data.fd = fd;
    if(epoll_ctl(state->epfd,op,fd,&ee) == -1)
        return -1;


    fe->mask |= mask;
    if(mask & EVENT_READABLE)
        fe->rfileProc = proc;
    if(mask & EVENT_WRITABLE)
        fe->wfileProc = proc;
    if(mask & EVENT_ERRABLE)
        fe->efileProc = proc;

    fe->clientData = clientData;

    if(fd > maxfd)
        maxfd = fd;

    return 0;
}

void EventLoop::evDeleteFileEvent(int fd, int delmask)
{
    if(fd >= EVENT_MAX)
        return ;
    evFileEvent *fe = &events[fd];

    if(fe->mask == EVENT_NONE)
        return ;
    fe->mask = fe->mask & (~delmask);

    if(fd == maxfd && fe->mask == EVENT_NONE)
    {
        /* update max fd*/
        int j;
        for(j = maxfd - 1;j >= 0;j--)
        {
            if(events[j].mask != EVENT_NONE)
                break;
            maxfd = j;
        }
    }

    /** del event */
    struct epoll_event ee;
    int mask = events[fd].mask & (~delmask);
    ee.events = 0;
    if(mask & EVENT_READABLE)
        ee.events |= EPOLLIN;
    if(mask & EVENT_WRITABLE)
        ee.events |= EPOLLOUT;
    if(mask & EVENT_ERRABLE)
        ee.events |= EPOLLERR | EPOLLHUP;

    ee.data.u64 = 0;
    ee.data.fd = fd;
    if(mask != EVENT_NONE)
    {
        epoll_ctl(state->epfd,EPOLL_CTL_MOD,fd,&ee);
    }
    else
    {
        epoll_ctl(state->epfd,EPOLL_CTL_DEL,fd,&ee);
    }
}

void EventLoop::evGetTime(long *seconds, long *milliseconds)
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    *seconds = tv.tv_sec;
    *milliseconds = tv.tv_usec / 1000;
}

void EventLoop::evAddMillisecondsToNow(long long milliseconds, long *sec, long *ms)
{
    long cur_sec, cur_ms, when_sec, when_ms;

    evGetTime(&cur_sec, &cur_ms);
    when_sec = cur_sec + milliseconds/1000;
    when_ms = cur_ms + milliseconds%1000;
    if (when_ms >= 1000) {
        when_sec ++;
        when_ms -= 1000;
    }
    *sec = when_sec;
    *ms = when_ms;
}

long long EventLoop::evCreateTimeEvent(long long msec,evTimeProc *proc,void *clientData,evEventFinalizerProc *finalizerProc)
{
    long long id = timeEventNextId++;
    evTimeEvent *te;

    te = (evTimeEvent *)malloc(sizeof(*te));
    if(te == NULL)
        return -1;

    te->id = id;
    evAddMillisecondsToNow(msec,&te->when_sec,&te->when_ms);

    te->timeProc = proc;
    te->finalizerProc = finalizerProc;
    te->clientData = clientData;
    te->next = timeEventHead;
    timeEventHead = te;

    return id;
}

int EventLoop::evDeleteTimeEvent(long long id)
{
    evTimeEvent *te,*prev = NULL;

    te = timeEventHead;
    while(te)
    {
        if(te->id == id)
        {
            if(prev == NULL)
            {
                timeEventHead = te->next;
            }
            else
            {
                prev->next = te->next;
            }

            if(te->finalizerProc)
            {
                te->finalizerProc(te->clientData);
            }
            free(te);
            return 0;
        }

        prev = te;
        te = te->next;
    }
    return 0;
}

evTimeEvent *EventLoop::evSearchNearestTimer()
{
    evTimeEvent *te = timeEventHead;
    evTimeEvent *nearest = NULL;

    while(te)
    {
        if(!nearest || te->when_sec < nearest->when_sec ||
                (te->when_sec == nearest->when_sec &&
                 te->when_ms < nearest->when_ms))
        {
            nearest = te;
        }
        te = te->next;
    }
    return nearest;
}

int EventLoop::processTimeEvents()
{
    int processed = 0;
    evTimeEvent *te;
    long long maxId;

    te = timeEventHead;
    maxId = timeEventNextId - 1;

    while(te)
    {
        long now_sec,now_ms;
        long long id;

        if(te->id > maxId)
        {
            te = te->next;
            continue;
        }

        evGetTime(&now_sec,&now_ms);
        if(now_sec > te->when_sec ||
               (now_sec == te->when_sec && now_ms >= te->when_ms))
        {
            int retval;
            id = te->id;
            retval = te->timeProc(id,te->clientData);
            processed++;

            if(retval != EVENT_NOMORE)
            {
                evAddMillisecondsToNow(retval,&te->when_sec,&te->when_ms);
            }
            else
            {
                evDeleteTimeEvent(id);
            }
            te = timeEventHead;
        }
        else
        {
            te = te->next;
        }
    }
    return processed;
}

int EventLoop::evProcessEvents(int flags)
{
    int processed = 0,numevents = 0;

    if(!(flags & EVENT_TIME_EVENTS) && !(flags & EVENT_FILE_EVENTS))
    {
        return 0;
    }

    if(maxfd != -1 ||
            ((flags & EVENT_TIME_EVENTS) && !(flags & EVENT_DONT_WAIT)))
    {
        int j;
        evTimeEvent *shortest = NULL;
        struct timeval tv,*tvp;

        if(flags & EVENT_TIME_EVENTS && !(flags & EVENT_DONT_WAIT))
        {
            shortest = evSearchNearestTimer();
        }
        if(shortest)
        {
            long now_sec,now_ms;
            evGetTime(&now_sec,&now_ms);
            tvp = &tv;
            tvp->tv_sec = shortest->when_sec - now_sec;
            if(shortest->when_ms < now_ms)
            {
                tvp->tv_usec = ((shortest->when_ms+1000) - now_ms)*1000;
                tvp->tv_sec --;
            }
            else
            {
                tvp->tv_usec = (shortest->when_ms - now_ms)*1000;
            }
            if (tvp->tv_sec < 0) tvp->tv_sec = 0;
            if (tvp->tv_usec < 0) tvp->tv_usec = 0;
        }
        else
        {
            if (flags & EVENT_DONT_WAIT) {
                tv.tv_sec = tv.tv_usec = 0;
                tvp = &tv;
            } else {
                /* Otherwise we can block */
                tvp = NULL; /* wait forever */
            }
        }

        /* epoll */
        int retval;

        retval = epoll_wait(state->epfd,state->events,EVENT_MAX,
                tvp ? (tvp->tv_sec*1000 + tvp->tv_usec/1000) : -1);
        if (retval > 0) {
            int j;

            numevents = retval;
            for (j = 0; j < numevents; j++) {
                int mask = 0;
                struct epoll_event *e = state->events+j;

                if(e->events & EPOLLIN) mask |= EVENT_READABLE;
                if(e->events & EPOLLOUT) mask |= EVENT_WRITABLE;
                if((e->events & EPOLLERR) ||
                   (e->events & EPOLLHUP))
                {
                    mask |= EVENT_ERRABLE;
                }
                fired[j].fd = e->data.fd;
                fired[j].mask = mask;
            }
        }

        for (j = 0; j < numevents; j++) {
            evFileEvent *fe = &events[fired[j].fd];
            int mask = fired[j].mask;
            int fd = fired[j].fd;
            int rfired = 0;

            /* note the fe->mask & mask & ... code: maybe an already processed
             * event removed an element that fired and we still didn't
             * processed, so we check if the event is still valid. */
            if (fe->mask & mask & EVENT_READABLE) {
                rfired = 1;
                fe->rfileProc(fd,fe->clientData,mask);
            }
            if (fe->mask & mask & EVENT_WRITABLE) {
                if (!rfired || fe->wfileProc != fe->rfileProc)
                    fe->wfileProc(fd,fe->clientData,mask);
            }
            if(fe->mask & mask & EVENT_ERRABLE)
            {
                fe->efileProc(fd,fe->clientData,mask);
            }
            processed++;
        }
    }

    if (flags & EVENT_TIME_EVENTS)
    {
        //printf("process time event...\n");
        processed += processTimeEvents();
    }

    return processed;
}

void EventLoop::evSetBeforeSleepProc(evBeforeSleepProc *beforesleep, void *clientData)
{
    beforesleepProc = beforesleep;
    beforesleepArgs = clientData;
}

void EventLoop::evStart()
{
    stop = false;
    while(!stop)
    {
        if(beforesleepProc)
        {
            beforesleepProc(beforesleepArgs);
        }

        evProcessEvents(EVENT_ALL_EVENTS);
    }
}
