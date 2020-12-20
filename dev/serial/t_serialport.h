#ifndef T_SERIALPORT_H
#define T_SERIALPORT_H

#include <stdint.h>

#include <string>
#include <list>

using std::list;
using std::string;

class SerialPortInfo;
class SerialPortPrivte;
class SerialPortInfoPrivate;


class SerialPort
{
public:
    enum Direction
    {
        Comm_Input = 1,
        Comm_Output = 2,
        Comm_All = Comm_Input | Comm_Output
    };

    enum BaudRate
    {
        Comm_Baud1200 = 1200,
        Comm_Baud2400 = 2400,
        Comm_Baud4800 = 4800,
        Comm_Baud9600 = 9600,
        Comm_Baud19200 = 19200,
        Comm_Baud38400 = 38400,
        Comm_Baud57600 = 57600,
        Comm_Baud115200 = 115200,
        Comm_UnknownBaud = -1
    };

    enum DataBits
    {
        Comm_Data5 = 5,
        Comm_Data6 = 6,
        Comm_Data7 = 7,
        Comm_Data8 = 8,
        Comm_UnknownDataBits = -1
    };
    enum Parity {
        Comm_NoParity = 0,
        Comm_EvenParity = 2,
        Comm_OddParity = 3,
        Comm_SpaceParity = 4,
        Comm_MarkParity = 5,
        Comm_UnknownParity = -1
    };

    enum StopBits
    {
        Comm_OneStop = 1,
        Comm_OneAndHalfStop = 3,
        Comm_TwoStop = 2,
        Comm_UnknownStopBits = -1
    };

    enum FlowControl
    {
        Comm_NoFlowControl,
        Comm_HardwareControl,
        Comm_SoftwareControl,
        Comm_UnknownFlowControl = -1
    };

    enum SerialPortError
    {
        Comm_NoError,
        Comm_DeviceNotFoundError,
        Comm_PermissionError,
        Comm_OpenError,
        Comm_ParityError,
        Comm_FramingError,
        Comm_BreakConditionError,
        Comm_WriteError,
        Comm_ReadError,
        Comm_ResourceError,
        Comm_UnsupportedOperationError,
        Comm_UnknownError,
        Comm_TimeoutError,
        Comm_NotOpenError
    };

    /** 显示构造*/
    explicit SerialPort();
    explicit SerialPort(const string &name);
    explicit SerialPort(const SerialPortInfo &info);
    virtual ~SerialPort();

    void setPortName(const string &name);
    string portName() const;

    void setPort(const SerialPortInfo &info);

    bool isOpen() const;

    /** 读写模式 */
    bool open(Direction dir=Comm_All);
    void close();

    bool setBaudRate(int32_t baudRate);
    int32_t BaudRate() const;

    bool setDataBits(DataBits dataBits);
    DataBits dataBits() const;

    bool setParity(Parity parity);
    Parity parity() const;

    bool setStopBits(StopBits stopBits);
    StopBits stopBits() const;

    bool setFlowControl(FlowControl flowControl);
    FlowControl flowControl() const;

    bool flush();
    /** 默认都是清理输入和输出的,如后续有需求需要分离 */
    void clear(Direction dir = Comm_All);

    SerialPortError error() const;

    SerialPortError getSystemError(int systemErrorCode = -1);
    void clearError();



    /** 内核层缓存 大小*/
    int64_t readBufferSize() const;


    int64_t readData(char *data,int64_t maxSize);

    int64_t writeData(const char *data,int64_t maxSize);

private:
    SerialPortError m_error;
    string m_systemLocation;
    int32_t m_baudRate;
    DataBits m_dataBits;
    Parity m_parity;
    StopBits m_stopBits;
    FlowControl m_flowControl;
    string m_portname;
    int m_fd;

};


class SerialPortInfo
{
public:
    SerialPortInfo();
    explicit SerialPortInfo(const SerialPort &port);
    explicit SerialPortInfo(const string &name);
    /** 拷贝构造 */
    SerialPortInfo(const SerialPortInfo &other);
    ~SerialPortInfo();
    /** 赋值拷贝*/
    SerialPortInfo& operator=(const SerialPortInfo &other);
    void swap(SerialPortInfo &other);

    string portName() const;
    string systemLocation() const;
    string description() const;
    string manufacturer() const;
    string serialNumber() const;

    uint16_t vendorIdentifier() const;
    uint16_t productIdendtifier() const;
    bool hasVendorIdentifier() const;
    bool hasProductIentifier() const;

    bool isNull() const;
    /** depend on udev not complete */
    static list<SerialPortInfo> avaliablePorts();

private:
    SerialPortInfo(const SerialPortInfoPrivate &dd);
    SerialPortInfoPrivate *d_ptr;
};

class SerialPortInfoPrivate
{
public:
    SerialPortInfoPrivate()
        :vendorIdentifier(0)
        ,productIdentifier(0)
        ,hasVendorIdentifier(false)
        ,hasProductIdentifier(false)
    {

    }

    ~SerialPortInfoPrivate()
    {

    }

    static string portNameToSystemLocation(const string &source);
    static string portNameFromSystemLocation(const string &source);

    string portName;
    string device;
    string description;
    string manufacturer;
    string serialNumber;

    uint16_t vendorIdentifier;
    uint16_t productIdentifier;

    bool hasVendorIdentifier;
    bool hasProductIdentifier;
};


#endif
