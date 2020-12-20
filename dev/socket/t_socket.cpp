#ifdef __linux__
#include <fcntl.h>
#include <unistd.h>
#include <net/if.h>
#include <strings.h>
#include <string.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <net/if_arp.h>
#include <sys/time.h>
#endif

#include "t_socket.h"

Socket::Socket(string devname,SockType type)
    :mType(type)
    ,mSockfd(-1)
    ,mDevName(devname)
{

}

Socket::~Socket()
{

}

bool Socket::isValid() const
{
    return mSockfd > 0 ? true : false;
}

/** 文件描述符 */
int Socket::socketDescriptor() const
{
    return mSockfd;
}


static inline int set_opt_keepalive(int fd, int interval)
{
    int val = 1;

    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val)) == -1)
    {
        return -1;
    }

    val = interval;
    /* 如果interval内没有任何数据交互,则进行探测,默认7200s*/
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &val, sizeof(val)) < 0) {
        return -1;
    }

    val = interval/3;
    /* 探测时发探测包的时间间隔为 interval / 3 秒*/
    if (val == 0) val = 1;
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &val, sizeof(val)) < 0) {
        return -1;
    }

    val = 3;
    /* 探测重试的次数 缺省9次*/
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &val, sizeof(val)) < 0) {
        return -1;
    }
    return 0;
}

static inline int set_opt_nodelay(int sockfd, int ok)
{
    /*控制ngale算法是否开启*/
    if(setsockopt(sockfd,IPPROTO_TCP,TCP_NODELAY,&ok,sizeof(ok)) == -1)
        return -1;
    return 0;
}

static inline int set_opt_sendbuffer(int fd, int buffsize)
{
    if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &buffsize, sizeof(buffsize)) == -1)
    {
       return -1;
    }
    return 0;
}

static inline int set_opt_sendtimeout(int fd, long long ms)
{
    struct timeval tv;

    tv.tv_sec = ms/1000;
    tv.tv_usec = (ms%1000)*1000;
    /*发送超时时间*/
    if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) == -1) {
        return -1;
    }
    return 0;
}

static inline int set_opt_ipv6_only(int sockfd)
{
    int yes = 1;
    if (setsockopt(sockfd,IPPROTO_IPV6,IPV6_V6ONLY,&yes,sizeof(yes)) == -1) {

        //close(sockfd);
        return -1;
    }
    return 1;
}

static inline int set_opt_reuseaddr(int sockfd)
{
    int mw_optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&mw_optval,sizeof(mw_optval));
}

void Socket::setSocketOption(SockOption opt,int value)
{
    if(isValid())
    {
        switch (opt)
        {
        case SOCKOPTION_KEEPALIVE:
            set_opt_keepalive(mSockfd,value);
            break;
        case SOCKOPTION_NODELAY:
            set_opt_nodelay(mSockfd,value);
            break;
        case SOCKOPTION_SNDBUFF:
            set_opt_sendbuffer(mSockfd,value);
            break;
        case SOCKOPTION_SNDTIMEOUT:
            set_opt_sendtimeout(mSockfd,value);
            break;
        case SOCKOPTION_ONLYIPV6:
            set_opt_ipv6_only(mSockfd);
            break;
        case SOCKOPTION_REUSEADDR:
            set_opt_reuseaddr(mSockfd);
            break;
        default:
            break;
        }
    }
}
Socket::SockType Socket::sockType() const
{
    return mType;
}


void Socket::close()
{
    if(isValid())
    {
        ::close(mSockfd);
    }
}

bool Socket::shutdown(ShutdownType type)
{
    if(!isValid())
        return false;

    if(type & SHUTDOWN_TYPE_READ)
        shutdown(mSockfd,SHUT_RD);
    if(type & SHUTDOWN_TYPE_WRITE)
        shutdown(mSockfd,SHUT_WR);

    return true;
}

bool Socket::setNonBlock()
{
    int flags = 0;
    flags = fcntl(mSockfd,F_GETFL,0);
    if(flags == -1)
        return false;
    if(fcntl(mSockfd,F_SETFL,flags | O_NONBLOCK) < 0)
        return false;
    return true;
}
bool Socket::setNonBlock(int sockfd)
{
    int flags = 0;
    flags = fcntl(sockfd,F_GETFL,0);
    if(flags == -1)
        return false;
    if(fcntl(sockfd,F_SETFL,flags | O_NONBLOCK) < 0)
        return false;
    return true;
}

string Socket::getIpAddr()
{
    if(!isValid())
        return "";

    struct ifreq ifr;
    bzero(&ifr,sizeof(struct ifreq));
    strcpy(ifr.ifr_name,mDevName.c_str());

    if(ioctl(mSockfd,SIOCGIFADDR,&ifr) < 0)
        return "";
    struct sockaddr_in *pAddr;
    pAddr = (struct sockaddr_in *)&(ifr.ifr_addr);

    char addr[64] = "";
    const char *ptr = inet_ntop(AF_INET,&(pAddr->sin_addr),addr,sizeof(addr));
    if(!ptr)
        return "";

    string value(addr);
    return value;
}
string Socket::getMacAddr()
{
    if(!isValid())
        return "";

    struct ifreq ifr;
    bzero(&ifr,sizeof(struct ifreq));
    strcpy(ifr.ifr_name,mDevName.c_str());

    if(ioctl(mSockfd,SIOCGIFHWADDR,&ifr) < 0)
        return "";

    char result[64] = "";
    sprintf(result,"%02x:%02x:%02x:%02x:%02x:%02x",(unsigned char)ifr.ifr_hwaddr.sa_data[0],\
                (unsigned char)ifr.ifr_hwaddr.sa_data[1],\
                (unsigned char)ifr.ifr_hwaddr.sa_data[2],\
                (unsigned char)ifr.ifr_hwaddr.sa_data[3],\
                (unsigned char)ifr.ifr_hwaddr.sa_data[4],\
                (unsigned char)ifr.ifr_hwaddr.sa_data[5]);
    string value(result);

    return value;
}
string Socket::getMask()
{
    if(!isValid())
        return "";
    struct sockaddr_in *pAddr;
    struct ifreq ifr;

    memset(&ifr,0,sizeof(ifr));
    strcpy(ifr.ifr_name,mDevName.c_str());

    if(ioctl(mSockfd,SIOCGIFNETMASK,&ifr) < 0)
    {
        return "";
    }


    pAddr = (struct sockaddr_in *)&(ifr.ifr_addr);
    char buf[64] = "";
    const char *ptr = inet_ntop(AF_INET,&(pAddr->sin_addr),buf,sizeof(buf));
    if(ptr == NULL)
    {
        return "";
    }

    string value(buf);

    return value;
}
/* dns 解析 */
string Socket::getIpByDomain(const char *domain)
{
    struct hostent *pHost = NULL;
    int j = 0;
    char *pszTemp = NULL;
    unsigned int uIP = 0;

    pHost=gethostbyname(domain);
    if(pHost == NULL)
    {
        return "";
    }

    pszTemp = (char*)&uIP;
    for(j=0; j<1&&NULL!=*(pHost->h_addr_list); pHost->h_addr_list++,j++)
    {
        memcpy(pszTemp,*(pHost->h_addr_list),pHost->h_length);
        break;
    }

    /**/
    char buf[64] = "";
    const char *ptr = inet_ntop(AF_INET,&uIP,buf,sizeof(buf));
    if(ptr == NULL)
    {
        return "";
    }

    string value(buf);
    return value;
}

bool Socket::interfaceUp(const char *devname)
{
    if(!devname)
        return false;

    int sockfd = socket(AF_INET,SOCK_DGRAM,0);
    if(sockfd < 0)
        return -1;

    struct ifreq ifr;
    strcpy(ifr.ifr_name,devname);
    short flag;
    flag = IFF_UP;

    if(ioctl(sockfd,SIOCGIFFLAGS,&ifr) < 0)
    {
        ::close(sockfd);
        return false;
    }


    ifr.ifr_ifru.ifru_flags |= flag;
    if(ioctl(sockfd,SIOCSIFFLAGS,&ifr) < 0)
    {
        ::close(sockfd);
        return false;
    }


    ::close(sockfd);
    return true;
}
bool Socket::interfaceDown()
{
    if(!devname)
        return false;

    int sockfd = socket(AF_INET,SOCK_DGRAM,0);
    if(sockfd < 0)
        return false;

    struct ifreq ifr;
    strcpy(ifr.ifr_name,devname);
    short flag;
    flag = ~IFF_UP;

    if(ioctl(sockfd,SIOCGIFFLAGS,&ifr) < 0)
    {
        ::close(sockfd);
        return false;
    }

    ifr.ifr_ifru.ifru_flags &= flag;
    if(ioctl(sockfd,SIOCSIFFLAGS,&ifr) < 0)
    {
        ::close(sockfd);
        return false;
    }

    ::close(sockfd);
    return false;
}
