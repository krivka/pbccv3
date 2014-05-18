#ifndef PBLAZE_UTIL_H
#define PBLAZE_UTIL_H
#ifdef __cplusplus

#include <string>

class ICode;

#define CRASH() do { *(int*)NULL=0; } while(0)
#define CRASH_IF(x) do { if (x) CRASH() } while(0)

class Emitter {
public:
    Emitter& operator<<(const char &c);
    Emitter& operator<<(const char *s);
    Emitter& operator<<(unsigned int s);
    Emitter& operator<<(unsigned long s);
    Emitter& operator<<(const std::string &s);
    static int i;
private:
};

extern Emitter emit;

class Function {
public:
    static bool isMain;
    static int argumentCnt;
    static int registerSize;
    static int stackSize;
    static void processNew(ICode *ic);
};

#endif // __cplusplus
#endif // PBLAZE_UTIL_H
