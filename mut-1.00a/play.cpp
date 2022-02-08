#include "play.h"
#include "util.h"
#include <e32math.h>
#include "Mut.rsg"

////////////////////////////////////////////
// Some static data
//

// Take care! an element can only have MAX_STONES_PER_ELEMENT stones at max.
const struct ElementDesc {
  char * const iForm;
  int iWidth, iHeight;
  int iRed, iGreen, iBlue;
  int iNRot;
} elist[] = {
  { "##"
    "##",   2,2,  120,120,120,  1 },

  { "####", 4,1,  240,  0,  0,  2 },
                           
  { "###"                    
    " # ",  3,2,    0,220,220,  4 },
                           
  { "###"                    
    "#  ",  3,2,   60, 60,240,  4 },
                           
  { "###"                    
    "  #",  3,2,    0,240,  0,  4 },
                           
  { "## "                    
    " ##",  3,2,  240,220, 80,  2 },
                           
  { " ##"                    
    "## ",  3,2,  220,  0,220,  2 },
};

#define NELS (sizeof(elist)/sizeof(*elist))
#define NLEVELS 9

static const struct {
  int iRows[MAX_STONES_PER_ELEMENT];	// Points for 1/2/3/4 rows
  int iFaller;			// Points for letting an element fall
  int iNxtTimeout;		// How many moves do we show the next piece
  int iTimeout;			// Timeout (usec)
} level[NLEVELS] =  {
  { { 20, 80,180, 320 }, 10, 15, 2200000 }, // 1
  { { 15, 60,135, 240 },  5, 12, 1822222 }, // 2
  { { 10, 40, 90, 160 },  2,  9, 1491666 }, // 3
  { {  8, 32, 72, 128 },  1,  6, 1208333 }, // 4
  { {  5, 20, 45,  80 },  0,  3,  972222 }, // 5
  { {  3, 12, 27,  48 },  0,  2,  783333 }, // 6
  { {  2,  8, 18,  32 },  0,  1,  641666 }, // 7
  { {  1,  4,  9,  16 },  0,  0,  547222 }, // 8
  { {  0,  1,  4,   9 },  0,  0,  500000 }  // 9
};


/////////////////////////////////////
// Timeout helper class
class CPlayTimer : public CTimer
{
public:

  CPlayTimer(CMutPlay *aPlay);
  void ConstructL();
  void RunL();

  CMutPlay *iPlay;
};


CPlayTimer::CPlayTimer(CMutPlay *aPlay)
	: CTimer(EPriorityHigh)
{ 
  iPlay = aPlay;
}


void
CPlayTimer::ConstructL()
{
  CTimer::ConstructL();
}


void
CPlayTimer::RunL()
{
  int ok = 1;
  for(int i = 0; i < iPlay->iNCmp+1; i++)
    {
      Player *p = iPlay->iPl + i;
      if(i == 0)
        {
	  if(iPlay->DoMove(p, 0, 1))
	    ok = iPlay->ElementAtBottom(p, 0, 0);
	  else
	    iPlay->CheckFaller(p);
	}
      else
        {
          iPlay->DoCompMove(i);
	}
    }

  // SmallScreen Update
  for(int i = 0; i <= iPlay->iNCmp; i++)
    {
      if(i == iPlay->iFgIdx)
	continue;
      iPlay->DrawBgPlayer(i, 1);
    }
  if(ok)
    iPlay->Start();
}


/////////////////////////////////////
// The Play functions
void
CMutPlay::ConstructL()
{
  iScreenSize = iCoeEnv->ScreenDevice()->SizeInPixels();
  iDpyMode = iCoeEnv->ScreenDevice()->DisplayMode();


  CreateWindowL();
  SetExtent(TPoint(0,0), iScreenSize);
 
  iStoneSize.iWidth  = iScreenSize.iWidth / PLAY_WIDTH;
  iStoneSize.iHeight = (iScreenSize.iHeight + PLAY_HEIGHT - 1) / PLAY_HEIGHT;
  if(iStoneSize.iWidth > iStoneSize.iHeight)
    iStoneSize.iWidth = iStoneSize.iHeight;
  else
    iStoneSize.iHeight = iStoneSize.iWidth;

  iPlaySize.iWidth  = iStoneSize.iWidth  * PLAY_WIDTH;
  iPlaySize.iHeight = iStoneSize.iHeight * PLAY_HEIGHT;
  //iOff.iX = (iScreenSize.iWidth  - iPlaySize.iWidth )/2;
  iOff.iX = 0;
  iOff.iY = iScreenSize.iHeight - iPlaySize.iHeight;

  for(int i = 0; i < STONEQUEUE; i++)
    {
      iStone[i] = new CFbsBitmap();
      iStone[i]->Create(iStoneSize, iDpyMode);
    }
  iSs = new CFbsBitmap();
  iSs->Create(TSize(PLAY_WIDTH+2, PLAY_HEIGHT+2), iDpyMode);

  iTimer = new CPlayTimer(this);
  iTimer->ConstructL();

  TTime t;
  t.HomeTime();
  iRandSeed = t.Int64();

  for(int i = 0; i < NCOMP; i++)
    {
      iCmp[i].iIdx = i;
      iCmp[i].iRandSeed = (i+1)*iRandSeed.Low();
    }

  TFontSpec fontSpec;
  fontSpec.iTypeface.iName = _L("LatinBold19");
  iEikonEnv->ScreenDevice()->GetNearestFontInTwips(iFtxt, fontSpec);
  fontSpec.iTypeface.iName = _L("Acb14");
  iEikonEnv->ScreenDevice()->GetNearestFontInTwips(iFnum, fontSpec);
  fontSpec.iTypeface.iName = _L("LatinBold12");
  iEikonEnv->ScreenDevice()->GetNearestFontInTwips(iFcnt, fontSpec);

  iTimerRunning = 0;

  CActiveScheduler::Add(iTimer);
  ActivateL();
}




////////////////////////////////////////////////////////
// Draw a single stone at position pt, with transparency
//
void
CMutPlay::DrawStonePiece(TPoint pt, int idx,
			unsigned short content, int stidx, int transp) const
{
  TBitmapUtil* dest = new TBitmapUtil(iStone[stidx]);
  TBitmapUtil* src = 0;

  if(iBg && iTransp && transp)
    {
      src = new TBitmapUtil(iBg);
      src->Begin(pt);
    }
  dest->Begin(TPoint(0,0));

  int rbit = 0, gbit = 0;

  if(iDpyMode == EColor64K)
    rbit = 5, gbit = 6;
  else if(iDpyMode == EColor4K)
    rbit = gbit = 4;
  else
    User::Panic(_L("UNKNOWN DPY"), 1);

  int rmask = (1<<rbit)-1;
  int gmask = (1<<gbit)-1;
  int roff  = rbit + gbit;
  int goff  = rbit;

  int r, g, b;

  if(idx == -1)		// White
    {
      r = b = rmask;
      g = gmask;
    }
  else
    {
      r = elist[idx].iRed   >> (8-rbit);
      g = elist[idx].iGreen >> (8-gbit);
      b = elist[idx].iBlue  >> (8-rbit);
    }

  int w = iStoneSize.iWidth, h = iStoneSize.iHeight;
  int w3 = w-3, h3 = h-3;

  for(int i = 0; i < w; i++)
    {
      if(src)
        src->SetPos(TPoint(pt.iX, pt.iY+i));
      dest->SetPos(TPoint(0,i));

      for(int j = 0; j <= w; j++)
        {
	  int tr, tg, tb, transp = 0;

	  // Take care of the border
	  if((i < 2 || j < 2) && i+j < w)
	    {
	      tr = r+rbit; if(tr > rmask) tr = rmask;
	      tg = g+gbit; if(tg > gmask) tg = gmask;
	      tb = b+rbit; if(tb > rmask) tb = rmask;
	    }
	  else if(i > h3 || j > w3)
	    {
	      tr = r-rbit; if(tr < 0) tr = 0;
	      tg = g-gbit; if(tg < 0) tg = 0;
	      tb = b-rbit; if(tb < 0) tb = 0;
	    }
	  else
	    {
	      transp = 1;
	      tr = r;
	      tg = g;
	      tb = b;
	    }

	  // Make it transparent
	  if(src && transp && iTransp)
	    {
	      int s = src->GetPixel();
	      int sr = (s>>roff) & rmask,
	          sg = (s>>goff) & gmask,
		  sb =  s        & rmask;
	      if(iTransp == 1)
	        {
		  if(sr < tr) tr = sr;
		  if(sg < tg) tg = sg;
		  if(sb < tb) tb = sb;
	        }
	      else
	        {
		  tr = sr;
		  tg = sg;
		  tb = sb;
		}
	    }
	  dest->SetPixel((tr<<roff)|(tg<<goff)|(tb));

	  dest->IncXPos();
	  if(src)
	    src->IncXPos();
	}
    }

  if(src)
    {
      src->End();
      delete src;
    }
  dest->End();
  delete dest;

  CWindowGc &gc = SystemGc();
  gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
  gc.DrawBitmap(TRect(pt+iOff, iStoneSize), iStone[stidx]);

  if(content != ' ')
    {
      TBuf<1> buf;
      buf.Append(content);
      int w = iFcnt->TextWidthInPixels(buf);
      int h = iFcnt->HeightInPixels();
      int o = iFcnt->AscentInPixels();

      pt.iX += (iStoneSize.iWidth - w)/2 + 1;
      pt.iY += (iStoneSize.iHeight - h)/2;

      gc.UseFont(iFcnt);
      gc.SetPenColor(KRgbWhite);
      gc.SetBrushStyle(CGraphicsContext::ENullBrush);
      gc.DrawText(buf, TRect(pt+iOff,TSize(w,h)), o);

    }
    
}


void
CMutPlay::DrawFloatingElement(const Player *p, int draw, int activate) const
{
  if(p != iPl + iFgIdx)
    return;

  if(activate)
    ActivateGc();
  CWindowGc &gc = SystemGc();
  int i, stidx;

  TPoint pt;

  for(i = 0, stidx = 0; i < p->iElSize.iHeight; i++)
    {
      pt.iX = p->iElPos.iX * iStoneSize.iWidth;
      pt.iY = (p->iElPos.iY+i) * iStoneSize.iHeight;

      for(int j = 0; j < p->iElSize.iWidth; j++, pt.iX += iStoneSize.iWidth)
	if(p->iEl[i][j] != ' ')
	  {
	    if(draw)
	      DrawStonePiece(pt, p->iElIdx, ' ', stidx++, 1);
	    else
	      gc.DrawBitmap(TRect(pt+iOff,iStoneSize),iBg,TRect(pt,iStoneSize));
	  }
    }
  iEikonEnv->WsSession().Flush();
  if(activate)
    {
      SetInfo(p);
      DeactivateGc();
    }
}


////////////////////////////////////////////////////////
// (Re)draw the whole field.
//
void
CMutPlay::Draw(const TRect &aRect) const
{
  if(!iReadyToDraw)
    return;

  const Player *p = iPl + iFgIdx;
  CWindowGc &gc = SystemGc();
  gc.SetClippingRect(aRect);

  gc.SetBrushColor(KRgbBlack);
  gc.Clear();
  if(iBg)
    {
      gc.DrawBitmap(TRect(iOff,iPlaySize), iBg);
    }
  else 
    {
      gc.SetBrushColor(KRgbWhite);
      gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
      gc.DrawRect(TRect(iOff, iPlaySize));
    }
  
  TPoint pt(0,0);
  int stidx = 0;
  for(int i = 0; i < PLAY_HEIGHT; i++)
    {
      for(int j = 0; j < PLAY_WIDTH; j++, pt.iX += iStoneSize.iWidth)
	if(p->iField[i][j] != ' ')
	  {
	    DrawStonePiece(pt, p->iField[i][j], p->iChars[i][j], stidx++, 1);
	    if(stidx == STONEQUEUE)
	      {
		stidx = 0;
	        iEikonEnv->WsSession().Flush();
	      }
	  }
      pt.iY += iStoneSize.iHeight;
      pt.iX = 0;
    }
  iEikonEnv->WsSession().Flush();
  if(p->iElIdx != -1)
    DrawFloatingElement(p, 1, 0);
  SetInfo(p);
}



////////////////////////////////////////////////////////
// Keyboard input handling
//
TKeyResponse
CMutPlay::OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType)
{
  int i, c = aKeyEvent.iCode;

#ifdef _P800
  if(c == EQuartzKeyConfirm)    c = EKeyEnter;
  if(c == EQuartzKeyTwoWayUp)   c = EKeyRightArrow;
  if(c == EQuartzKeyTwoWayDown) c = EKeyLeftArrow;
#endif
#ifdef _SERIES60
  if(c == EKeyOK) c = EKeyEnter;
#endif


  if(iPl[0].iDizzyMove && c >= EKeyLeftArrow && c <= EKeyDownArrow)
    c = EKeyDownArrow;


  switch(c)
    {
      case EKeyLeftArrow:
        DoMove(iPl, -1, 0);
	break;

      case EKeyRightArrow:
        DoMove(iPl, 1, 0);
	break;

      case EKeyDownArrow:	// Faller
	Cancel();
        DoMove(iPl, 0, PLAY_HEIGHT);
	if(iPl[0].iDizzyMove) 
	  {
	    i = ElementAtBottom(iPl, 1, iPl[0].iDizzyFrom);
	    iPl[0].iDizzyMove--;
	  }
	else
	  i = ElementAtBottom(iPl, 1, 0);
	  
	if(!i)
	  return EKeyWasConsumed;

	Start();
	break;

      case EKeyUpArrow:		// Rotate left or right
      case EKeyEnter:
	i = (c == EKeyEnter);
        Rotate(iPl, i);
	c = IsElementFloating(iPl, TPoint(0,0));
	Rotate(iPl, !i);
	if(c)	// If its possible to rotate, then do it.
	  {
	    ActivateGc();
	    DrawFloatingElement(iPl, 0, 0);
            Rotate(iPl, i);
	    if(iPl[0].iElPos.iY < 0)
	      iPl[0].iElPos.iY = 0;
	    DrawFloatingElement(iPl, 1, 0);
	    DeactivateGc();
	  }
	break;
    }
  return EKeyWasConsumed;
}

void
CMutPlay::Start()
{
  Cancel();
  iTimer->After(level[iPl[0].iLevel-1].iTimeout);
  iTimerRunning = 1;
}

void
CMutPlay::Cancel()
{
  if(iTimerRunning)
    iTimer->Cancel();
  iTimerRunning = 0;
}

////////////////////////////////////////////////////////
// Initialize the playfield
//
void
CMutPlay::Reset(int idx)
{
  int from, to;
  if(idx == -1)
    {
      from = 0;
      to = iNCmp;
    }
  else
    {
      from = to = idx;
    }

  for(idx = from; idx <= to; idx++)
    {
      iPl[idx].iIdx = idx;
      Reset(iPl + idx);
    }

  iFgIdx = -1;		// To set the postition of the Small Screens
  SwitchToNextScreen();
}

void
CMutPlay::Reset(Player *p)
{
  for(int i = 0; i < PLAY_HEIGHT; i++)
    for(int j = 0; j < PLAY_WIDTH; j++)
      p->iField[i][j] = p->iChars[i][j] = ' ';
  p->iLevel = 1;
  p->iScore = 0;

  GenerateNewElement(p);
  GenerateNewElement(p);
  if(p->iIdx > 0)
    ComputePath(iCmp + p->iIdx - 1, p);

  p->iDizzyMove = 0;
  p->iDizzyFrom = 0;
  if(p->iIdx == 0)
    {
      p->iName.SetLength(PLAY_WIDTH);
      int l = iName.Length();
      for(int i = 0; i < PLAY_WIDTH; i++)
        p->iName[i] = iName[i%l];
    }
  else
    {
      TBuf<3> buf;
      buf.Copy(_L("C  "));
      buf[2] = p->iIdx+'0';
      p->iName.SetLength(PLAY_WIDTH);

      for(int i = 0; i < PLAY_WIDTH; i++)
        p->iName[i] = buf[i%3];
    }
}


////////////////////////////////////////////////////////
// Throw the dice and build a new element
//
int
CMutPlay::GenerateNewElement(struct Player *p)
{
  int i, j;

  p->iElIdx = p->iNxtIdx;
  p->iElNRot = elist[p->iElIdx].iNRot;

  p->iNxtIdx = Math::Rand(iRandSeed) % NELS;

  int w = elist[p->iElIdx].iWidth;
  int h = elist[p->iElIdx].iHeight;

  p->iElSize.iWidth  = w;
  p->iElSize.iHeight = h;

  for(i = 0; i < h; i++)
    for(j = 0; j < w; j++)
      p->iEl[i][j] = elist[p->iElIdx].iForm[i*w + j];

  i = Math::Rand(iRandSeed) % 4;
  while(i--)
    Rotate(p, 1);
  p->iElPos.iX = (PLAY_WIDTH - p->iElSize.iWidth)/2,
  p->iElPos.iY = 0;  

  if(!IsElementFloating(p, TPoint(0,0)))
    {
      if(p->iIdx == 0)
        {
	  Cancel();
	  ReportEventL(MCoeControlObserver::EEventInteractionRefused);
	  return 0;
        }
      else
        {
	  Reset(p);
	  if(p->iIdx == iFgIdx)
	    DrawNow();
	}
    }
  iLinesFallen = 0;
  return 1;
}


////////////////////////////////////////////////////////
// Rotate the element by 90 grade to the left or the right
//
void
CMutPlay::Rotate(Player *p, int toright)
{
  char lbuf[4][4];
  int i, j, w, h;

  w = p->iElSize.iWidth;
  h = p->iElSize.iHeight;

  for(i = 0; i < h; i++)
    for(j = 0; j < w; j++)
      if(toright)
        lbuf[j][h-i-1] = p->iEl[i][j];
      else
        lbuf[w-j-1][i] = p->iEl[i][j];

  for(i = 0; i < 4; i++)
    for(j = 0; j < 4; j++)
      p->iEl[i][j] = lbuf[i][j];


  // Recalibrate size & Position
  i = p->iElSize.iWidth;
  j = p->iElSize.iHeight;
  p->iElSize.iWidth = j;
  p->iElSize.iHeight= i;
  i -= j;
  if(i != 1 && i != -1)
    i /= 2;
  p->iElPos.iX += i;
  p->iElPos.iY -= i;
}


////////////////////////////////////////////////////////
// Move the element into one of the directions until it
// hits a wall or another element. Return 1 if it hit something
//
int
CMutPlay::DoMove(Player *p, int xdist, int ydist)
{
  int x = (xdist == 0 ? 0 : (xdist > 0 ? 1 : -1));
  int y = (ydist == 0 ? 0 : (ydist > 0 ? 1 : -1));

  if(p == iPl + iFgIdx)
    ActivateGc();
  while(xdist || ydist)
    {
      if(!IsElementFloating(p, TPoint(x, y)))
        break;

      DrawFloatingElement(p, 0, 0);
      p->iElPos.iX += x; p->iElPos.iY += y;
      DrawFloatingElement(p, 1, 0);

      xdist -= x; ydist -= y;
    }
  if(p == iPl + iFgIdx)
    DeactivateGc();
  return (xdist != 0 || ydist != 0);
}


////////////////////////////////////////////////////////
// If element would be moved by off, would it touch something
//
int
CMutPlay::IsElementFloating(const Player * p, TPoint off) const
{
  int x = p->iElPos.iX + off.iX;
  int y = p->iElPos.iY + off.iY;
  int i, j;

  if(x < 0 || x + p->iElSize.iWidth  > PLAY_WIDTH
           || y + p->iElSize.iHeight > PLAY_HEIGHT)
    return 0;

  if(y < 0)
    y = 0;
  for(i = 0; i < p->iElSize.iHeight; i++)
    for(j = 0; j < p->iElSize.iWidth; j++)
      if(p->iEl[i][j] != ' ' && p->iField[y+i][x+j] != ' ')
        return 0;
  return 1;
}



////////////////////////////////////////////////////
// Copy the floating element into the iField buffer.
// If there is a complete line, then do the flashing
//
void
CMutPlay::FloatingArrived(Player *p, Player *from)
{
  int i, j, linesfound = 0;

  // First copy the floating element to the field
  int ox = p->iElPos.iX;
  int oy = p->iElPos.iY;
  int k = 0, stidx = 0;
  if(from && p->iIdx == iFgIdx)
    ActivateGc();

  for(i = 0; i < p->iElSize.iHeight; i++)
    for(j = 0; j < p->iElSize.iWidth; j++)
      if(p->iEl[i][j] != ' ')
        {
	  int x = ox+j, y = oy+i;
	  p->iField[y][x] = p->iElIdx;
	  if(from)
	    {
	      p->iChars[y][x] = from->iName[k++];
	      if(p->iIdx == iFgIdx)
	        {
		  TPoint pt(x*iStoneSize.iWidth, y*iStoneSize.iHeight);
		  DrawStonePiece(pt,
		    p->iField[y][x], p->iChars[y][x], stidx++, 1);
		}
	    }
	}
  if(from && p->iIdx == iFgIdx)
    {
      iEikonEnv->WsSession().Flush();
      DeactivateGc();
    }


  // Look for complete lines and do some animation.
  // No removing happens here...
  char mark[PLAY_HEIGHT];
  for(i = PLAY_HEIGHT-1; i >= 0; i--)
    {
      mark[i] = 0;
      for(j = 0; j < PLAY_WIDTH; j++)
	if(p->iField[i][j] == ' ')
	  break;

      if(j < PLAY_WIDTH)
        continue;

      // Found a line
      mark[i] = 1;
      linesfound++;
      if(p != iPl+iFgIdx)
        continue;

      // Make it white.
      if(linesfound == 1)
	ActivateGc();

      TPoint pt(0, i*iStoneSize.iHeight);
      stidx = 0;
      for(j = 0; j < PLAY_WIDTH; j++)	// Redraw it
	{
	  DrawStonePiece(pt, -1, p->iChars[i][j], stidx++, 1); // Make it white
          pt.iX += iStoneSize.iWidth;
	  if(stidx == STONEQUEUE)
	    {
	      stidx = 0;
	      iEikonEnv->WsSession().Flush();
	    }
	}
    }

  if(!linesfound)
    return;

  if(p == iPl+iFgIdx)
    {
      iEikonEnv->WsSession().Flush();
      User::After(500000); // Wait a moment
    }

  // Now remove the complete lines
  for(i = PLAY_HEIGHT-1; i >= 0; i--)
    {
      if(!mark[i])
        continue;

      for(int i2 = i; i2 > 0; i2--)	// Scroll one line down
        for(j = 0; j < PLAY_WIDTH; j++)
	  {
	    p->iField[i2][j] = p->iField[i2-1][j];
	    p->iChars[i2][j] = p->iChars[i2-1][j];
	  }

      for(j = 0; j < PLAY_WIDTH; j++)	// and reset the topmost one
	{
	  p->iField[0][j] = ' ';
	  p->iChars[0][j] = ' ';
	}

      for(j = i; j > 0; j--)		// Do the same with the marked field
	mark[j] = mark[j-1];
      mark[0] = 0;

      i++;				// Check this line again
    }

  p->iElIdx = -1;			// Do not draw a floating element extra
  AddScore(p, linesfound);

  if(p == iPl+iFgIdx)
    {
      Draw(iScreenSize);		// Redraw everything
      DeactivateGc();
    }
}


////////////////////////////////////////////////////
//
int
CMutPlay::AddScore(Player *p, int linesfound)
{
  // Linesfound = 0 if the stone just fall....
  if(linesfound)
    p->iScore += level[p->iLevel-1].iRows[linesfound-1];
  else
    p->iScore += level[p->iLevel-1].iFaller;

  while(p->iScore >= 1000)
    {
      p->iLevel++;
      p->iScore -= 1000;
    }
  if(p->iLevel > NLEVELS)
    {
      if(p->iIdx == 0)
        {
	  Cancel();
	  ReportEventL(MCoeControlObserver::EEventInteractionRefused);
          return 0;
        }
      else
        Reset(p);
    }


  if(linesfound == 2)
    {
      for(int i = 0; i <= iNCmp; i++)
        if(p->iIdx != i)
	  AddRandomLine(iPl+i, p);
    }
  else if(linesfound == 3)
    {
      for(int i = 0; i <= iNCmp; i++)
        if(p->iIdx != i)
	  {
	    DoMove(iPl+i, 0, PLAY_HEIGHT);
	    ElementAtBottom(iPl+i, 1, p);
	  }
    }
  else if(linesfound == 4)
    {
      for(int i = 0; i <= iNCmp; i++)
        if(p->iIdx != i)
	  {
	    iPl[i].iDizzyMove += 2;
	    iPl[i].iDizzyFrom = p;
	  }
    }

  return 1;
}

////////////////////////////////////////////////////
// Set the status bar / Info
//
void
CMutPlay::SetInfo(const Player *p) const
{
  if(p != iPl+iFgIdx)
    return;
  
  CWindowGc &gc = SystemGc();
  TBuf<32> buf;
  int i, j, x, w, h, stidx;
  TRect box;

  gc.SetBrushColor(KRgbBlack);
  gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
  gc.SetPenColor(KRgbWhite);

  h = iFtxt->HeightInPixels();
  x = iPlaySize.iWidth+iOff.iX+h/4;
  h += h/4;

  int offtxt = iFtxt->AscentInPixels();
  int offnum = iFnum->AscentInPixels()+2;

  // Who
  if(p->iIdx == 0)
    {
      iEikonEnv->ReadResource(buf, R_YOU);
    }
  else
    {
      buf.Copy(_L("C  "));
      buf[2] = p->iIdx+'0';
    }
  w = iFtxt->TextWidthInPixels(buf);
  box.SetRect(TPoint(x, 2), TSize(w,h));
  gc.UseFont(iFtxt);
  gc.DrawText(buf, box, offtxt);


  // Level
  iEikonEnv->ReadResource(buf, R_LEVEL);
  w = iFtxt->TextWidthInPixels(buf);
  box.iTl.iY += h;
  box.SetSize(TSize(w,h));
  gc.UseFont(iFtxt);
  gc.DrawText(buf, box, offtxt);

  buf.Num(p->iLevel);
  box.iTl.iX += w;
  w = iFtxt->TextWidthInPixels(buf);
  box.SetSize(TSize(w,h));
  gc.UseFont(iFnum);
  gc.DrawText(buf, box, offnum);


  // Score
  iEikonEnv->ReadResource(buf, R_SCORE);
  w = iFtxt->TextWidthInPixels(buf);
  box.iTl.iX = x;
  box.iTl.iY += h;
  box.SetSize(TSize(w,h));
  gc.UseFont(iFtxt);
  gc.DrawText(buf, box, offtxt);

  buf.Format(_L("%03d"), p->iScore);
  w = iFtxt->TextWidthInPixels(buf);
  box.iTl.iY += h - h/4;
  box.SetSize(TSize(w+2,h));
  gc.UseFont(iFnum);
  gc.DrawText(buf, box, offnum);


  // Opponents
  box.iTl.iY += 3*h/2;
  for(i = 0; i <= iNCmp; i++)
    {
      if(i == iFgIdx)
	continue;
      DrawBgPlayer(i, 0);
    }

  // Next
  iEikonEnv->ReadResource(buf, R_NEXT);
  box.iTl.iY += PLAY_HEIGHT + 2 + h/2;
  w = iFtxt->TextWidthInPixels(buf);
  box.SetSize(TSize(w,h));
  gc.UseFont(iFtxt);
  gc.DrawText(buf, box, offtxt);

  gc.SetPenColor(KRgbBlack);
  TPoint pt(box.iTl);
  pt.iY += h;
  gc.DrawRect(TRect(TPoint(pt.iX-2,pt.iY-2),
  		TSize(2*iStoneSize.iWidth+4, 4*iStoneSize.iHeight+4)));

  TPoint tpt;
  int nidx = p->iNxtIdx;
  for(i = stidx = 0; i < elist[nidx].iWidth; i++)
    {
      tpt.iX = pt.iX + iStoneSize.iWidth;
      tpt.iY = pt.iY + i*iStoneSize.iHeight;
      for(j = 0; j < elist[nidx].iHeight; j++, tpt.iX -= iStoneSize.iWidth)
	if(elist[nidx].iForm[j*elist[nidx].iWidth+i] != ' ')
	  DrawStonePiece(tpt, nidx, ' ', stidx++, 0);
    }
  iEikonEnv->WsSession().Flush();

}


// Remove the next stone from the display, if its appropriate
void
CMutPlay::CheckFaller(const Player *p)
{
  if(++iLinesFallen >= level[p->iLevel-1].iNxtTimeout &&
    p->iIdx == iFgIdx)
    {
      ActivateGc();
      CWindowGc &gc = SystemGc();
      gc.SetPenColor(KRgbBlack);
      gc.SetBrushColor(KRgbBlack);
      gc.SetBrushStyle(CGraphicsContext::ESolidBrush);

      int h = iFtxt->HeightInPixels();
      h += h/4;

      TPoint pt(iPlaySize.iWidth+iOff.iX+h/4, 6*h+h/2);
      gc.DrawRect(TRect(TPoint(pt.iX-2,pt.iY-2),
  		TSize(2*iStoneSize.iWidth+4, 4*iStoneSize.iHeight+4)));

      DeactivateGc();
    }
}

void
CMutPlay::DoCompMove(int idx)
{
  Player *p = iPl+idx;
  Comp *c = iCmp+idx-1;

  if(p->iDizzyMove) // let it fall
    c->iCol = p->iElPos.iX;

  if(c->iRot)
    {
      int i = (c->iRot > 0) ? 1 : 0;
      Rotate(p, i);
      int j = IsElementFloating(p, TPoint(0,0));
      Rotate(p, !i);

      if(j)	// If its possible to rotate, then do it.
	{
	  if(idx == iFgIdx)
	    {
	      ActivateGc();
	      DrawFloatingElement(p, 0, 0);
	    }
	  Rotate(p, i);
	  if(p->iElPos.iY < 0)
	    p->iElPos.iY = 0;

	  if(idx == iFgIdx)
	    {
	      DrawFloatingElement(p, 1, 0);
	      DeactivateGc();
	    }
          if(c->iRot > 0)
	    c->iRot--;
	  else
	    c->iRot++;
	}
    }

  int ret;
  if(c->iCol == p->iElPos.iX && !c->iRot)
    {
      ret = DoMove(p, 0, PLAY_HEIGHT);
    }
  else
    {
      ret = DoMove(p, (c->iCol > p->iElPos.iX) ? 1 : -1, 1);
      if(ret)
	DoMove(p, 0, PLAY_HEIGHT);
    }

  if(ret)
    {
      if(p->iDizzyMove)
        {
          ElementAtBottom(p, 1, p->iDizzyFrom);
	  p->iDizzyMove--;
	}
      else
        ElementAtBottom(p, 1, 0);

    }
}

void
CMutPlay::DrawBgPlayer(int idx, int activate) const
{
  const Player *p = iPl + idx;
  TBitmapUtil* dest = new TBitmapUtil(iSs);

  int rbit = 0, gbit = 0;
  if(iDpyMode == EColor64K)
    rbit = 5, gbit = 6;
  else if(iDpyMode == EColor4K)
    rbit = gbit = 4;
  int rmask = (1<<rbit)-1;
  int gmask = (1<<gbit)-1;
  int roff  = rbit + gbit;
  int goff  = rbit;

  int r, g, b;

  int w = PLAY_WIDTH+1, h = PLAY_HEIGHT+1;

  dest->Begin(TPoint(0,0));
  for(int j = 0; j <= h; j++)
    {
      dest->SetPos(TPoint(0, j));

      for(int i = 0; i <= w; i++)
        {
	  if(i == 0 || j == 0 || i == w || j == h)
	    {
	      r = b = rmask; // White
	      g = gmask;
	    }
	  else 
	    {
	      int i2 = i-1, j2 = j-1;
	      int k = p->iField[j2][i2];

	      // Look for the floating element
	      if(k == ' ')
	        {
		  if(i2 >= p->iElPos.iX &&
		     i2 < p->iElPos.iX+p->iElSize.iWidth &&
		     j2 >= p->iElPos.iY &&
		     j2 < p->iElPos.iY+p->iElSize.iHeight)
		    {
		      if(p->iEl[j2 - p->iElPos.iY][i2 - p->iElPos.iX] != ' ')
		        k = p->iElIdx;
		    }
		}

	      if(k == ' ')
	        {
	          r = g = b = 7;
		}
	      else
	        {
		  r = elist[k].iRed   >> (8-rbit);
		  g = elist[k].iGreen >> (8-gbit);
		  b = elist[k].iBlue  >> (8-rbit);
	        }
	    }
	  dest->SetPixel((r<<roff)|(g<<goff)|(b));
	  dest->IncXPos();
	}
    }
  dest->End();
  delete dest;

  if(activate)
    ActivateGc();
  CWindowGc &gc = SystemGc();
  gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
  gc.DrawBitmap(TRect(p->iSsp, TSize(w+1,h+1)), iSs);
  iEikonEnv->WsSession().Flush();
  if(activate)
    DeactivateGc();
}

void
CMutPlay::SwitchToNextScreen()
{
  int i, j;

  iFgIdx = (iFgIdx+1) % (iNCmp+1);
  for(j = i = 0; i <= iNCmp; i++)
    {
      Player *p = iPl+i;
      if(i == iFgIdx)
        continue;
      p->iSsp.iX = 144 + j * (PLAY_WIDTH + 5);
      p->iSsp.iY = 100;
      j++;
    }
  DrawNow();
}

void
CMutPlay::AddRandomLine(Player *to, Player *from)
{
  // First scroll everything up
  int i, j;
  for(i = 0; i < PLAY_HEIGHT-1; i++)
    for(j = 0; j < PLAY_WIDTH; j++)
      {
	to->iField[i][j] = to->iField[i+1][j];
	to->iChars[i][j] = to->iChars[i+1][j];
      }
  
  // Now insert the Random line. We need at least one filled and one empty
  // field.
  int nfilled = 3; // This should be the avarage of holes
  int k = 0;
  for(j = 0; j < PLAY_WIDTH; j++)
    if(Math::Rand(iRandSeed) % PLAY_WIDTH >= nfilled)
      {
	to->iField[i][j] = Math::Rand(iRandSeed) % NELS;
	to->iChars[i][j] = from->iName[k++];
	nfilled++;
      }
    else
      {
	nfilled--;
	to->iField[i][j] = ' ';
	to->iChars[i][j] = ' ';
      }

  if(!IsElementFloating(to, TPoint(0, 0)))
    ElementAtBottom(to, 0, 0);
  if(to->iIdx == iFgIdx)
    DrawNow();
}

int
CMutPlay::ElementAtBottom(Player *p, int dropped, Player *from)
{
  if(dropped && !AddScore(p, 0))
    return 0;
  FloatingArrived(p, from);
  GenerateNewElement(p);
  if(p->iIdx)
    ComputePath(iCmp+(p->iIdx-1), p);
  DrawFloatingElement(p, 1, 1);
  return 1;
}
