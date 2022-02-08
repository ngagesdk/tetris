#include <f32file.h>
#include <libc/string.h>
#include <libc/unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <mdaimageconverter.h>
#include <bitdev.h>

#include "util.h"


Cnv::Cnv()
{
  iWP = new TPtr(0, 0, 0);
  iWideLength = 0;
  iWideBuffer = 0;
  iUtf8Length = 0;
  iUtf8Buffer = 0;
}

const TDesC &
Cnv::FromUtf8(const char *arg)
{
  int n = strlen(arg);
  if(n > iWideLength)
    {
      if(iWideBuffer)
        free(iWideBuffer);
      iWideBuffer = (CHAR*)malloc(sizeof(CHAR) * (n+1));
      iWideLength = n;
    }
  CHAR *wbp = iWideBuffer;
  for(n = 0; *arg; n++)
    {
      int w = 0;
      if((*arg & 0x80) == 0)		// 0xxx xxxx
	w = *arg++;
      else if((*arg & 0xe0) == 0xc0)	// 110y yyyy 10xx xxxx
	{
	  w = ((arg[0]&0x1f) << 6) | (arg[1]&0x3f);
	  arg += 2;
	}
      else if((*arg & 0xf0) == 0xe0)	// 1110 zzzz 10yy yyyy 10xx xxxx
	{
	  w = ((arg[0]&0xf) << 12) | ((arg[1]&0x3f) << 6) | (arg[2]&0x3f);
	  arg += 3;
	}
      *wbp++ = w;
    }
  iWP->Set(iWideBuffer, n, n);
  iWideBuffer[n] = 0;
  return *iWP;
}

#define CHUNK 128

const char *
Cnv::ToUtf8(const TDesC &x)
{
  int i, j, l = x.Length();
  for(j = i = 0; i < l; i++)
    {
      if(j+5 > iUtf8Length)
        {
	  iUtf8Length += CHUNK;
	  iUtf8Buffer = (char *)realloc(iUtf8Buffer, iUtf8Length);
	}

      int c = x[i];
      if(c <= 0x7f)
        {
	  iUtf8Buffer[j++] = c;
	}
      else if(c <= 0x7ff)
        {
	  iUtf8Buffer[j++] = 0xc0 | (c >>   6);	// first 5 bits
	  iUtf8Buffer[j++] = 0x80 | (c & 0x3f);	// last 6 bits
	}
      else if(c <= 0x7fff)
        {
	  iUtf8Buffer[j++] = 0xe0 | (c >>  12);		// first 5 bits
	  iUtf8Buffer[j++] = 0x80 | ((c>>6) & 0x3f);	// next 6 bits
	  iUtf8Buffer[j++] = 0x80 | (c & 0x3f);		// last 6 bits
	}
    }
  iUtf8Buffer[j] = 0;
  return iUtf8Buffer;
}


void
debugline(const char *a, ...)
{
  va_list alist;
  FILE *fp;

  if(!(fp = fopen("C:\\debug", "a")))
    return;

  {
    // Get the time
    TTime now;
    now.HomeTime();
    TBuf<64> tbuf;
    _LIT(KFmt, "%F%H:%T:%S.%C");
    now.FormatL(tbuf, KFmt);

    // Convert it to ascii
    char mbuf[64];
    int i;
    for(i = 0; i < tbuf.Length(); i++)
      mbuf[i] = tbuf[i];
    mbuf[i] = 0;

    fprintf(fp, "%s ", mbuf);
  }

  va_start(alist, a);
  vfprintf(fp, a, alist);
  va_end(alist);
  fputc('\n', fp);
  fclose(fp);
}

////////////////////////////////////////////////////
//
// Simple Image loader
//
////////////////////////////////////////////////////
class CImageLoadObserver : public CBase, 
		public MMdaImageUtilObserver
{
public:
  void
  MiuoCreateComplete(TInt aError)
  {
  }

  void
  MiuoOpenComplete(TInt aError)
  {
    if(aError != KErrNone)
      return;

    TFrameInfo fi;
    iLd->FrameInfo(0, fi);
    iImg = new (ELeave) CFbsBitmap();

    // Compute the right size....
    TSize sz = fi.iOverallSizeInPixels;
    if(sz.iWidth == 640 && sz.iHeight == 480)
      sz = TSize(320,240);
    if(iImg->Create(sz, iImgType) != KErrNone)
      {
        delete iImg;
	iImg = 0;
      }
    else
      {
        TRAP(aError,iLd->ConvertL(*iImg, 0));
      }
  }

  void
  MiuoConvertComplete(TInt aError)
  {
    CActiveScheduler::Stop();
  }

  CFbsBitmap *iImg;
  TDisplayMode iImgType;
  CMdaImageFileToBitmapUtility *iLd;
};


CFbsBitmap *
LoadJpgImage(const TDesC &path, TSize dsz, TDisplayMode aImgType)
{
  CImageLoadObserver *obs = new CImageLoadObserver();
  obs->iLd = CMdaImageFileToBitmapUtility::NewL(*obs);
  obs->iImgType = aImgType;

  TRAPD(oerr, obs->iLd->OpenL(path));
  if(!oerr)
    CActiveScheduler::Start();

  // If we are here, the load process is finished
  CFbsBitmap *img = obs->iImg;
  TSize ssz = img->SizeInPixels();

  if(ssz.iWidth > dsz.iWidth || ssz.iHeight > dsz.iHeight)
    {
      // Now scale the image. 
      double r1 = (double)ssz.iWidth/(double)dsz.iWidth;
      double r2 = (double)ssz.iHeight/(double)dsz.iHeight;
      if(r2 < r1)
	r1 = r2;
      TSize nsz;
      nsz.iWidth = (int)(dsz.iWidth  * r1);
      nsz.iHeight = (int)(dsz.iHeight  * r1);

      TPoint off;
      off.iX = (ssz.iWidth  - nsz.iWidth) / 2;
      off.iY = (ssz.iHeight - nsz.iHeight) / 2;

      img = new (ELeave) CFbsBitmap();
      img->Create(dsz, aImgType);
      CFbsBitmapDevice *dev = CFbsBitmapDevice::NewL(img);
      CBitmapContext *gc = 0;
      dev->CreateBitmapContext(gc);
      gc->DrawBitmap(TRect(TPoint(0,0),dsz), obs->iImg, TRect(off, nsz));
      delete obs->iImg;
    }

  delete obs->iLd;
  delete obs;
  return img;
}
