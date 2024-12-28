//_____________________________________________________________________________
//
//  File:      TFImage.h
//
//  Version:   1.0
//
//  Author:    Reiner Rohlfs (GADC)
//
//  History:   1.0   12.08.03  first released version
//             1.1.5 08.12.06  TFBaseImage copy constructor
//
//_____________________________________________________________________________
#ifndef ROOT_TFImage
#define ROOT_TFImage

#ifndef ROOT_TTree
#include "TTree.h"
#endif

#ifndef ROOT_TH2
#include "TH2.h"
#endif

#ifndef ROOT_TFIOElement
#include "TFIOElement.h"
#endif

class TFNameConvert;
template <class T, class F> class TFImage;


template <class T>
class TFImageSplice
{
   T        * fData;
   UInt_t   * fSize;
   UInt_t   * fSubOffset;
   UInt_t   * fSubFreeze;

public:
   TFImageSplice() {fData = NULL; fSize = NULL; fSubOffset = NULL; fSubFreeze = NULL;}
   TFImageSplice(T * data, UInt_t * size)
      : fData(data), fSize(size) {}
   TFImageSplice(T * data, UInt_t * size, UInt_t * subOffset, UInt_t * subFreeze)
      : fData(data), fSize(size), fSubOffset(subOffset), fSubFreeze(subFreeze) {}

   TFImageSplice<T>    operator [] (int index) {return TFImageSplice<T> (fData + index * *fSize, fSize + 1);}
   TFImageSplice<T>    operator () (int index) {return TFImageSplice<T> (fData + (index + *fSubOffset) * *fSize + *fSubFreeze, 
                                                                         fSize + 1, fSubOffset + 1, fSubFreeze + 1);}
                       operator T  ()          {return *fData;}
   TFImageSplice<T> &  operator = (T data)     {*fData = data; return *this;}

   ClassDef(TFImageSplice, 0) // internal class for TFImage
};

//_____________________________________________________________________________

class TFBaseImage : public TFIOElement
{
protected:
   UInt_t   fNumData;      // Number of pixels in image
   UInt_t   fNumDim;       // Number of dimensions of image
   UInt_t   * fSize;       // [fNumDim]  Coded size of image in each dimension

   Bool_t   fSubImage;     //! kTRUE if subimage is defined
   UInt_t   fNumSubDim;    //! Number of dimensions of sub image
   UInt_t   * fSubOffset;  //! Offset in each dimension of the subSection
   UInt_t   * fSubSize;    //! Size of the subimage in each not frozen dimension
   UInt_t   * fSubFreeze;  //! Frozen offset in each dimension of the subSection
   UInt_t   * fSizeNFr;    //! Original size of the subimage in each not frozen dimension

public:
   TFBaseImage();
   TFBaseImage(const TFBaseImage & image);
   TFBaseImage(const char * name, UInt_t dim1);
   TFBaseImage(const char * name, UInt_t dim1, UInt_t dim2);
   TFBaseImage(const char * name, UInt_t dim1, UInt_t dim2, UInt_t dim3);
   TFBaseImage(const char * name, UInt_t numDim, UInt_t * size);

   TFBaseImage(const char * name, const char * fileName, UInt_t dim1);
   TFBaseImage(const char * name, const char * fileName, UInt_t dim1, UInt_t dim2);
   TFBaseImage(const char * name, const char * fileName, UInt_t dim1, UInt_t dim2, UInt_t dim3);
   TFBaseImage(const char * name, const char * fileName, UInt_t numDim, UInt_t * size);

   virtual ~TFBaseImage();
   
   virtual UInt_t    GetNumDim(Bool_t sub = kFALSE) const     {return sub ? fNumSubDim : fNumDim;}
   virtual void      GetSize(UInt_t * size, Bool_t sub = kFALSE) const;
   virtual UInt_t    GetNumPixel(Bool_t sub = kFALSE);

/*   virtual TFBaseImage & operator =  (const TFBaseImage & image);
   virtual bool          operator == (const TFHeader & image) const;
*/
   virtual void      MakeSubSection(UInt_t * begin, UInt_t * end);
   virtual void      ResetSubSection();
   virtual Bool_t    IsSubSection() const      {return fSubImage;}

   virtual TH1 *     MakeHisto(TClass * type = TH2D::Class());
   virtual TH1 *     MakeHisto(UInt_t zPos, TClass * type = TH2D::Class());
   virtual TTree *   MakeTree(TFNameConvert * nameConvert = NULL) const;

           operator  TFImage<Bool_t, BoolFormat>       * ();
           operator  TFImage<Char_t, CharFormat>       * ();
           operator  TFImage<UChar_t, UCharFormat>     * ();
           operator  TFImage<Short_t, ShortFormat>     * ();
           operator  TFImage<UShort_t, UShortFormat>   * ();
           operator  TFImage<Int_t, IntFormat>         * ();
           operator  TFImage<UInt_t, UIntFormat>       * ();
           operator  TFImage<Float_t, FloatFormat>     * ();
           operator  TFImage<Double_t, DoubleFormat>   * ();

protected:
           void   InitMemory();
   virtual void   FillHist(TH1 * hist, UInt_t xSize) {};
   virtual void   FillHist(TH2 * hist, UInt_t ySize, UInt_t xSize) {};
   virtual void   FillHist_3D(TH2 * hist, UInt_t zPos, UInt_t ySize, UInt_t xSize) {};
   virtual void   MakePixelBranch(TTree * tree) const {}
   virtual Bool_t FillBranchBuffer(UInt_t index) const {return kFALSE;}

// current version of THtml will not create the description of the constructor
// with these pure virtual functions. 
//   virtual void   FillHist(TH1 * hist, UInt_t xSize) = 0;
//   virtual void   FillHist(TH2 * hist, UInt_t ySize, UInt_t xSize) = 0;

   ClassDef(TFBaseImage, 1) // base class of TFImage without data
};

//_____________________________________________________________________________

template <class T, class F = DefaultFormat<T> > 
class TFImage : public TFBaseImage
{
protected:
   T           * fData;       // [fNumData] data of the image
   T           fNull;         // NULL value of this image
   Bool_t      fNullDefined;  // kTRUE if a NULL value is defined
   mutable T   treeBuffer;    //! buffer to fill a TTree

public:
   TFImage() {fData = NULL; ClearNull();}
   TFImage(const char * name, UInt_t dim1) : TFBaseImage(name, dim1)                   
      {fData = new T [fNumData]; ClearNull();}
   TFImage(const char * name, UInt_t dim1, UInt_t dim2) : TFBaseImage(name, dim1, dim2)                   
      {fData = new T [fNumData]; ClearNull();}
   TFImage(const char * name, UInt_t dim1, UInt_t dim2, UInt_t dim3) : TFBaseImage(name, dim1, dim2, dim3)
      {fData = new T [fNumData]; ClearNull();}
   TFImage(const char * name, UInt_t numDim, UInt_t * size) : TFBaseImage(name, numDim, size)                   
      {fData = new T [fNumData]; ClearNull();}

   TFImage(const char * name, const char * fileName, UInt_t dim1) : TFBaseImage(name, fileName, dim1)                   
      {fData = new T [fNumData]; ClearNull(); if(fio) fio->CreateElement();}
   TFImage(const char * name, const char * fileName, UInt_t dim1, UInt_t dim2) : TFBaseImage(name, fileName, dim1, dim2)                   
      {fData = new T [fNumData]; ClearNull(); if(fio) fio->CreateElement();}
   TFImage(const char * name, const char * fileName, UInt_t dim1, UInt_t dim2, UInt_t dim3) : TFBaseImage(name, fileName, dim1, dim2, dim3)
      {fData = new T [fNumData]; ClearNull(); if(fio) fio->CreateElement();}
   TFImage(const char * name, const char * fileName, UInt_t numDim, UInt_t * size) : TFBaseImage(name, fileName, numDim, size)                   
      {fData = new T [fNumData]; ClearNull(); if(fio) fio->CreateElement();}
   
   TFImage(const TFImage<T, F> & image) : TFBaseImage(image)
      {fData = new T [fNumData];
       memcpy(fData, image.fData, fNumData * sizeof(T));
       fNull = image.fNull; fNullDefined = image.fNullDefined;}

   ~TFImage()  {delete [] fData;}

   virtual TFImage<T,F> & operator = (const TFImage<T,F> & image);
   virtual bool           operator == (const TFHeader & image) const;

   virtual T      GetNull() const      {return fNull;}
   virtual void   SetNull(T null)      {fNull = null; fNullDefined = kTRUE;}
   virtual void   ClearNull()          {fNullDefined = kFALSE;}
   virtual Bool_t NullDefined() const  {return fNullDefined;}

           T *    GetDataArray()    {return fData;}

   TFImageSplice<T> operator[] (int index)      {return TFImageSplice<T> (fData + index * *fSize, fSize + 1);}
   TFImageSplice<T> operator() (int index)      {return TFImageSplice<T> (fData + (index + *fSubOffset) * *fSizeNFr + *fSubFreeze, 
                                                                          fSizeNFr + 1, fSubOffset + 1, fSubFreeze + 1);}

protected:
   virtual void   FillHist(TH1 * hist, UInt_t xSize) {
                      if (IsSubSection())
                        for (int x = 0; x < xSize; x++) 
                           if (fNullDefined)
                              {
                              T val = fData[(x + *fSubOffset) * *fSizeNFr + *fSubFreeze];
                              hist->Fill(x + 1, (double) (val == fNull) ? 0 : val);
                              }
                           else
                              hist->Fill(x + 1, (double)(fData[(x + *fSubOffset) * *fSizeNFr + *fSubFreeze]));
                      else
                        for (int x = 0; x < xSize; x++) 
                           if (fNullDefined)
                              {
                              T val = fData[x * *fSize];
                              hist->Fill(x + 1, (double) (val == fNull) ? 0 : val);
                              }
                           else
                              hist->Fill(x + 1, (double)(fData[x * *fSize]));}

   virtual void   FillHist(TH2 * hist, UInt_t ySize, UInt_t xSize) {
                   if (IsSubSection())
                      for (int y = 0; y < ySize; y++) 
                        for (int x = 0; x < xSize; x++) 
                           if (fNullDefined)
                              {
                              T val = (T)(operator[](y)(x));
                              hist->Fill(x + 1, y + 1, (double) (val == fNull ? 0 : val));
                              }
                           else 
                              hist->Fill(x + 1, y + 1, (double) (T)(operator()(y)(x)));
                   else  
                      for (int y = 0; y < ySize; y++) 
                        for (int x = 0; x < xSize; x++) 
                           if (fNullDefined)
                              {
                              T val = (T)(operator[](y)[x]);
                              hist->Fill(x + 1, y + 1, (double) (val == fNull ? 0 : val));
                              }
                           else 
                              hist->Fill(x + 1, y + 1, (double) (T)(operator[](y)[x]));}

   virtual void   FillHist_3D(TH2 * hist, UInt_t zPos, UInt_t ySize, UInt_t xSize) {
                   if (IsSubSection())
                      for (int y = 0; y < ySize; y++) 
                        for (int x = 0; x < xSize; x++) 
                           if (fNullDefined)
                              {
                              T val = (T)(operator[](zPos)(y)(x));
                              hist->Fill(x + 1, y + 1, (double) (val == fNull ? 0 : val));
                              }
                           else 
                              hist->Fill(x + 1, y + 1, (double) (T)(operator()(zPos)(y)(x)));
                   else  
                      for (int y = 0; y < ySize; y++) 
                        for (int x = 0; x < xSize; x++) 
                           if (fNullDefined)
                              {
                              T val = (T)(operator[](zPos)[y][x]);
                              hist->Fill(x + 1, y + 1, (double) (val == fNull ? 0 : val));
                              }
                           else 
                              hist->Fill(x + 1, y + 1, (double) (T)(operator[](zPos)[y][x]));}



   virtual void   MakePixelBranch(TTree * tree) const 
                     {
                        if (F::GetBranchType()[0])
                           {
                           char branch[80];                                          
                           sprintf(branch, "pixel%s", F::GetBranchType());
                           tree->Branch("pixel", &treeBuffer, (const char*)branch);
                           }
                     }

   virtual Bool_t FillBranchBuffer(UInt_t index) const 
                     {
                     if (fNullDefined && fData[index] == fNull) return kFALSE;
                     treeBuffer = fData[index]; return kTRUE;
                     }

   
   ClassDef(TFImage, 1) // an image with n dimension
};


//_____________________________________________________________________________
//_____________________________________________________________________________

typedef    TFImage<Bool_t, BoolFormat>      TFBoolImg;
typedef    TFImage<Char_t, CharFormat>      TFCharImg;
typedef    TFImage<UChar_t, UCharFormat>    TFUCharImg;
typedef    TFImage<Short_t, ShortFormat>    TFShortImg;
typedef    TFImage<UShort_t, UShortFormat>  TFUShortImg;
typedef    TFImage<Int_t, IntFormat>        TFIntImg;
typedef    TFImage<UInt_t, UIntFormat>      TFUIntImg;
typedef    TFImage<Float_t, FloatFormat>    TFFloatImg;
typedef    TFImage<Double_t, DoubleFormat>  TFDoubleImg;

//_____________________________________________________________________________
//_____________________________________________________________________________

template <class T, class F> 
inline TFImage<T, F> & TFImage<T, F>::operator = (const TFImage<T, F> & image)
{ 
   if (&image != this)
      {
      TFBaseImage::operator=(image);
      delete [] fData;
      fData = new T [fNumData];
      memcpy(fData, image.fData, fNumData * sizeof(T));
      fNull = image.fNull; 
      fNullDefined = image.fNullDefined;
      }

   return *this;
}


template <class T, class F> 
inline bool TFImage<T, F>::operator == (const TFHeader & image) const
{
   if ( !TFBaseImage::operator==(image)                   ||
        IsA() != image.IsA()                              ||
        fNullDefined != ((TFImage<T,F>&)image).fNullDefined ||
        (fNullDefined && (fNull != ((TFImage<T,F>&)image).fNull))  )
      return false;

   T * tmp = ((TFImage<T,F>&)image).fData;
   for (UInt_t num = 0; num < fNumData; num++)
      if (fData[num] != tmp[num])
         return false;

   return true;
}

inline TFBaseImage::operator TFImage<Bool_t, BoolFormat>       * () {return dynamic_cast <TFImage<Bool_t, BoolFormat>      *>(this);}
inline TFBaseImage::operator TFImage<Char_t, CharFormat>       * () {return dynamic_cast <TFImage<Char_t, CharFormat>      *>(this);}
inline TFBaseImage::operator TFImage<UChar_t, UCharFormat>     * () {return dynamic_cast <TFImage<UChar_t, UCharFormat>    *>(this);}
inline TFBaseImage::operator TFImage<Short_t, ShortFormat>     * () {return dynamic_cast <TFImage<Short_t, ShortFormat>    *>(this);}
inline TFBaseImage::operator TFImage<UShort_t, UShortFormat>   * () {return dynamic_cast <TFImage<UShort_t, UShortFormat>  *>(this);}
inline TFBaseImage::operator TFImage<Int_t, IntFormat>         * () {return dynamic_cast <TFImage<Int_t, IntFormat>        *>(this);}
inline TFBaseImage::operator TFImage<UInt_t, UIntFormat>       * () {return dynamic_cast <TFImage<UInt_t, UIntFormat>      *>(this);}
inline TFBaseImage::operator TFImage<Float_t, FloatFormat>     * () {return dynamic_cast <TFImage<Float_t, FloatFormat>    *>(this);}
inline TFBaseImage::operator TFImage<Double_t, DoubleFormat>   * () {return dynamic_cast <TFImage<Double_t, DoubleFormat>  *>(this);}


extern TFBaseImage *  TFReadImage(const char * fileName, const char * name,  
                                  UInt_t cycle = 0, FMode mode = kFRead);

#endif // ROOT_TFImage
