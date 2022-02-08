#ifndef __UTIL_H
#define __UTIL_H

void debugline(const char *a, ...);
#define DEBUGLINE debugline("%s:%s: %d", __BASE_FILE__, __FUNCTION__, __LINE__)

CFbsBitmap *LoadJpgImage(const TDesC &aFname, TSize aSize, TDisplayMode aImgType);

#define CHAR unsigned short

class Cnv {
public:
  Cnv();

  const TDesC &FromUtf8(const char *);
  const char *ToUtf8(const TDesC &);

private:
  TPtr*   iWP;
  int     iWideLength;
  CHAR*   iWideBuffer;
  int     iUtf8Length;
  char*   iUtf8Buffer;
};

#endif
