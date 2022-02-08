#ifndef _MUT_H
#define _MUT_H

#include <eikappui.h>
#include <eikapp.h>
#include <eikdoc.h>
#include "play.h"

class CMutAppUi;

#ifdef _SERIES60
#include <aknapp.h>     
#include <aknappui.h>   
#define CEikAppUi CAknAppUi
#define CEikApplication CAknApplication
#endif

#ifdef _SERIES60
class CAknNavigationControlContainer;
class CAknNavigationDecorator;
#endif

class CMutPlay;
class CMutStatus;
class Cnv;


////////////////////////////////////////////////////////////
class CMutAppView :
	public CCoeControl,		// It will be a control
	public MCoeControlObserver
{
public:
  void ConstructL(const TRect& aRect, class CMutAppUi *app);

  void HandleControlEventL(CCoeControl* aControl, TCoeEvent aEventType);
  TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType);
  void Draw(const TRect &aRect) const;
  void SetPlayMode(int on, int reset);
  void PrintMessage(TDesC &msg);

  CMutPlay   *iPlay;
  CFbsBitmap *iBg;

private:
  void DrawText(CFont *fnt, TPoint offset, const TDesC &txt, int left) const;

  CMutAppUi *iApp;

  CFont      *iFbig, *iFsmall;

#ifdef _SERIES60
  CAknNavigationDecorator *iMsg;
  CAknNavigationControlContainer *iNavi;
#endif
};


////////////////////////////////////////////////////////////
class CMutAppUi : public CEikAppUi
{
public:
  void ConstructL();
  void HandleCommandL(int aCommand);
  void HandleForegroundEventL(TBool aForeground);
  void DynInitMenuPaneL(int aMenuId, CEikMenuPane* aMenuPane);
  void EndOfPlay(int level, int score);

  CMutAppView* iAppView;
  int iIsConnected;
  int iIsPlaying;
  int iHaveGame;
  TFileName iPgmDir;

  void ReadConfig();
  void WriteConfig();
  const char *GetConfig(const char *name, const char *def);
  const char* GetIdxConfig(const char *name, int idx);
  int   GetIdxConfigInt(const char *name, int idx);

  void  SetConfig(const char *name, const char *value);
  void  SetConfig(const char *name, int value);
  void  SetIdxConfig(const char *name, int idx, int value);
  void  SetIdxConfig(const char *name, int idx, const char *value);

  Cnv        *iCnv;

private:
  char **iConfigList;
  TFileName iConfigName;

};


////////////////////////////////////////////////////////////
class CMutDocument : public CEikDocument
{
public:
  CMutDocument(CEikApplication& aApp) : CEikDocument(aApp) { }
  CEikAppUi* CreateAppUiL() { return new(ELeave) CMutAppUi; }
};


////////////////////////////////////////////////////////////
class CMutApplication : public CEikApplication
{
public:
    CApaDocument* CreateDocumentL() { return new (ELeave)CMutDocument(*this); }
    TUid AppDllUid() const { TUid id = { UID3 };  return id; }
};


#endif
