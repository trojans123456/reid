#include <math.h>
#include <time.h>
#ifdef __linux__
#include <pthread.h>
#include <arpa/inet.h> /* for htonl*/
#include <stdlib.h>
#endif

#include "t_stropt.h"



//int 转换为 str
string StrOpt::int2str( int val, IntView view)
{
    char buf[100];
    switch(view)
    {
    case StrOpt::Oct:	snprintf(buf, sizeof(buf), "%o", (unsigned)val);	break;
    case StrOpt::Hex:	snprintf(buf, sizeof(buf), "%x", (unsigned)val);	break;
    default: snprintf(buf, sizeof(buf), "%d", val);				break;
    }
    return buf;
}

string StrOpt::uint2str( unsigned val, IntView view)
{
    char buf[100];
    switch(view)
    {
    case StrOpt::Oct:	snprintf(buf, sizeof(buf), "%o", val);	break;
    case StrOpt::Hex:	snprintf(buf, sizeof(buf), "%x", val);	break;
    default: snprintf(buf, sizeof(buf), "%u", val);		break;
    }
    return buf;
}

string StrOpt::ll2str( long long val, IntView view)
{
    char buf[100];
    switch(view)
    {
    case StrOpt::Oct:	snprintf(buf, sizeof(buf), "%llo", (long long unsigned int)val);	break;
    case StrOpt::Hex:	snprintf(buf, sizeof(buf), "%llx", (long long unsigned int)val);	break;
    default: snprintf(buf, sizeof(buf), "%lld", (long long int)val);			break;
    }
    return buf;
}

//实数(double float)转换为字符串
string StrOpt::real2str( double val, int prec, char tp)
{
    char buf[100];
    prec = vmax(0, prec);
    switch(tp)
    {
    case 'g': snprintf(buf, sizeof(buf), "%.*g", prec, val);	break;
    case 'e': snprintf(buf, sizeof(buf), "%.*e", prec, val);	break;
    default: snprintf(buf, sizeof(buf), "%.*f", prec, val);		break;
    }
    return buf;
}

//时间转换
string StrOpt::atime2str( time_t itm, const string &format)
{
    struct tm tm_tm;
    localtime_r(&itm, &tm_tm);
    char buf[100];
    int ret = strftime(buf, sizeof(buf), format.empty()?"%d-%m-%Y %H:%M:%S":format.c_str(), &tm_tm);
    return (ret > 0) ? string(buf,ret) : string("");
}

string StrOpt::time2str( double tm )
{
    if(tm < 1e-12) return "0";
    int lev = 0;
    int days = (int)floor(tm/(24*60*60));
    int hours = (int)floor(tm/(60*60))%24;
    int mins = (int)floor(tm/(60))%60;
    double usec = 1e6 * (tm - days*24*60*60 - hours*60*60 - mins*60);

    string rez;
    if(days)		{ rez += int2str(days); lev = vmax(lev,6); }
    if(hours)		{ rez += (rez.size()?" ":"")+int2str(hours); lev = vmax(lev,5); }
    if(mins && lev < 6)	{ rez += (rez.size()?" ":"")+int2str(mins); lev = vmax(lev,4); }
    if((1e-6*usec) > 0.5 && lev < 5)	{ rez += (rez.size()?" ":"")+real2str(1e-6*usec,3); lev = vmax(lev,3); }
    else if((1e-3*usec) > 0.5 && !lev)	{ rez += (rez.size()?" ":"")+real2str(1e-3*usec,4); lev = vmax(lev,2); }
    else if(usec > 0.5 && !lev)		{ rez += (rez.size()?" ":"")+real2str(usec,4); lev = vmax(lev,1); }
    else if(!lev)	rez += (rez.size()?" ":"")+real2str(1e3*usec,4);

    return rez;
}

//
string StrOpt::cpct2str( double cnt )
{
    if(cnt > 0.2*pow(2,80))	return real2str(cnt/pow(2,80),3,'g');
    if(cnt > 0.2*pow(2,70))	return real2str(cnt/pow(2,70),3,'g');
    if(cnt > 0.2*pow(2,60))	return real2str(cnt/pow(2,60),3,'g');
    if(cnt > 0.2*pow(2,50))	return real2str(cnt/pow(2,50),3,'g');
    if(cnt > 0.2*pow(2,40))	return real2str(cnt/pow(2,40),3,'g');
    if(cnt > 0.2*pow(2,30))	return real2str(cnt/pow(2,30),3,'g');
    if(cnt > 0.2*pow(2,20))	return real2str(cnt/pow(2,20),3,'g');
    if(cnt > 0.2*pow(2,10))	return real2str(cnt/pow(2,10),3,'g');
    return real2str(cnt,3,'g');
}

int64_t StrOpt::curTime()
{
    timeval cur_tm;
    gettimeofday(&cur_tm, NULL);
    return (int64_t)cur_tm.tv_sec*1000000 + cur_tm.tv_usec;
}

inline int vasprintf__(char **strp,const char *fmt,va_list ap)
{
   char *res = NULL;
   int len;
   static char empty = '\0';

   len = vsnprintf(&empty,0,fmt,ap);

   if(len == -1)
   {
       *strp = NULL;
       return -1;
   }
   res = (char *)malloc(sizeof(char) * (len + 1));
   if(res == NULL)
   {
       *strp = NULL;
       return -1;
   }

   int ret = vsnprintf(res,len+1,fmt,ap);

   if(ret < 0)
   {
       free(res);
       *strp = NULL;
       return -1;
   }

   *strp = res;

   return len;
}


string StrOpt::sprintf(const char *fmt,...)
{
    va_list ap;
    int ret;
    char *ptr = NULL;
    string value("");

    va_start(ap,fmt);
    ret = vasprintf__(&ptr,fmt,ap);
    va_end(ap);

    if(ret < 0)
        return value;

    value.assign(ptr);
    free(ptr);

    return value;
}


// str转换为real
double StrOpt::str2real( const string &val )
{
    const char *chChr = val.c_str();

    //Pass spaces before
    for( ; true; ++chChr) {
        switch(*chChr) {
        case ' ': case '\t': continue;
        }
        break;
    }

    //Check and process the base
    bool isNeg = false, isExpNeg = false;
    double tVl = 0;
    int16_t nAftRdx = 0, tAftRdx = 0;
    if(*chChr && ((*chChr >= '0' && *chChr <= '9') || *chChr == '-' || *chChr == '+')) {
        if(*chChr == '+')	++chChr;
        else if(*chChr == '-')	{ isNeg = true; ++chChr; }
        for(bool notFirst = false; *chChr >= '0' && *chChr <= '9'; ++chChr, notFirst = true) {
            if(notFirst) tVl *= 10;
            tVl += *chChr - '0';
        }
    }
    if(*chChr == '.' || *chChr == ',') {
        for(++chChr; *chChr >= '0' && *chChr <= '9'; ++chChr, ++nAftRdx)
            tVl = tVl*10 + (*chChr - '0');
    }
    if(isNeg) tVl *= -1;

    //Read exponent
    if(*chChr && (*chChr == 'e' || *chChr == 'E')) {
        ++chChr;
        if(*chChr == '+')	++chChr;
        else if(*chChr == '-')	{ isExpNeg = true; ++chChr; }
        for(bool notFirst = false; *chChr >= '0' && *chChr <= '9'; ++chChr, notFirst = true) {
            if(notFirst) tAftRdx *= 10;
            tAftRdx += *chChr - '0';
        }
        if(isExpNeg) tAftRdx *= -1;
    }

    //Combine
    return tVl * pow(10, tAftRdx-nAftRdx);
}

// 地址转换为字符串
string StrOpt::addr2str( void *addr )
{
    char buf[sizeof(void*)*2+3];
    snprintf(buf,sizeof(buf),"%p",addr);

    return buf;
}

void * StrOpt::str2addr( const string &str )
{
    return (void *)strtoul(str.c_str(),NULL,16);
}

//> 去除字符串前后的\r\n\t
string StrOpt::strTrim( const string &val, const string &cfg)
{
    int beg = -1, end = -1;

    for(unsigned iS = 0, iC = 0; iS < val.size(); iS++) {
        for(iC = 0; iC < cfg.size() && val[iS] != cfg[iC]; iC++) ;
        if(iC < cfg.size())	continue;
        if(beg < 0) beg = iS;
        end = iS;
    }

    return (beg >= 0) ? val.substr(beg, end-beg+1) : "";
}

string StrOpt::strSimplified(const string &val)
{
   if(val.size() == 0)
       return val;

   const char *const start = val.c_str();
   const char *from = start;
   const char *fromEnd = start + val.size();

   char ch;
   //删除前空格
   for(;;)
   {
       ch = *from;
       if(!isspace(ch))
       {
           break;
       }
       if(++from == fromEnd)
       {
           //为全是空格的字符串
           return val;
       }
   }
   //删除后空格
   while(isspace(*(fromEnd - 1)))
   {
       fromEnd--;
   }

   const char *copyFrom = from;
   int copyCount;
   for(;;)
   {
       if(++from == fromEnd)
       {
           return val.substr(copyFrom - start,from - copyFrom);
       }
       ch = *from;
       if(!isspace(ch))
       {
           continue;
       }
       if(ch != ' ')
       {
           copyCount = from - copyFrom;
           break;
       }
       ch = *++from;
       if(isspace(ch))
       {
           copyCount = from - copyFrom - 1;
           break;
       }
   }

   string result((fromEnd - from) + copyCount,'0');
   char *to = (char *)result.c_str();
   memcpy(to,copyFrom,copyCount * 2);
   to += copyCount;
   fromEnd--;
   for(;;)
   {
       *to++ = ' ';
       do
       {
           ch= *++from;
       }while(isspace(ch));
       if(from == fromEnd)
           break;
       do
       {
           *to++ = ch;
           ch = *++from;
           if(from == fromEnd)
               goto done;
       }while(!isspace(ch));
   }
done:
   *to++ = ch;
   result.resize(to - reinterpret_cast<const char *>(result.c_str()));
   return result;
}

//字符串按等级分割 11.22.33.44 ——> 11:0 22:1 33:2 44:3
string StrOpt::strSepParse( const string &path, int level, char sep, int *off)
{
    int an_dir = off ? *off : 0;
    int t_lev = 0;
    size_t t_dir;

    if(an_dir >= (int)path.size()) return "";
    while(true) {
        t_dir = path.find(sep,an_dir);
        if(t_dir == string::npos) {
            if(off) *off = path.size();
            return (t_lev==level) ? path.substr(an_dir) : "";
        }
        else if(t_lev == level) {
            if(off) *off = t_dir+1;
            return path.substr(an_dir, t_dir-an_dir);
        }
        an_dir = t_dir+1;
        t_lev++;
    }
    return "";
}

string StrOpt::strParse( const string &path, int level, const string &sep, int *off, bool mergeSepSymb)
{
    int an_dir = off ? *off : 0;
    int t_lev = 0;
    size_t t_dir;

    if(an_dir >= (int)path.size() || sep.empty()) return "";
    while(true) {
        t_dir = path.find(sep,an_dir);
        if(t_dir == string::npos) {
            if(off) *off = path.size();
            return (t_lev==level) ? path.substr(an_dir) : "";
        }
        else if(t_lev == level) {
            if(off) *off = t_dir+sep.size();
            return path.substr(an_dir,t_dir-an_dir);
        }
        if(mergeSepSymb && sep.size() == 1)
            for(an_dir = t_dir; an_dir < (int)path.size() && path[an_dir] == sep[0]; ) an_dir++;
        else an_dir = t_dir+sep.size();
        t_lev++;
    }
    return "";
}

string StrOpt::strLine( const string &str, int level, int *off)
{
    int an_dir = off ? *off : 0;
    int t_lev = 0, edLnSmbSz = 1;
    size_t t_dir;
    //0x0a 0x0d 回车换行
    if(an_dir >= (int)str.size()) return "";
    while(true) {
        for(t_dir = an_dir; t_dir < str.size(); t_dir++)
            if(str[t_dir] == '\x0D' || str[t_dir] == '\x0A')
            { edLnSmbSz = (str[t_dir] == '\x0D' && ((t_dir+1) < str.size()) && str[t_dir+1] == '\x0A') ? 2 : 1; break; }
        if(t_dir >= str.size()) {
            if(off) *off = str.size();
            return (t_lev==level) ? str.substr(an_dir) : "";
        }
        else if(t_lev == level) {
            if(off) *off = t_dir+edLnSmbSz;
            return str.substr(an_dir,t_dir-an_dir);
        }
        an_dir = t_dir+edLnSmbSz;
        t_lev++;
    }
    return "";
}


string StrOpt::strMess( const char *fmt, ... )
{
    char str[1024];
    va_list argptr;

    va_start(argptr, fmt);
    vsnprintf(str, sizeof(str), fmt, argptr);
    va_end(argptr);

    return str;
}

string StrOpt::strMess( unsigned len, const char *fmt, ... )
{
    if(len <= 0) return "";

    char str[len];
    va_list argptr;

    va_start(argptr, fmt);
    int lenRez = vsnprintf(str, sizeof(str), fmt, argptr);
    va_end(argptr);

    return (lenRez < (int)len) ? string(str) : string(str)+"...";
}



string StrOpt::sepstr2path( const string &str, char sep)
{
    string rez, curv;
    int off = 0;
    while(!(curv=StrOpt::strSepParse(str,0,sep,&off)).empty()) rez += "/"+curv;

    return rez;
}

unsigned char StrOpt::getBase64Code( unsigned char asymb )
{
    switch(asymb)
    {
    case '+':	return 62;
    case '/':	return 63;
    }

    if(asymb >= 'A' && asymb <= 'Z')
        return asymb-(unsigned char)'A';

    if(asymb >= 'a' && asymb <= 'z')
        return 26+asymb-(unsigned char)'a';

    if(asymb >= '0' && asymb <= '9')
        return 52+asymb-(unsigned char)'0';

    return 0;
}


int b2str(const char* buf, const int bufLen, char *str, const int strLen, bool capital)
{
    const char* format = capital ? "%02X" : "%02x";
    int index = 0;
    for (int i = 0; i < bufLen && index < strLen - 2; i++, index += 2)
    {
        sprintf(str + index, format, (unsigned char)buf[i]);
    }
    str[index] = '\0';
    return index;
}

bool strIsDigit(const char *str, const int strLen)
{
    int len = strLen ? strLen : (int)strlen(str);
    for (int i = 0; i < len; i++)
    {
        if (!isdigit(str[i]))
        {
            return false;
        }
    }

    return true;
}

bool strIsAlpha(const char *str, const int strLen)
{
    int len = strLen ? strLen : (int)strlen(str);
    for (int i = 0; i < len; i++)
    {
        if (!isalpha(str[i]))
        {
            return false;
        }
    }

    return true;
}

bool strIsAlnum(const char *str, const int strLen)
{
    int len = strLen ? strLen : (int)strlen(str);
    for (int i = 0; i < len; i++)
    {
        if (!isalnum(str[i]))
        {
            return false;
        }
    }

    return true;
}

bool strIsUpperHex(const char *str, const int strLen)
{
    int len = strLen ? strLen : (int)strlen(str);
    for (int i = 0; i < len; i++)
    {
        if (!(isdigit(str[i]) || (str[i] >= 'A' && str[i] <= 'F')))
        {
            return false;
        }
    }

    return true;
}

bool strIsDate(const char *str, const int strLen)
{
    int len = strLen ? strLen : (int)strlen(str);
    if (10 != len)
    {
        return false;
    }

    if (strIsDigit(str, 4) && strIsDigit(str + 5, 2) && strIsDigit(str + 8, 2)
        && (str[4] == '-' || str[4] == '/') && (str[7] == '-' || str[7] == '/'))
    {
        return true;
    }

    return false;
}

unsigned int strToHashValue(const char *str, const int strLen)
{
    int len = strLen ? strLen : strlen(str);
    unsigned int hashValue = 0;
    int index = 0;
    for (; index <= len - 4; index += 4)
    {
        hashValue += *(unsigned int*)(&str[index]);
    }

    if (index < len)
    {
        char tmp[] = "3061";
        unsigned int remainLen = len - index;
        memcpy(tmp, str + index, remainLen);
        hashValue += *(unsigned int*)tmp;
    }

    return htonl(hashValue);
}
