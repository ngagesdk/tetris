#include "e32math.h"
#include "play.h"
#include "comp.h"
#include "util.h"

#define PLAY_HEIGHT 15
#define PLAY_WIDTH  10

void Rotate(Player *p, int rnum, int *el, int *wp, int *hp);
int EvalField(unsigned int *fld);

void
ComputePath(Comp *c, Player *p)
{
//debugline("--------- CP: %d", p->iElIdx);
  // First convert the field into a bitmask.
  unsigned int fld[PLAY_HEIGHT];
  int hiscore = -200000;

  for(int i = 0; i < PLAY_HEIGHT; i++)
    {
      fld[i] = 0;
      for(int j = 0; j < PLAY_WIDTH; j++)
        if(p->iField[i][j] != ' ')
	  fld[i] |= (1<<(PLAY_WIDTH-j-1));
    }

  // For rotation 0 to # of rotations the field gets different.
  for(int r = 0; r < p->iElNRot; r++)
    {
      // First rotate the element and transform it into bits
      int w, h, el[4];
      Rotate(p, r, el, &w, &h);

      for(int col = PLAY_WIDTH-w; col >= 0; col--)	// For each column
        {
	  int cline = PLAY_HEIGHT-h;
	  for(int i = 0; i <= PLAY_HEIGHT-h; i++)	// Let it fall
	    for(int j = 0; j < h; j++)
	      if(fld[i+j] && (fld[i+j] ^ el[j]) != (fld[i+j] | el[j]))
		{
		  cline = i-1;
		  goto COMPSCORE;
		}

COMPSCORE:
	  for(int j = 0; j < h; j++)			// Put the element
	    fld[cline+j] ^= el[j];

	  int score = EvalField(fld);

	  for(int j = 0; j < h; j++)			// Delete the element
	    fld[cline+j] ^= el[j];


	  for(int i = 0; i < h; i++)			// Next column
	    el[i] <<= 1;

	  if(score > hiscore)
	    {
	      hiscore = score;
	      c->iCol = col;
	      c->iRot = r;
//debugline("Hiscore %d @ Col %d / Rot %d", score, col, r);
	    }
	}
    }
//debugline("--------- CP result:%d/%d", c->iCol, c->iRot);

}

void
Rotate(Player *p, int rnum, int *el, int *wp, int *hp)
{
  char lbuf[4][4], nbuf[4][4];
  int r,i, j, w, h;

  w = p->iElSize.iWidth;
  h = p->iElSize.iHeight;

  for(i = 0; i < h; i++)
    for(j = 0; j < w; j++)
      lbuf[i][j] = p->iEl[i][j];

  for(r = 0; r < rnum; r++)
    {
      for(i = 0; i < h; i++)
	for(j = 0; j < w; j++)
	  nbuf[j][h-i-1] = lbuf[i][j];
      i = w; w = h; h = i;

      for(i = 0; i < h; i++)
	for(j = 0; j < w; j++)
	  lbuf[i][j] = nbuf[i][j];
    }

  *wp = w;
  *hp = h;
  for(i = 0; i < h; i++)
    {
      el[i] = 0;
      for(j = 0; j < w; j++)
	if(lbuf[i][j] == '#')
	  el[i] |= (1<<(w-1-j));
    }

//  for(i = 0; i < h; i++)
//    debugline("R%d: %02x", rnum, el[i]);
//  debugline("Rwh: %d %d", w, h);
}

int
EvalField(unsigned int *fld)
{
  // Calculate the Value of the field
  int score = 0, first = 0;
  for(int i = 2; i < PLAY_HEIGHT; i++)
    {
      if(!fld[i])
	continue;
      if(!first)
	first = i;

      int sign = 1, counter = 0, morelines = 0;
      for(int j = 0; j < PLAY_WIDTH; j++)
	{
	  int x = (fld[i] & (1<<(PLAY_WIDTH-j-1))) ? 1 : 0;

	  // The more stones the better. Lower stones are better.
	  if(x)
	    score += i, counter++;

	  // Covered holes are bad. Check up to two rows upwards
	  if(!x && 
		((fld[i-1] && (fld[i-1] & (1<<(PLAY_WIDTH-j-1)))) ||
		 (fld[i-2] && (fld[i-2] & (1<<(PLAY_WIDTH-j-1))))))
	    score -= 30;

	  // The less "holes" the better. Holes deeper are worse
	  if(x != sign)
	    score -= (i>>2);

	  sign = x;
	}
      if(!sign)
        score -= (i>>2);

      if(counter == PLAY_WIDTH)
	{
	  // A single line gets more important if the lot is high
	  score += (PLAY_WIDTH-first)*PLAY_WIDTH;

	  // Double lines are even more important...
	  if(morelines++)
	    score += 500;
	}
//debugline("L:%d F: %03x Sc:%d Cnt:%d morelines:%d",i,fld[i],score,counter,morelines);
    }
  return score;
}
