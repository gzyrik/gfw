#ifndef __ADD_TEST_H__
#define __ADD_TEST_H__
#include <xos.h>

struct AddTest
{
    AddTest(const char* path, void (*func)(), const char* description=0); 
};
#define ADD_TEST                static AddTest X_PASTE(test,__LINE__)
#endif /* __ADD_TEST_H__ */
