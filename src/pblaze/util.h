#ifndef PBLAZE_UTIL_H
#define PBLAZE_UTIL_H
#ifdef __cplusplus

#include <string>
#include <cstring>
#include <iostream>
#include <sstream>
using std::stringstream;

#define emit Emitter::_emit
class ICode;

#define CRASH() do { *(int*)NULL=0; } while(0)
#define CRASH_IF(x) do { if (x) CRASH() } while(0)

class Emitter {
public:
    Emitter() { }
    Emitter& operator<<(const char &c);
    Emitter& operator<<(const char *s);
    Emitter& operator<<(unsigned int s);
    Emitter& operator<<(unsigned long s);
    Emitter& operator<<(const std::string &s);
    static int i;
    static const char *contents() {
        return strdup(Emitter::ss.str().c_str());
    }
    static stringstream ss;
    static Emitter _emit;
private:
};

class Function {
public:
    static bool isMain;
    static int argumentCnt;
    static int registerSize;
    static int stackSize;
    static int futureSP;
    static void processNew(ICode *ic);
};

#endif // __cplusplus
#endif // PBLAZE_UTIL_H
