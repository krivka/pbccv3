#ifndef PBLAZE_UTIL_H
#define PBLAZE_UTIL_H
#ifdef __cplusplus

class Emitter {
public:
    Emitter& operator<<(const char *s);
    Emitter& operator<<(unsigned long s);
private:
};

extern Emitter emit;

#endif // __cplusplus
#endif // PBLAZE_UTIL_H
