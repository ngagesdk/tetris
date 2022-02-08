#ifndef _COMP_H
#define _COMP_H

struct Comp {
  int           iIdx;
  int 		iCol;
  int 		iRot;
  TInt64        iRandSeed;
};

struct Player;
void ComputePath(Comp *p, Player *p);

#endif

