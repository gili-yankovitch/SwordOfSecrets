#ifndef __SECBOOT_H__
#define __SECBOOT_H__

// #define SETUP
// #define SOLVE

#ifdef SETUP

void setupQuest();

#else

int palisade();
int parapet();
int postern();
void plunderLoad();
int treasuryVisit();
void digForTreasure();

#endif
#endif // __SECBOOT_H__
