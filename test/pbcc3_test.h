#ifdef __pbccv3
#   define END_EXECUTION __asm BREAK ; __endasm;
#else
#   define END_EXECUTION
#endif

