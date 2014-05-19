#ifdef __pbccv3
#   define END_EXECUTION do{ __asm("BREAK") }while(0)
#else
#   define END_EXECUTION
#endif

