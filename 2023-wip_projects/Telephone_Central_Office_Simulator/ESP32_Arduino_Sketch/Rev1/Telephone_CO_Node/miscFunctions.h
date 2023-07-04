/*
  miscFunctions.h
*/
#ifndef miscFunctions_h
#define miscFunctions_h

#include "Arduino.h"

void initSysFlags();
void ringNorthAmericaInit();
void ringUkInit();
void initSlic();
void initMT8870();
void debug_print_sysflags();
bool offHook();
void ringGenTestStart();
void slicRingStop();
void slicRingGenerate();
char readDtmf();

#endif
