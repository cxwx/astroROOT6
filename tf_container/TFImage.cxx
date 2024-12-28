// ///////////////////////////////////////////////////////////////////
//
//  File:      TFImage.cpp
//
//  Version:   1.0
//
//  Author:    Reiner Rohlfs (GADC)
//
//  History:   1.0    12.08.03  first released version
//             1.1.2  14.02.05  new MakeHisto() function for 3 dim 
//                              images
//             1.1.5  08.12.06  TFBaseImage copy constructor
//
// ///////////////////////////////////////////////////////////////////
#include "TFImage.h"
#include "TFNameConvert.h"

#ifndef TF_CLASS_IMP
#define TF_CLASS_IMP
ClassImp(TFBaseImage)
ClassImpT(TFImage, T)
ClassImpT(TFImageSplice, T)
#endif

//_____________________________________________________________________________
// TFBaseImage, TFImage:
//
//    A TFImage is a n - dimensional array of data. The template class
//    TFImage stores the data in differend data formats while common information
//    like the number of dimensins and the size is stored in the base class
//    TFBaseImage.
//    A TFImage has a name (see TNamed) and a header (see TFHeader) with
//    Attributes (see TFBaseAttr).
//    A TFImage can be created in memory without a reference in a file.
//    But also this kind of image can be saved later in an ASRO file, in  
//    a ROOT file or in a FITS file.
//    A TFImage can be read from a ASRO file, ROOT file and a FITS file at 
//    creation time (see function TFReadImage)
//    All changes of a TFImage are done only in memory. For example resizing
//    the image or updating the pixel value. To save the changes the member
//    function SaveElement() has to be called.
//
//    Any pixel of an image can be accesed (read and write) like a 
//    n - dimensional C - array.:
//    image[z][y][x] = 4;
//    double val = image[z][y][x]
//    Note: the most frequently changing dimension is the most right index,
//    like in C - arrays, not as in FORTRAN - arrays.
//
//    A sub-section (see MakeSubSection() ) can be defined. To access a pixel
//    of a previously defined sub-section the operator() instead of the 
//    operator [] has to be used. For example:
//    subImage(z)(x) = 4.5;
//
//
// TFImageSplice:
//    This is an internal class and should not be used direcly by an application


//_____________________________________________________________________________
TFBaseImage *  TFReadImage(const char * fileName, const char * name,  
                           UInt_t cycle, FMode mode)
{
// reads an image from a file 

   TFIOElement * element = TFRead(fileName, name, cycle, mode, TFBaseImage::Class());

   TFBaseImage * image = dynamic_cast<TFBaseImage *>(element);
   if (image == NULL)
      delete element;

   return image;
}
//_____________________________________________________________________________
TFBaseImage::TFBaseImage() 
{
// Default constructor. The image has 0 dimension and is not
// associated with an image in a file.
// Don't use this constructor. The image should have at least a name.

   fNumData    = 0; 
   fNumDim     = 0; 
   fSize       = NULL; 

   fSubImage   = kFALSE;
   fNumSubDim  = 0;
   fSubOffset  = NULL; 
   fSubSize    = NULL;
   fSubFreeze  = NULL; 
   fSizeNFr    = NULL;
}
//_____________________________________________________________________________
TFBaseImage::TFBaseImage(const TFBaseImage & image)
   : TFIOElement(image)
{
// copy constructor
   fNumDim   = image.fNumDim;
   fNumData  = image.fNumData;
   InitMemory();

   memcpy(fSize,      image.fSize,      fNumDim * sizeof(UInt_t) );
   memcpy(fSubOffset, image.fSubOffset, fNumDim * sizeof(UInt_t) );
   memcpy(fSubSize,   image.fSubSize,   fNumDim * sizeof(UInt_t) );
   memcpy(fSubFreeze, image.fSubFreeze, fNumDim * sizeof(UInt_t) );
   memcpy(fSizeNFr,   image.fSizeNFr,   fNumDim * sizeof(UInt_t) );

   fSubImage  = image.fSubImage;
   fNumSubDim = image.fNumSubDim;

}
//_____________________________________________________________________________
TFBaseImage::TFBaseImage(const char * name, UInt_t dim1)
   :TFIOElement(name)
{
// Creates a 1 - dimensional image in memory

   fNumDim = 1;
   InitMemory();
   fSize[0] = 1;
   fNumData = dim1;

   ResetSubSection();
}
//_____________________________________________________________________________
TFBaseImage::TFBaseImage(const char * name, UInt_t dim1, UInt_t dim2)
   :TFIOElement(name)
{
// Creates a 2 - dimensional image in memory
// dim2 is the most frequently changing dimension, i.e X
// to access one pixel use image[dim1][dim2] 

   fNumDim = 2;
   InitMemory();
   fSize[1] = 1;
   fSize[0] = dim2;
   fNumData = fSize[0] * dim1;

   ResetSubSection();
}
//_____________________________________________________________________________
TFBaseImage::TFBaseImage(const char * name, UInt_t dim1, UInt_t dim2, UInt_t dim3)
   :TFIOElement(name)
{
// Creates a 3 - dimensional image in memory
// dim3 is the most frequently changing dimension, i.e X
// to access one pixel use image[dim1][dim2][dim3] 

   fNumDim = 3;
   InitMemory();
   fSize[2] = 1;
   fSize[1] = dim3;
   fSize[0] = fSize[1] * dim2;
   fNumData = fSize[0] * dim1;

   ResetSubSection();
}
//_____________________________________________________________________________
TFBaseImage::TFBaseImage(const char * name, UInt_t numDim, UInt_t * size)
   :TFIOElement(name)
{
// Creates a n - dimensional image in memory
// to create a 5 - dimensional image one can write:
//    UInt_t size[5] = {4, 6, 3, 10, 15};
//    TIntImage * img = new TIntImage("name", 5, size);
// Again the most right index (in the example the value 15) is the most 
// frequently changing dimension

   fNumDim = numDim;
   InitMemory();
   fSize[numDim - 1] = 1;
   for (int dim = numDim - 2; dim >= 0; dim--)
      fSize[dim] = fSize[dim + 1] * size[dim + 1];

   fNumData = fSize[0] * size[0];

   ResetSubSection();
}
//_____________________________________________________________________________
TFBaseImage::TFBaseImage(const char * name, const char * fileName, UInt_t dim1)
   :TFIOElement(name, fileName)
{
// Creates a new 1 - dimensional image in memory and associates it with a new
// image in a file.

   fNumDim = 1;
   InitMemory();
   fSize[0] = 1;
   fNumData = dim1;

   ResetSubSection();
}
//_____________________________________________________________________________
TFBaseImage::TFBaseImage(const char * name, const char * fileName, 
                         UInt_t dim1, UInt_t dim2)
   :TFIOElement(name, fileName)
{
// Creates a new 2 - dimensional image in memory and associates it with a new
// image in a file.
// dim2 is the most frequently changing dimension, i.e X
// to access one pixel use image[dim1][dim2] 

   fNumDim = 2;
   InitMemory();
   fSize[1] = 1;
   fSize[0] = dim2;
   fNumData = fSize[0] * dim1;

   ResetSubSection();
}
//_____________________________________________________________________________
TFBaseImage::TFBaseImage(const char * name, const char * fileName, 
                         UInt_t dim1, UInt_t dim2, UInt_t dim3)
   :TFIOElement(name, fileName)
{
// Creates a new 3 - dimensional image in memory and associates it with a new
// image in a file.
// dim3 is the most frequently changing dimension, i.e X
// to access one pixel use image[dim1][dim2][dim3] 

   fNumDim = 3;
   InitMemory();
   fSize[2] = 1;
   fSize[1] = dim3;
   fSize[0] = fSize[1] * dim2;
   fNumData = fSize[0] * dim1;

   ResetSubSection();
}
//_____________________________________________________________________________
TFBaseImage::TFBaseImage(const char * name, const char * fileName, 
                         UInt_t numDim, UInt_t * size)
   :TFIOElement(name, fileName)
{
// Creates a new 3 - dimensional image in memory and associates it with a new
// image in a file.
// to create a 5 - dimensional image one can write:
//    UInt_t size[5] = {4, 6, 3, 10, 15};
//    TIntImage * img = new TIntImage("name", 5, size);
// Again the most right index (in the example the value 15) is the most 
// frequently changing dimension

   fNumDim = numDim;
   InitMemory();
   fSize[numDim - 1] = 1;
   for (int dim = numDim - 2; dim >= 0; dim--)
      fSize[dim] = fSize[dim + 1] * size[dim + 1];

   fNumData = fSize[0] * size[0];

   ResetSubSection();
}
//_____________________________________________________________________________
TFBaseImage::~TFBaseImage()  
{
// destructor

   delete [] fSize; 
   delete [] fSubOffset; 
   delete [] fSubSize;
   delete [] fSubFreeze; 
   delete [] fSizeNFr;
}
//_____________________________________________________________________________
void TFBaseImage::InitMemory()
{
// protected funcion to initialze some arrays.

   fSubOffset = new UInt_t[fNumDim];
   fSubSize   = new UInt_t[fNumDim];
   fSubFreeze = new UInt_t[fNumDim];
   fSizeNFr   = new UInt_t[fNumDim];
   fSize      = new UInt_t[fNumDim];
}
//_____________________________________________________________________________
void TFBaseImage::GetSize(UInt_t * size, Bool_t sub) const
{
// Returns the size in each dimension. The array of size must be large enough
// to hold a value for each dimension. Use GetNumDim() to ask for the number
// of dimesnions of the image. 
// If sub == kFALSE (the default) the size of the original image is returnd
// independent of a previously call of the MakeSubSection() - function.
// If sub == kTRUE the size of a sub - image is returned which was previously
// defined with the MakeSubSection() - function.
// In any case size[0] will hold the size of the least frequently changing
// dimension. For example for a 2 - dimensional image the Y - axis.

   if (sub)
      {
      for (int dim = 0; dim < fNumSubDim; dim++)
         size[dim] = fSubSize[dim];
      }
   else
      {
      size[0] = fNumData / fSize[0];
      for (int dim = 1; dim < fNumDim; dim++)
         size[dim] = fSize[dim-1] / fSize[dim];
      }
}
//_____________________________________________________________________________
UInt_t TFBaseImage::GetNumPixel(Bool_t sub)
{
// returns the total number of pixels of the original image (sub == kTRUE)
// or of a previously defined sub - image (sub = kTRUE)

   if (sub)
      {
      UInt_t numP = 1;
      for (int dim = 0; dim < fNumSubDim; dim++)
         numP *= fSubSize[dim];
      return numP;
      }

   return fNumData;      

}
//_____________________________________________________________________________
void TFBaseImage::MakeSubSection(UInt_t * begin, UInt_t * end)
{
// Defines a subsection of an image. 
// begin defines the first pixel in each dimension of the original 
// image which will be become the index 0 of the sub section.
// end defines the pixel in each dimension of the original 
// image which will be behind the last pixel of the sub - image.
// The size of the sub - image in a dimension will be 
// end[dimX] - begin[dimX]
// The first index ( index 0 ) of begin and end defines the least
// frequently changing dimension. For example in a 2 - dimensional
// image the Y - axis.
//
// A dimension can be freezed if begin[dimX] == end[dimX] 
// A pixel of the sub - image must be accessed with the operator ()
// instead of the opertor []. Frozen dimensions must be skipped.
// 
// For example:
//    TFIntImg img("name", 10, 5, 20);
//    UInt_t begin[3] = {3 , 3, 10};
//    UInt_t end[3]   = {10, 3, 15};
//    img.MakeSubSection(begin, end);
//
// The sub - image of img has 2 dimensions and is of size  7 X 5
// img[4][3][12] will now access the same pixel as img(1)(2)
//
// As second call of this function will first reset the image to its
// original size and than apply the begin and end - values.

   ResetSubSection();
   fSubImage = kTRUE;

   
   fNumSubDim = 0;
   for (UInt_t dim = 0; dim < fNumDim; dim++)
      {
      fSizeNFr[fNumSubDim] = fSize[dim];
      fSubOffset[fNumSubDim] = begin[dim];
      if (end[dim] == begin[dim])
         // freeze this dimension
         fSubFreeze[fNumSubDim] += begin[dim] * fSize[dim];
      else if (end[dim] > begin[dim])
         {
         fSubSize[fNumSubDim] = end[dim] - begin[dim];
         fNumSubDim++;
         }
      }
   
   if (fNumSubDim == 0)
      return;

   UInt_t dim = fNumDim - 1;
   while (end[dim] <= begin[dim])
      {
      fSubFreeze[fNumSubDim - 1] += begin[dim] * fSize[dim];
      if (dim == 0)
         break;
      dim--;
      }
}
//_____________________________________________________________________________
void TFBaseImage::ResetSubSection()
{
// Resets a previously resized image ( fucntion MakeSubSection() ) to its
// original size.

   for (int dim = 0; dim < fNumDim; dim++)
      {
      fSubOffset[dim] = 0;
      fSubFreeze[dim] = 0;
      fSizeNFr[dim]   = fSize[dim];
      }

   fNumSubDim = 0;
   fSubImage = kFALSE;
}
//_____________________________________________________________________________
TH1 * TFBaseImage::MakeHisto(TClass * type)
{
// Create a new histogram of the required type 
// Only one and two dimensional histograms are supported
// If a sub - image is defined ( function MakeSubSection() ) the histogram
// will be created using only the sub -image, else the full image is 
// used. If the image has more dimensins than the histogram the least
// frequently dimensions of the image are used. 
// For example to create a two dimensional histogram of the X - and Z - dimension
// from a 3 - dimensional image first a sub - section with a freezing
// third dimension has to be defined. For example:
//
//    UInt_t begin[3] = {3,  2, 20};
//    UInt_t end[3]   = {35, 2, 432};
//    img.MakeSubSection(begin, end);
//    TH2D * hist = dynamic_cast<TH2D*>(img.MakeHisto());
//
// The parameter type defines the Class type of the returned histogram.
// For example:
//    TH2F * hist = (TH2F*)MakeHisto( TH2F::Class() )
// 
// The returned histogram has to be deleted by the calling function

   return MakeHisto(0, type);
}

//_____________________________________________________________________________
TH1 * TFBaseImage::MakeHisto(UInt_t zPos, TClass * type)
{
// Create a new histogram of the required type 
// Only one and two dimensional histograms are supported
//
// If this image is has 3 dimensions (z,y,x) and the histogram has 2 
// dimensions (y,x) the parameter zPos defines the z - position which
// is used to build the histogram. zPos = 0 defines the first layer.
// In any other case the value of zPos has not affect. To build a 
// subcube of the images use the function MakeSubSection() as described
// hereafter.
//
// If a sub - image is defined ( function MakeSubSection() ) the histogram
// will be created using only the sub -image, else the full image is 
// used. If the image has more dimensins than the histogram the least
// frequently dimensions of the image are used. 
// For example to create a two dimensional histogram of the X - and Z - dimension
// from a 3 - dimensional image first a sub - section with a freezing
// third dimension has to be defined. For example:
//
//    UInt_t begin[3] = {3,  2, 20};
//    UInt_t end[3]   = {35, 2, 432};
//    img.MakeSubSection(begin, end);
//    TH2D * hist = dynamic_cast<TH2D*>(img.MakeHisto());
//
// The parameter type defines the Class type of the returned histogram.
// For example:
//    TH2F * hist = (TH2F*)MakeHisto( TH2F::Class() )
// 
// The returned histogram has to be deleted by the calling function

   TH1 * hist = (TH1*)type->New();
   hist->SetName(GetName());

   // get size of this image
   UInt_t numDim = GetNumDim(IsSubSection());
   UInt_t * size = new UInt_t[numDim];
   GetSize(size, IsSubSection());

   if (hist->GetDimension() == 1)
      {
      hist->SetBins(size[numDim-1], 0.5, size[numDim-1] + 0.5);

      FillHist(hist, size[numDim-1]);
      }
   else if (hist->GetDimension() == 2)
      {
      if ( numDim < 2 || numDim > 3)
         {
         delete hist;
         delete [] size;
         return NULL;
         }

      hist->SetBins(size[numDim-1], 0.5, size[numDim-1] + 0.5, 
                    size[numDim-2], 0.5, size[numDim-2] + 0.5);

      if (numDim == 2)
         FillHist((TH2*)hist, size[numDim - 2], size[numDim - 1]);
      else
         {
         if (zPos < 0) zPos = 0;
         if (zPos >= size[0]) zPos = size[0] - 1;
         FillHist_3D((TH2*)hist, zPos, size[numDim - 2], size[numDim - 1]);
         }
      }
   else
      {
      // we do not support 3 dimensional histograms
      delete hist;
      delete [] size;
      return NULL;
      }

   delete [] size;
   return hist;
}
//_____________________________________________________________________________
TTree * TFBaseImage::MakeTree(TFNameConvert * nameConvert) const
{
// cretes a TTree. One branch is the pixel value the other branches are
// the axis of this image. There is one record per image pixel which are
// not NULL. If a sub - image is defined the tree will be build only of 
// the pixels of this sub - image.
// nameConvert can be NULL. But it will be adopted by this
// function and will be deleted by this function if it is not NULL.
// The calling function has to delete the returning tree.
   if (nameConvert == NULL)
      nameConvert = new TFNameConvert();

   TTree * tree= new TTree(nameConvert->Conv(GetName()), nameConvert->Conv(GetName()) );

   // create branch for pixel values
   MakePixelBranch(tree);

   UInt_t numDim = GetNumDim(IsSubSection());
   if (numDim == 0)
      {
      delete nameConvert;
      return tree;
      }

   // get size of image
   UInt_t * pos  = new UInt_t[numDim];
   UInt_t * size = new UInt_t[numDim];
   GetSize(size, IsSubSection());

   // create one branch per dimension
   UInt_t dim = numDim;
   do {
      dim--;
      char branch[10];                                          
      char name[10];
      if (numDim <= 3)
         {
         // branches are called x, y, z
         if      (dim == numDim -1) strcpy(name, "x");
         else if (dim == numDim -2) strcpy(name, "y");
         else                       strcpy(name, "z");
         }
      else
         // branches are called a1, a2, a3, ...
         sprintf(name,   "a%u", numDim - dim);
      sprintf(branch, "%s/i", name);
      tree->Branch((const char*)name, pos + dim, (const char*)branch);
      pos[dim] = 0;
      } while (dim != 0);
   

   // fill the tree pixel by pixel
   if (IsSubSection())
      {
      // for sub- image
      do {
         int index = 0;
         for (int dim = 0; dim < numDim; dim++)
            index += (pos[dim] + fSubOffset[dim]) * fSizeNFr[dim] + fSubFreeze[dim];

         if (FillBranchBuffer(index))
            tree->Fill();
            
         // set pos for next pixel
         UInt_t dim = numDim;
         do {
            dim--;
            pos[dim] += 1;
            if (pos[dim] != size[dim])
               break;
            pos[dim] = 0;
            } while (dim != 0);

         int num = 0;
         for (int dim = 0; dim < numDim; dim++)
            num += pos[dim];
         if (num == 0) break;
         } while (1);
      }
   else
      {
      // original size of image
      for (UInt_t index = 0; index < fNumData; index++)
         {
         if (FillBranchBuffer(index))
            tree->Fill();
            
         // set pos for next pixel
         UInt_t dim = numDim;
         do {
            dim--;
            pos[dim] += 1;
            if (pos[dim] != size[dim])
               break;
            pos[dim] = 0;
            } while (dim != 0);

         }
      }

   delete nameConvert;
   delete [] pos;
   delete [] size;

   return tree;

}

