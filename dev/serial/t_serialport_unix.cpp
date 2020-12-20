#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#ifdef __linux__
#include <errno.h>
#include <termios.h>
#include <sys/ioctl.h>
#endif

#include "t_serialport.h"

static inline void set_common_props(struct termios *tio,int m)
{
    ::cfmakeraw(tio);

    tio->c_cflag |= CLOCAL;
    tio->c_cc[VTIME] = 0;
    tio->c_cc[VMIN] = 0;

    if (m & O_RDONLY)
        tio->c_cflag |= CREAD;
}

static inline void set_baud(struct termios *tio,int32_t baud)
{
    switch(baud)
    {
    case SerialPort::Comm_Baud1200:
        cfsetispeed(tio, B1200);
        cfsetospeed(tio, B1200);
        break;
    case SerialPort::Comm_Baud2400:
        cfsetispeed(tio, B2400);
        cfsetospeed(tio, B2400);
        break;
    case SerialPort::Comm_Baud4800:
        cfsetispeed(tio, B4800);
        cfsetospeed(tio, B4800);
        break;
    case SerialPort::Comm_Baud9600:
        cfsetispeed(tio, B9600);
        cfsetospeed(tio, B9600);
        break;
    case SerialPort::Comm_Baud19200:
        cfsetispeed(tio, B19200);
        cfsetospeed(tio, B19200);
        break;
    case SerialPort::Comm_Baud38400:
        cfsetispeed(tio, B38400);
        cfsetospeed(tio, B38400);
        break;
    case SerialPort::Comm_Baud57600:
        cfsetispeed(tio, B57600);
        cfsetospeed(tio, B57600);
        break;
    case SerialPort::Comm_Baud115200:
        cfsetispeed(tio, B115200);
        cfsetospeed(tio, B115200);
    default:
        break;
    }
}

static inline void set_databits(struct termios *tio, SerialPort::DataBits databits)
{
    tio->c_cflag &= ~CSIZE;
    switch (databits) {
    case SerialPort::Comm_Data5:
        tio->c_cflag |= CS5;
        break;
    case SerialPort::Comm_Data6:
        tio->c_cflag |= CS6;
        break;
    case SerialPort::Comm_Data7:
        tio->c_cflag |= CS7;
        break;
    case SerialPort::Comm_Data8:
        tio->c_cflag |= CS8;
        break;
    default:
        tio->c_cflag |= CS8;
        break;
    }
}

static inline void set_parity(struct termios *tio, SerialPort::Parity parity)
{
    tio->c_iflag &= ~(PARMRK | INPCK);
    tio->c_iflag |= IGNPAR;

    switch (parity) {

#ifdef CMSPAR
    // Here Installation parity only for GNU/Linux where the macro CMSPAR.
    case SerialPort::Comm_SpaceParity:
        tio->c_cflag &= ~PARODD;
        tio->c_cflag |= PARENB | CMSPAR;
        break;
    case SerialPort::Comm_MarkParity:
        tio->c_cflag |= PARENB | CMSPAR | PARODD;
        break;
#endif
    case SerialPort::Comm_NoParity:
        tio->c_cflag &= ~PARENB;
        break;
    case SerialPort::Comm_EvenParity:
        tio->c_cflag &= ~PARODD;
        tio->c_cflag |= PARENB;
        break;
    case SerialPort::Comm_OddParity:
        tio->c_cflag |= PARENB | PARODD;
        break;
    default:
        tio->c_cflag |= PARENB;
        tio->c_iflag |= PARMRK | INPCK;
        tio->c_iflag &= ~IGNPAR;
        break;
    }
}

static inline void set_stopbits(struct termios *tio, SerialPort::StopBits stopbits)
{
    switch (stopbits) {
    case SerialPort::Comm_OneStop:
        tio->c_cflag &= ~CSTOPB;
        break;
    case SerialPort::Comm_TwoStop:
        tio->c_cflag |= CSTOPB;
        break;
    default:
        tio->c_cflag &= ~CSTOPB;
        break;
    }
}

static inline void set_flowcontrol(struct termios *tio, SerialPort::FlowControl flowcontrol)
{
    switch (flowcontrol) {
    case SerialPort::Comm_NoFlowControl:
        tio->c_cflag &= ~CRTSCTS;
        tio->c_iflag &= ~(IXON | IXOFF | IXANY);
        break;
    case SerialPort::Comm_HardwareControl:
        tio->c_cflag |= CRTSCTS;
        tio->c_iflag &= ~(IXON | IXOFF | IXANY);
        break;
    case SerialPort::Comm_SoftwareControl:
        tio->c_cflag &= ~CRTSCTS;
        tio->c_iflag |= IXON | IXOFF | IXANY;
        break;
    default:
        tio->c_cflag &= ~CRTSCTS;
        tio->c_iflag &= ~(IXON | IXOFF | IXANY);
        break;
    }
}

static inline bool getTermios(int fd,struct termios *tio)
{
    ::memset(tio,0,sizeof(struct termios));
    if(::tcgetattr(fd,tio) == -1)
        return false;
    return true;
}

static inline bool setTermios(int fd,struct termios *tio)
{
    if(::tcsetattr(fd,TCSANOW,tio) == -1)
        return false;
    return true;
}

SerialPort::SerialPort()
    :m_error(Comm_NoError)
    ,m_systemLocation("")
    ,m_baudRate(115200)
    ,m_dataBits(Comm_Data8)
    ,m_parity(Comm_NoParity)
    ,m_stopBits(Comm_OneStop)
    ,m_flowControl(Comm_NoFlowControl)
    ,m_portname("")
    ,m_fd(-1)
{

}

SerialPort::SerialPort(const string &name)
    :m_error(Comm_NoError)
    ,m_systemLocation("")
    ,m_baudRate(115200)
    ,m_dataBits(Comm_Data8)
    ,m_parity(Comm_NoParity)
    ,m_stopBits(Comm_OneStop)
    ,m_flowControl(Comm_NoFlowControl)
    ,m_portname(name)
    ,m_fd(-1)
{

}

SerialPort::SerialPort(const SerialPortInfo &info)
    :m_error(Comm_NoError)
    ,m_systemLocation("")
    ,m_baudRate(115200)
    ,m_dataBits(Comm_Data8)
    ,m_parity(Comm_NoParity)
    ,m_stopBits(Comm_OneStop)
    ,m_flowControl(Comm_NoFlowControl)
    ,m_portname("")
    ,m_fd(-1)
{
    m_systemLocation = info.systemLocation();
}

SerialPort::~SerialPort()
{
    if(m_fd > 0)
        close();
}

void SerialPort::setPortName(const string &name)
{
    m_systemLocation = SerialPortInfoPrivate::portNameToSystemLocation(name);
}

string SerialPort::portName() const
{
    return SerialPortInfoPrivate::portNameFromSystemLocation( m_systemLocation);
}

void SerialPort::setPort(const SerialPortInfo &info)
{
    m_systemLocation = info.systemLocation();
}

bool SerialPort::isOpen() const
{
    return m_fd > 0 ? true : false;
}

/** 读写模式 */
bool SerialPort::open(Direction dir)
{
    int flags = O_NOCTTY | O_NDELAY;

    if(isOpen())
        return false;

    clearError();

    switch(dir)
    {
    case Comm_Input:
        flags |= O_RDONLY;
        break;
    case Comm_Output:
        flags |= O_WRONLY;
        break;
    case Comm_All:
    default:
        flags |= O_RDWR;
        break;
    }

    if(m_systemLocation.empty() ||
       access(m_systemLocation.c_str(),F_OK) != 0)
    {
        return false;
    }

    m_fd = ::open(m_systemLocation.c_str(),flags);
    if(m_fd < 0)
    {
        m_error = Comm_OpenError;
        return false;
    }

    struct termios tio;
    getTermios(m_fd,&tio);

    set_common_props(&tio,flags);
    set_databits(&tio,m_dataBits);
    set_parity(&tio,m_parity);
    set_stopbits(&tio,m_stopBits);
    set_flowcontrol(&tio,m_flowControl);
    set_baud(&tio,m_baudRate);

    setTermios(m_fd,&tio);

    return true;
}

void SerialPort::close()
{
    if(!isOpen())
    {
        m_error = Comm_NotOpenError;
    }
    ::close(m_fd);
}

bool SerialPort::setBaudRate(int32_t baudRate)
{
    struct termios tio;
    if(!getTermios(m_fd,&tio))
        return false;
    set_baud(&tio,baudRate);

    return setTermios(m_fd,&tio);
}

int32_t SerialPort::BaudRate() const
{
    return m_baudRate;
}

bool SerialPort::setDataBits(DataBits dataBits)
{
    struct termios tio;
    if(!getTermios(m_fd,&tio))
        return false;
    set_databits(&tio,dataBits);

    return setTermios(m_fd,&tio);
}

SerialPort::DataBits SerialPort::dataBits() const
{
    return m_dataBits;
}

bool SerialPort::setParity(Parity parity)
{
    struct termios tio;
    if(!getTermios(m_fd,&tio))
        return false;
    set_parity(&tio,parity);

    return setTermios(m_fd,&tio);
}

SerialPort::Parity SerialPort::parity() const
{
    return m_parity;
}

bool SerialPort::setStopBits(StopBits stopBits)
{
    struct termios tio;
    if(!getTermios(m_fd,&tio))
        return false;
    set_stopbits(&tio,stopBits);

    return setTermios(m_fd,&tio);
}

SerialPort::StopBits SerialPort::stopBits() const
{
    return m_stopBits;
}

bool SerialPort::setFlowControl(FlowControl flowControl)
{
    struct termios tio;
    if(!getTermios(m_fd,&tio))
        return false;
    set_flowcontrol(&tio,flowControl);

    return setTermios(m_fd,&tio);
}

SerialPort::FlowControl SerialPort::flowControl() const
{
    return m_flowControl;
}

bool SerialPort::flush()
{
    if(!isOpen())
        return false;
    tcflush(m_fd,TCOFLUSH);
    tcflush(m_fd,TCIFLUSH);
    return true;
}

/** 默认都是清理输入和输出的,如后续有需求需要分离 */
void SerialPort::clear(SerialPort::Direction dir)
{
    switch(dir)
    {
    case Comm_Input:
        break;
    case Comm_Output:
        break;
    case Comm_All:
    default:
        break;
    }
}

SerialPort::SerialPortError SerialPort::error() const
{
    return m_error;
}


SerialPort::SerialPortError SerialPort::getSystemError(int systemErrorCode)
{
    if (systemErrorCode == -1)
        systemErrorCode = errno;

    switch (systemErrorCode) {
    case ENODEV:
        m_error = SerialPort::Comm_DeviceNotFoundError;
        break;
#ifdef ENOENT
    case ENOENT:
        m_error = SerialPort::Comm_DeviceNotFoundError;
        break;
#endif
    case EACCES:
        m_error = SerialPort::Comm_PermissionError;
        break;
    case EBUSY:
        m_error = SerialPort::Comm_PermissionError;
        break;
    case EAGAIN:
        m_error = SerialPort::Comm_ResourceError;
        break;
    case EIO:
        m_error = SerialPort::Comm_ResourceError;
        break;
    case EBADF:
        m_error = SerialPort::Comm_ResourceError;
        break;
#ifdef Q_OS_OSX
    case ENXIO:
        m_error = SerialPort::Comm_ResourceError;
        break;
#endif
#ifdef EINVAL
    case EINVAL:
        m_error = SerialPort::Comm_UnsupportedOperationError;
        break;
#endif
#ifdef ENOIOCTLCMD
    case ENOIOCTLCMD:
        m_error = SerialPort::Comm_UnsupportedOperationError;
        break;
#endif
#ifdef ENOTTY
    case ENOTTY:
        m_error = SerialPort::Comm_UnsupportedOperationError;
        break;
#endif
#ifdef EPERM
    case EPERM:
        m_error = SerialPort::Comm_PermissionError;
        break;
#endif
    default:
        m_error = SerialPort::Comm_UnknownError;
        break;
    }
    return m_error;
}

void SerialPort::clearError()
{
    m_error = Comm_NoError;
}

/** 内核层缓存 大小*/
int64_t SerialPort::readBufferSize() const
{
    int bytes = 0;
    if(ioctl(m_fd,FIONREAD,&bytes) < 0)
        bytes = 0;
    return bytes;
}


int64_t SerialPort::readData(char *data,int64_t maxSize)
{
    int cur = 0,read_len = 0;

    while(cur < maxSize)
    {
        read_len = ::read(m_fd,data + cur,maxSize - cur);
        if(read_len < 0)
        {
            if(errno == EINTR ||
                    errno == EAGAIN)
            {
                continue;
            }
            return -1;
        }
        if(read_len == 0)
            break;

        cur += read_len;
    }
    return cur;
}


int64_t SerialPort::writeData(const char *data,int64_t maxSize)
{
    int cur = 0;
    int written = 0;
    while(cur < maxSize)
    {
        written = ::write(m_fd,data + cur,maxSize - cur);
        if(written < 0)
        {
            /*被中断 或资源不足*/
            if(errno == EINTR ||
                    errno == EAGAIN)
            {
                continue;
            }
            return -1;
        }
        if(written == 0)
            break;

        cur += written;
    }
    return cur;
}


SerialPortInfo::SerialPortInfo()
    :d_ptr(NULL)
{

}

SerialPortInfo::SerialPortInfo(const SerialPortInfo &other)
    :d_ptr(other.d_ptr ? new SerialPortInfoPrivate(*other.d_ptr) : NULL)
{

}

SerialPortInfo::SerialPortInfo(const SerialPort &port)
   // :SerialPortInfo(port.portName()) 委托构造 c++11
{
    SerialPortInfo(port.portName());
}

SerialPortInfo::SerialPortInfo(const std::string &name)
{
    list<SerialPortInfo> infos;
    infos.clear();
    infos = SerialPortInfo::avaliablePorts();
    list<SerialPortInfo>::iterator it;
    for(it = infos.begin();it != infos.end();it++)
    {
        if(name == (*it).portName())
        {
            *this = *it;
            break;
        }
    }
}

SerialPortInfo::SerialPortInfo(const SerialPortInfoPrivate &dd)
    :d_ptr(new SerialPortInfoPrivate(dd))
{

}

SerialPortInfo::~SerialPortInfo()
{
    if(d_ptr)
        delete d_ptr;
}

void SerialPortInfo::swap(SerialPortInfo &other)
{
    std::swap(*this,other);
}

SerialPortInfo &SerialPortInfo::operator =(const SerialPortInfo &other)
{
    SerialPortInfo(other).swap(*this);
    return *this;
}

string SerialPortInfo::portName() const
{
    return d_ptr ? d_ptr->portName : "";
}
/** 返回系统位置 */
string SerialPortInfo::systemLocation() const
{
    return d_ptr ? d_ptr->device : "";
}

string SerialPortInfo::description() const
{
    return d_ptr ? d_ptr->description : "";
}

string SerialPortInfo::manufacturer() const
{
    return d_ptr ? d_ptr->manufacturer : "";
}

string SerialPortInfo::serialNumber() const
{
    return d_ptr ? d_ptr->serialNumber : "";
}

uint16_t SerialPortInfo::vendorIdentifier() const
{
    return d_ptr ? d_ptr->vendorIdentifier : 0;
}

uint16_t SerialPortInfo::productIdendtifier() const
{
    return d_ptr ? d_ptr->productIdentifier : 0;
}

bool SerialPortInfo::hasVendorIdentifier() const
{
    return d_ptr ? d_ptr->hasVendorIdentifier : false;
}

bool SerialPortInfo::hasProductIentifier() const
{
    return d_ptr ? d_ptr->hasProductIdentifier : false;
}

bool SerialPortInfo::isNull() const
{
    return d_ptr ? true : false;
}

/** 这里要通过udev 获取 */
list<SerialPortInfo> SerialPortInfo::avaliablePorts()
{
    list<SerialPortInfo> unsed_;
    return unsed_;
}

string SerialPortInfoPrivate::portNameToSystemLocation(const std::string &source)
{
    return (source.compare("/")
            || source.compare("./")
            || source.compare("../"))
            ? source : (string("/dev/") + source);
}

string SerialPortInfoPrivate::portNameFromSystemLocation(const std::string &source)
{
    return source.compare("/dev/") ? source.substr(5) : source;
}
