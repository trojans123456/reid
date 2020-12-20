#ifndef T_SOCKET_H
#define T_SOCKET_H

#include <stdint.h>
#include <string>

using std::string;


class Socket
{
public:
    enum SockType
    {
        SOCK_TYPE_TCP,
        SOCK_TYPE_UDP,
        SOCK_TYPE_CAN,
        SOCK_TYPE_UNIX,
        SOCK_TYPE_NETLINK
    };

    enum IpType
    {
        IP_TYPE_V4 = 1,
        IP_TYPE_V6,
        IP_TYPE_BOTH
    };

    enum LocalNetType
    {
        LOCAL_NET_TYPE_NONE,
        LOCAL_NET_TYPE_ETH, /* 有线 */
        LOCAL_NET_TYPE_PPP,
        LOCAL_NET_TYPE_LOOP
    };

    enum ShutdownType
    {
        SHUTDOWN_TYPE_READ = 1,
        SHUTDOWN_TYPE_WRITE = 2,
        SHUTDOWN_TYPE_BOTH
    };

    enum SockOption
    {
        SOCKOPTION_KEEPALIVE,
        SOCKOPTION_NODELAY,
        SOCKOPTION_SNDBUFF,
        SOCKOPTION_SNDTIMEOUT,
        SOCKOPTION_ONLYIPV6,
        SOCKOPTION_REUSEADDR
    };

    Socket(std::string devname, SockType type);
    virtual ~Socket();

    bool isValid() const;

    /** 文件描述符 */
    int socketDescriptor() const;

    void setSocketOption(SockOption opt,int value);
    SockType sockType() const;


    void close();

    bool shutdown(ShutdownType type = SHUTDOWN_TYPE_BOTH);

    bool setNonBlock();
    static bool setNonBlock(int sockfd);

    string getIpAddr();
    string getMacAddr();
    string getMask();
    string getDestAddr();

    /* dns 解析 */
    static string getIpByDomain(const char *domain);

    static bool interfaceUp(const char *devname);
    static bool interfaceDown();


private:
    SockType mType;
    int mSockfd;
    string mDevName;
};

#endif // T_SOCKET_H
