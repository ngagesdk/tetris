#ifndef _PLAY_H
#define _PLAY_H

#include "mut.h"
#include "comp.h"

#define PLAY_HEIGHT 15
#define PLAY_WIDTH  10

// Bug in OS 6.1: When the program in a loop changes a bitmap and draws it to
// the screen whithout a flush, only the last form of the bitmap will take
// effect, and this last form will be drawn everywhere. -> a flush is needed
// before each change of the bitmap. To make things faster we use up to
// STONEQUEUE bitmaps at once, so that flush is more seldom.
// Make sure that MAX_STONES_PER_ELEMENT <= STONEQUEUE

#define MAX_STONES_PER_ELEMENT 4
#define STONEQUEUE 10
#define NCOMP 2		// Maximum number of computer player

class CPlayTimer;

struct Player  {
  int  iIdx;
  int  iLevel;
  int  iScore;
  char iField[PLAY_HEIGHT][PLAY_WIDTH];	// The Field. empty field is ' '
  unsigned short iChars[PLAY_HEIGHT][PLAY_WIDTH]; // The Text in the field
  int  iElIdx, iNxtIdx;			// Idx of the current & the next el.
  int  iElNRot;				// Have to copy it, as extern elist
  					// does not work...
  TBuf<PLAY_WIDTH> iName;

  TPoint  iElPos;			// In stones
  TSize	  iElSize;			// In stones
  char	  iEl[4][4];

  TPoint  iSsp;				// Small Screen Position, changing
  int     iDizzyMove;                   // Number of dizzy moves
  Player* iDizzyFrom;                   // Who caused the dizziness
};


class CMutPlay : public CCoeControl
{
public:

  void ConstructL();
  void Draw(const TRect &aRect) const;
  TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType);

  void Start();
  void Cancel();
  void Reset(int idx);		// Use -1 to reset all of them.
  void Reset(Player *p);

  int DoMove(Player *p, int dir, int dist);
  int GenerateNewElement(Player *p);
  int AddScore(Player *p, int linesfound);
  void FloatingArrived(Player *p, Player *from);
  void SwitchToNextScreen();
  void DrawBgPlayer(int idx, int activate) const;
  void DoCompMove(int idx);

  void DrawFloatingElement(const Player *p, int draw, int activate) const;
  void CheckFaller(const Player *p);
  int ElementAtBottom(Player *p, int dropped, Player *from);

  int		iTransp;
  int		iLinesFallen;
  CFbsBitmap	*iBg;
  TSize		iPlaySize;	// In pixels
  int		iReadyToDraw;
  int           iNCmp;			// Number of comps configured
  Player	iPl[NCOMP+1];
  int           iFgIdx;			// Idx of Player currently displayed
  TBuf<32>	iName;
  int           iTimerRunning;

private:
  void Rotate(Player *p, int toright);
  void DrawStonePiece(TPoint pt, int idx, unsigned short content,
  			int stidx, int transp) const;
  int  IsElementFloating(const Player *p, TPoint off) const;;
  void SetInfo(const Player *p) const;
  void AddRandomLine(Player *to, Player *from);

  TSize		iStoneSize;		// In pixels
  Comp		iCmp[NCOMP];

  TSize		iScreenSize;		// In pixels
  TPoint	iOff;			// Playfield offset in pixels

  // If we send the same bitmap without flush, garbage occurs. We drow on
  // STONEQUE count differnt bitmaps, before we flush and reuse them again.
  CFbsBitmap	*iStone[STONEQUEUE];
  CFbsBitmap	*iSs; 			// Small Screen

  CPlayTimer	*iTimer;
  CFont		*iFtxt, *iFnum, *iFcnt;	// Text, numeric and content fonts
  TDisplayMode	iDpyMode;

  TInt64        iRandSeed;
};

#endif
