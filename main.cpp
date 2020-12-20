#include <iostream>
#include "dev/serial/t_serialport.h"
#include "base/exception/t_exception.h"
#include "base/stropt/t_stropt.h"

int main(int argc,char *argv[])
{
    SerialPort *serial = new SerialPort("/dev/ttyO2");
    if(!serial)
        return -1;

    try
    {
        bool ret = serial->open();
        if(!ret)
            throw TError("serial open failed\n");
    }catch(TError &err)
    {
        std::cout << TError::getStack() <<std::endl;
    }

    string str = StrOpt::sprintf("hongtao%d\n",1);

    std::cout <<str <<std::endl;

    return 0;
}
