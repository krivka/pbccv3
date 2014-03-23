#ifndef PBLAZE_UTIL_H
#define PBLAZE_UTIL_H
#ifdef __cplusplus

#define CRASH() do { *(int*)NULL=0; } while(0)
#define CRASH_IF(x) do { if (x) CRASH() } while(0)

class Emitter {
public:
    Emitter& operator<<(const char *s);
    Emitter& operator<<(unsigned long s);
private:
};

extern Emitter emit;

#endif // __cplusplus
#endif // PBLAZE_UTIL_H
