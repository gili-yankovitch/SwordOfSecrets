#ifndef __SECBOOT_H__
#define __SECBOOT_H__

// #define SETUP

#ifdef SETUP

void setupQuest();

#else

int stage1();
int stage2();
int stage3();
void finalStageLoad();
int callFinalStage();
void finalDataExec();

#endif
#endif // __SECBOOT_H__
