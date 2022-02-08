#include "mut.h"
#include "mut.hrh"
#include "Mut.rsg"
#include "util.h"
#include <uikon.hrh>	//EEikCmdExit
#include <eikdll.h>
#include <eikmenup.h>
#include <string.h>	// memset
#include <stdlib.h>	// atoi

#ifdef _SERIES60
#include <akntitle.h>
#include <aknnotedialog.h>
#include <aknnavi.h>				// CAknNavigationContainer
#include <aknnavide.h> 				// CAknNavigationDecorator
#include <aknquerydialog.h>			// CAknTextQueryDialog
#include <eikfrlb.h>				// CEikFormattedCellListBox
#endif


const char * const configname[] = {
  "HS_1_Name", "HS_1_Level", "HS_1_Score", 
  "HS_2_Name", "HS_2_Level", "HS_2_Score", 
  "HS_3_Name", "HS_3_Level", "HS_3_Score", 
  "HS_4_Name", "HS_4_Level", "HS_4_Score", 
  "HS_5_Name", "HS_5_Level", "HS_5_Score", 
  "BG1_Name",
  "BG2_Name",
  "BG2_Transp",
  "Computer_Opponents",
  "Username",
};
#define NSCORE 5

#define NCONFIG (sizeof(configname)/sizeof(*configname))
#define LCFG(a,b)  iApp->iCnv->FromUtf8(iApp->GetConfig(a,b))
#define LCFGI(a,b)  iApp->iCnv->FromUtf8(iApp->GetIdxConfig(a,b))

EXPORT_C CApaApplication *
NewApplication()
{
  return new CMutApplication;
}

GLDEF_C int
E32Dll(TDllReason)
{
  return KErrNone;
}


//////////////////////////////////////////////////////
//
// The AppUi class, holding the game data, initiating
// menu actions
//
//////////////////////////////////////////////////////
void
CMutAppUi::ConstructL()
{
  BaseConstructL();

  iCnv = new Cnv;
  iPgmDir.Copy(Application()->AppFullName());
  iPgmDir.SetLength(iPgmDir.LocateReverse('\\')+1);
  iConfigList = 0;
  iConfigName.Copy(iPgmDir);
  iConfigName.Append(_L("config.txt"));
  ReadConfig();

  iIsPlaying = 0;

  iAppView = new CMutAppView();
  iAppView->ConstructL(ClientRect(), this);
  AddToStackL(iAppView);

#ifdef _SERIES60
  CEikStatusPane* sp = iEikonEnv->AppUiFactory()->StatusPane();
  CAknTitlePane *tp =
          (CAknTitlePane *)sp->ControlL(TUid::Uid(EEikStatusPaneUidTitle));
  tp->SetTextL(_L("MultiUserTetris"));
#endif
}


////////////////////////////////////////////////////////////////
// Configfile stuff
////////////////////////////////////////////////////////////////
void
CMutAppUi::ReadConfig()
{
  // Reset the value list
  if(iConfigList)
    {
      for(unsigned int i = 0; i < NCONFIG; i++)
	if(iConfigList[i])
	  free(iConfigList[i]);
      free(iConfigList);
    }
  iConfigList = (char **)malloc(sizeof(char *) * NCONFIG);
  for(unsigned int i = 0; i < NCONFIG; i++)
    iConfigList[i] = 0;


  // Read in the File
  RFile *r = new RFile;
  int ret = r->Open(iCoeEnv->FsSession(), iConfigName,
	EFileShareAny|EFileRead|EFileStream);
  if(ret != KErrNone)
    return;

    
  TBuf8<2048> buf;
  if(r->Read(buf) != KErrNone)
    return;

  char *p, *n, *v;
  for(p = (char *)buf.PtrZ(); *p; p = n)
    {
      n = strchr(p, '\n');
      if(!n)
        break;
      *n++ = 0;

      v = strchr(p, '=');
      if(!v)
        continue;
      *v++ = 0;

      for(unsigned int i = 0; i < NCONFIG; i++)
        if(!strcmp(p, configname[i]))
	  {
	    iConfigList[i] = strdup(v);
	    break;
	  }
    }

  r->Close();
  delete r;
}


void
CMutAppUi::WriteConfig()
{
  RFile *iW = new RFile;
  iW->Open(iCoeEnv->FsSession(), iConfigName,
  	EFileShareExclusive|EFileWrite|EFileStream);

  for(unsigned int i = 0; i < NCONFIG; i++)
    if(iConfigList[i])
      {
	TBuf8<256> buf;
	buf.Format(_L8("%s=%s\n"), configname[i], iConfigList[i]);
        iW->Write(buf);
      }

  iW->Close();
  delete iW;
}

const char *
CMutAppUi::GetConfig(const char *name, const char *def)
{
  for(unsigned int i = 0; i < NCONFIG; i++)
    if(!strcmp(name, configname[i]))
      {
        if(iConfigList[i])
          return iConfigList[i];
	else
	  return def;
      }
  return "NoSuchConfig";
}

void
CMutAppUi::SetConfig(const char *name, const char *value)
{
  for(unsigned int i = 0; i < NCONFIG; i++)
    if(!strcmp(name, configname[i]))
      {
        if(iConfigList[i])
	  free(iConfigList[i]);
	iConfigList[i] = strdup(value);
	return;
      }
}

void
CMutAppUi::SetConfig(const char *name, int value)
{
  TBuf8<16> nbuf;
  nbuf.Format(_L8("%d"), value);
  SetConfig(name, (const char *)nbuf.PtrZ());
}

int
CMutAppUi::GetIdxConfigInt(const char *name, int idx)
{
  char b[16];
  strcpy(b, name);
  b[3] = idx+'0';
  return atoi(GetConfig(b, "0"));
}

const char *
CMutAppUi::GetIdxConfig(const char *name, int idx)
{
  char b[16];
  strcpy(b, name);
  b[3] = idx+'0';
  return GetConfig(b, "NONE");
}

void
CMutAppUi::SetIdxConfig(const char *name, int idx, int value)
{
  char b[16];
  strcpy(b, name);
  b[3] = idx+'0';
  SetConfig(b, value);
}

void
CMutAppUi::SetIdxConfig(const char *name, int idx, const char *value)
{
  char b[16];
  strcpy(b, name);
  b[3] = idx+'0';
  SetConfig(b, value);
}



////////////////////////////////////////////////////

class CPictureDialog : public CAknListQueryDialog
{
public:
  CPictureDialog(int *aIdx, TFileName &aBuf) 
  	: CAknListQueryDialog(aIdx)
  {
    iBufPtr = &aBuf;
    iIdx = aIdx;
  }

  ~CPictureDialog() 
  {
    if(*iIdx >= 0 && *iIdx < iArray->Count())
      {
        iBufPtr->Copy(_L("C:\\Nokia\\Images\\"));
        iBufPtr->Append(iArray->MdcaPoint(*iIdx));
      }
    delete iArray;
  }

  void
  PreLayoutDynInitL()
  {
    CAknListQueryDialog::PreLayoutDynInitL();
    CAknListQueryControl *lc =(CAknListQueryControl*)Control(EListQueryControl);
    CEikFormattedCellListBox *lb = (CEikFormattedCellListBox *)lc->Listbox();

    iArray = new(ELeave)CDesCArrayFlat(1);

    CDir *cdir = 0;
    if(iCoeEnv->FsSession().GetDir(_L("C:\\Nokia\\Images\\*.jpg"),
			 KEntryAttMatchMask, ESortByName, cdir) != KErrNone)
      return;
    for(int i = 0; i < cdir->Count(); i++)
      iArray->AppendL((*cdir)[i].iName);
    delete cdir;

    lb->Model()->SetItemTextArray(iArray);
    lb->Model()->SetOwnershipType(ELbmDoesNotOwnItemArray);
  }

private:
  CDesCArray *iArray;
  int *iIdx;
  TFileName *iBufPtr;
};


////////////////////////////////////////////////////

void
CMutAppUi::HandleCommandL(int aCommand)
{
  switch(aCommand)
    {
      case EMutPlay: // New Game
        iAppView->SetPlayMode(1, 1);
	break;

      case EMutNextScreen:
	{
	  iAppView->iPlay->SwitchToNextScreen();
	}
	break;

      case EMutTerminate:
        iHaveGame = 0;
        iAppView->SetPlayMode(0, 1);
	break;

      case EMutContinue:
        if(!iIsPlaying)
          iAppView->SetPlayMode(1, 0);
        break;

      case EMutPause:
        iAppView->SetPlayMode(0, 0);
        break;

      case EOptTransparent:
	{
	  CAknListQueryDialog* dlg =
	  	new(ELeave) CAknListQueryDialog(&iAppView->iPlay->iTransp);
	  dlg->ExecuteLD(R_DLG_TRANSPARENT);
	  SetConfig("BG2_Transp", iAppView->iPlay->iTransp);
          WriteConfig();
	}
        break;
      case EOptBg1:
      case EOptBg2:
	{
          TFileName fname;
	  int idx;
	  CPictureDialog* dlg = new(ELeave) CPictureDialog(&idx, fname);
	  if(dlg->ExecuteLD(R_DLG_PICTURE) != 0)
	    {
              CFbsBitmap *pic;
	      TDisplayMode dm = iCoeEnv->ScreenDevice()->DisplayMode();
	      if(aCommand == EOptBg1)
	        {
                  if((pic = LoadJpgImage(fname, iAppView->Size(), dm)))
		    {
		      if(iAppView->iBg)
			delete iAppView->iBg;
		      iAppView->iBg = pic;
		      iAppView->DrawNow();
                      SetConfig("BG1_Name", iCnv->ToUtf8(fname));
		    }
		}
	      else
	        {
                  if((pic = LoadJpgImage(fname, iAppView->iPlay->iPlaySize,dm)))
		    {
		      if(iAppView->iPlay->iBg)
		        delete iAppView->iPlay->iBg;
		      iAppView->iPlay->iBg = pic;
                      SetConfig("BG2_Name", iCnv->ToUtf8(fname));
		    }
		}

	      if(pic)
                WriteConfig();
	    }
	}
        break;
      case EOptUsername:
        {
	  TBuf<32> buf;
	  buf.Copy(iCnv->FromUtf8(GetConfig("Username", "Player")));
	  CAknTextQueryDialog* dlg = CAknTextQueryDialog::NewL(buf);
	  if(!dlg->ExecuteLD(R_DLG_NAMEREQUESTER))
	    return;
          SetConfig("Username", iCnv->ToUtf8(buf));
	  iAppView->iPlay->iName.Copy(buf);
          WriteConfig();
	}

        break;

      case EMultiComputer:
	{
          int ncomp = atoi(GetConfig("Computer_Opponents", "0"));
	  CAknListQueryDialog* dlg = new(ELeave) CAknListQueryDialog(&ncomp);
	  dlg->ExecuteLD(R_DLG_COMPUTER);

	  iAppView->iPlay->iNCmp = ncomp;
	  iAppView->iPlay->Reset(-1);
	  SetConfig("Computer_Opponents", ncomp);
          WriteConfig();
	}
        break;

      case EMutAbout:
#ifdef _SERIES60 // The other one is ugly, leaves place for an empty image
        {
	  CAknNoteDialog* dlg = new CAknNoteDialog();
	  dlg->ExecuteLD(R_ABOUT);
	}
#else
	iEikonEnv->InfoWinL(R_ABOUT1, R_ABOUT2);
#endif
        break;

      case EAknSoftkeyExit:
      case EEikCmdExit:
	if(iIsPlaying)
          iAppView->SetPlayMode(0, 0);
	else
	  {
            iAppView->iPlay->Cancel();	// Else the exit panics :-/
            Exit();
	  }
	break;
    }
}

void
CMutAppUi::DynInitMenuPaneL(int aMenuId, CEikMenuPane* aMenuPane)
{  
  if(aMenuId == R_MULTI_MENU)
    {
      aMenuPane->SetItemDimmed(EMultiAddServer, iIsConnected);
      aMenuPane->SetItemDimmed(EMultiAddClient, iIsConnected);
      aMenuPane->SetItemDimmed(EMultiDelHost,   !iIsConnected);
      aMenuPane->SetItemDimmed(EMultiComputer,  iIsConnected);
    }
  if(aMenuId == R_MUT_MENU)
    {
      aMenuPane->SetItemDimmed(EMutPlay,     iHaveGame);
      aMenuPane->SetItemDimmed(EMutContinue, !iHaveGame);
      aMenuPane->SetItemDimmed(EMutTerminate,!iHaveGame);
      aMenuPane->SetItemDimmed(EMutNextScreen,
      			!(iIsPlaying && iAppView->iPlay->iNCmp > 0));
      aMenuPane->SetItemDimmed(EMutPause,    !iIsPlaying);
      aMenuPane->SetItemDimmed(EMutMulti,    iHaveGame);
      aMenuPane->SetItemDimmed(EMutOptions,  iIsPlaying);
      aMenuPane->SetItemDimmed(EMutAbout,    iIsPlaying);
      aMenuPane->SetItemDimmed(EEikCmdExit,  iIsPlaying);
    }
}

void
CMutAppUi::HandleForegroundEventL(TBool aForeground)
{
  if(!aForeground)
    iAppView->SetPlayMode(0, 0);
}

void
CMutAppUi::EndOfPlay(int level, int score)
{
  /////////////////////////////////////
  // Display it anyway on the title bar
  TBuf<32> buf, buf2;
  iEikonEnv->ReadResource(buf, R_LAST);
  buf2.Format(buf, level, score);
  iAppView->PrintMessage(buf2);

  ///////////////////////////////////
  // Check the score where it belongs
  int i;
  for(i = 1; i <= NSCORE; i++)
    {
      int l = GetIdxConfigInt("HS_?_Level", i);
      int s = GetIdxConfigInt("HS_?_Score", i);
      if(level > l || (level == l && score > s))
        break;
    }


  ///////////////////////////////
  // Insert/Save it if it made it
  if(i < NSCORE+1)
    {
      buf.Copy(iCnv->FromUtf8(GetConfig("Username", "Player")));
      CAknTextQueryDialog* dlg = CAknTextQueryDialog::NewL(buf);
      if(!dlg->ExecuteLD(R_DLG_NAMEREQUESTER))
	return;
      iAppView->iPlay->iName.Copy(buf);

      for(int j = NSCORE-1; j >= i; j--)
        {
	  SetIdxConfig("HS_?_Name",  j+1, GetIdxConfig("HS_?_Name", j));
	  SetIdxConfig("HS_?_Level", j+1, GetIdxConfigInt("HS_?_Level", j));
	  SetIdxConfig("HS_?_Score", j+1, GetIdxConfigInt("HS_?_Score", j));
	}
      SetIdxConfig("HS_?_Name",  i, iCnv->ToUtf8(buf));
      SetIdxConfig("HS_?_Level", i, level);
      SetIdxConfig("HS_?_Score", i, score);

      iAppView->DrawNow();

      SetConfig("Username", iCnv->ToUtf8(buf));
      WriteConfig();
    }

}


//////////////////////////////////////////////////////
//
// The AppView class, responsible for drawing the status
// data or starting/feeding the play class
//
//////////////////////////////////////////////////////
void
CMutAppView::ConstructL(const TRect& aRect, class CMutAppUi *app)
{
  iApp = app;

  CreateWindowL();


  TDisplayMode dm = iCoeEnv->ScreenDevice()->DisplayMode();
  TFileName f;
  const char *pic = iApp->GetConfig("BG1_Name", "bg1.jpg");
  if(pic[1] == ':') // absolute path
    {
      f.Copy(iApp->iCnv->FromUtf8(pic));
    }
  else 
    {
      f.Copy(iApp->iPgmDir);
      f.Append(iApp->iCnv->FromUtf8(pic));
    }
  iBg = LoadJpgImage(f, aRect.Size(), dm);


#ifdef _SERIES60
  CEikStatusPane* sp = iEikonEnv->AppUiFactory()->StatusPane();
  iNavi = (CAknNavigationControlContainer *)
                             sp->ControlL(TUid::Uid(EEikStatusPaneUidNavi));
#endif


  iPlay = new CMutPlay;
  iPlay->iReadyToDraw = 0;
  iPlay->ConstructL();
  pic = iApp->GetConfig("BG2_Name", "bg2.jpg");
  if(pic[1] == ':') // absolute path
    {
      f.Copy(iApp->iCnv->FromUtf8(pic));
    }
  else 
    {
      f.Copy(iApp->iPgmDir);
      f.Append(iApp->iCnv->FromUtf8(pic));
    }
  iPlay->iBg = LoadJpgImage(f, iPlay->iPlaySize, dm);
  iPlay->iTransp = atoi(iApp->GetConfig("BG2_Transp",         "1"));
  iPlay->iNCmp   = atoi(iApp->GetConfig("Computer_Opponents", "0"));
  iPlay->iName.Copy(iApp->iCnv->FromUtf8(iApp->GetConfig("Username","Player")));
  iPlay->SetObserver(this);
  iPlay->Reset(-1);
  iPlay->MakeVisible(EFalse);
  iPlay->iReadyToDraw = 1;

  TFontSpec fontSpec;
  fontSpec.iTypeface.iName = _L("LatinBold19");
  iEikonEnv->ScreenDevice()->GetNearestFontInTwips(iFbig, fontSpec);
  fontSpec.iTypeface.iName = _L("LatinBold13");
  iEikonEnv->ScreenDevice()->GetNearestFontInTwips(iFsmall, fontSpec);

  SetRect(aRect);
  ActivateL();
}

TKeyResponse
CMutAppView::OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType)
{
  if(aType != EEventKey)
    return EKeyWasConsumed;

  int c = aKeyEvent.iCode;

  if(iApp->iIsPlaying)
    {
#ifdef _SERIES60
      if(c == 63499 && iPlay->iNCmp > 0)	 // This is the "ABC" Key
	{
	  iApp->HandleCommandL(EMutNextScreen);
          return EKeyWasConsumed;
	}
      else
#endif
        return iPlay->OfferKeyEventL(aKeyEvent, aType);
    }


#ifdef _P800
  if(c == EQuartzKeyConfirm)    c = EKeyEnter;
#endif
#ifdef _SERIES60
  if(c == EKeyOK) c = EKeyEnter;
#endif
  if(c == EKeyEnter)
    iApp->HandleCommandL(iApp->iHaveGame ? EMutContinue : EMutPlay);

  return EKeyWasConsumed;
}

void
CMutAppView::DrawText(CFont *fnt, TPoint offset,
			const TDesC &txt, int left) const
{
  CWindowGc &gc = SystemGc();
  TRect box;
  box.SetRect(offset, TSize(fnt->TextWidthInPixels(txt),
				5*fnt->HeightInPixels()/4));
  if(!left)
    box.iTl.iX -= fnt->TextWidthInPixels(txt);

  box.Move(TPoint(1,1));
  gc.SetPenColor(iEikonEnv->ControlColor(EColorControlText,*this));
  gc.DrawText(txt, box, fnt->AscentInPixels());

  box.Move(TPoint(-1,-1));
  gc.SetPenColor(iEikonEnv->ControlColor(EColorControlBackground, *this));
  gc.DrawText(txt, box, fnt->AscentInPixels());
}


void
CMutAppView::Draw(const TRect &aRect) const
{
  if(iApp->iIsPlaying)
    iPlay->Draw(aRect);

  CWindowGc &gc = SystemGc();
  gc.SetClippingRect(aRect);

  if(iBg)
    gc.BitBlt(TPoint(0,0), iBg);
  else
    gc.Clear(aRect);

  TPoint pt(6,5);
  TBuf<32> buf;
  iEikonEnv->ReadResource(buf, R_HS_TITLE);
  gc.UseFont(iFbig);
  DrawText(iFbig, pt, buf, 1);
  pt.iY += 3*iFbig->HeightInPixels()/2;

  gc.UseFont(iFsmall);
  iEikonEnv->ReadResource(buf, R_HS_NAME);
  DrawText(iFsmall, pt, buf, 1);

  iEikonEnv->ReadResource(buf, R_HS_LEV);
  DrawText(iFsmall, TPoint(pt.iX+80, pt.iY), buf, 1);

  iEikonEnv->ReadResource(buf, R_HS_SCORE);
  DrawText(iFsmall, TPoint(pt.iX+162, pt.iY), buf, 0);
  pt.iY += 3*iFsmall->HeightInPixels()/2;

  for(int i = 1; i <= NSCORE; i++)
    {
      DrawText(iFsmall, pt, LCFGI("HS_?_Name", i), 1);
      DrawText(iFsmall, TPoint(pt.iX+ 80, pt.iY), LCFGI("HS_?_Level", i), 1);
      DrawText(iFsmall, TPoint(pt.iX+162, pt.iY), LCFGI("HS_?_Score", i), 0);

      pt.iY += 3*iFsmall->HeightInPixels()/2;
    }
}

void
CMutAppView::SetPlayMode(int on, int reset)
{
  if(reset)
    iPlay->Reset(0);
  if(on)
    iApp->iHaveGame = 1;
  iApp->iIsPlaying = on;
  iPlay->MakeVisible(on);
  if(on)
    {
      iPlay->Start();
      iPlay->DrawNow();
    }
  else
    {
      iPlay->Cancel();
      DrawNow();
    }
}

void
CMutAppView::HandleControlEventL(CCoeControl* aControl, TCoeEvent aEventType)
{
  if(aControl == iPlay && aEventType == EEventInteractionRefused)
    {
      iApp->iHaveGame = 0;
      SetPlayMode(0, 0);
      iApp->EndOfPlay(iPlay->iPl[0].iLevel, iPlay->iPl[0].iScore);
    }
}

void
CMutAppView::PrintMessage(TDesC &buf)
{
  if(iMsg)
    iNavi->Pop(iMsg);
  iMsg = iNavi->CreateMessageLabelL(buf);
  iNavi->PushL(*iMsg);
}
