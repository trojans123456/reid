#ifndef T_TCPSOCKET_H
#define T_TCPSOCKET_H

#include "t_socket.h"

class TcpSocket : public Socket
{
public:
    TcpSocket(string hostname,string service,IpType iptype);
    TcpSocket();

    ~TcpSocket();

    bool setHostName(string hostname);
    bool setPort(uint16_t port);



    bool connect

private:
};

#endif // T_TCPSOCKET_H
