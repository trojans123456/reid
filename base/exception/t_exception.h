#ifndef T_EXCEPTION_H
#define T_EXCEPTION_H

#include <string>

using std::string;

class TError
{
public:
    TError():mMsg(""),mCode(-1) {}
    TError(string msg,int code):mMsg(msg),mCode(code) {}
    TError(string msg):mMsg(msg),mCode(-1) {}

    string &getMessage() {return mMsg;}
    int &getCode() {return mCode;}

    static string getStack();
    static string lastError();

private:
    string mMsg;
    int mCode;
};


#endif
