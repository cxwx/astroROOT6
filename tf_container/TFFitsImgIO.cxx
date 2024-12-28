// ///////////////////////////////////////////////////////////////////
//
//  File:      TFFitsImgIO.cxx
//
//  Version:   1.0
//
//  Author:    Reiner Rohlfs (GADC)
//
//  History:   1.0   13.08.03  first released version
//
// ///////////////////////////////////////////////////////////////////
#include <float.h>
#include <math.h>

#include "TFFitsIO.h"
#include "TFError.h"
#include "TFIOElement.h"
#include "TFImage.h"

#include "fitsio.h"

static const char * errMsg =
{"Get cfitsio error %d while reading image from file %s"};


static TFIOElement * ReadByteImage(fitsfile *fptr, int numDim, 
                                   long * axisSize, int * status);
static TFIOElement * ReadShortImage(fitsfile *fptr, int numDim, 
                                   long * axisSize, int * status);
static TFIOElement * ReadIntImage(fitsfile *fptr, int numDim, 
                                   long * axisSize, int * status);
static TFIOElement * ReadFloatImage(fitsfile *fptr, int numDim, 
                                   long * axisSize, int * status);
static TFIOElement * ReadDoubleImage(fitsfile *fptr, int numDim, 
                                   long * axisSize, int * status);

//_____________________________________________________________________________
int CreateFitsImage(fitsfile* fptr, TFBaseImage* image)
{
// Creatres a new image in a FITS file

   int type= -10000;
   int bzero = 0;
   unsigned int ubzero = 0;

   if (image->IsA() == TFBoolImg::Class())
      type = BYTE_IMG;
   else if (image->IsA() == TFCharImg::Class())
      {
      type = BYTE_IMG;
      bzero = -128;
      }
   else if (image->IsA() == TFUCharImg::Class())
      type = BYTE_IMG;
   else if (image->IsA() == TFShortImg::Class())
      type = SHORT_IMG;
   else if (image->IsA() == TFUShortImg::Class())
      {
      type = SHORT_IMG;
      ubzero = 32768;
      }
   else if (image->IsA() == TFIntImg::Class())
      type = LONG_IMG;
   else if (image->IsA() == TFUIntImg::Class())
      {
      type = LONG_IMG;
      ubzero = 2147483648u;
      }
   else if (image->IsA() == TFFloatImg::Class())
      type = FLOAT_IMG;
   else if (image->IsA() == TFDoubleImg::Class())
      type = DOUBLE_IMG;
   else
      {
      TFError::SetError("CreateFitsImage", "Unknown image class: %s", 
                        image->IsA()->GetName()); 
      return -1;   
      }

   UInt_t size[9];
   long   axis[9];

   int numDim = image->GetNumDim();
   if (numDim > 9)   numDim = 9;
   image->GetSize(size);
   for (int dim = 0; dim < numDim; dim++)
      axis[dim] = size[numDim - dim - 1];

   int status = 0;
   fits_create_img(fptr, type, numDim, axis, &status);

   if (bzero != 0)
      {
      fits_write_key(fptr, TINT, (char*)"BZERO", &bzero, 
                     (char*)"Make values Signed", &status);
      bzero = 1;
      fits_write_key(fptr, TINT, (char*)"BSCALE", &bzero, 
                     (char*)"Make values Signed", &status);
      }
   if (ubzero != 0)
      {
      fits_write_key(fptr, TUINT, (char*)"BZERO", &ubzero, 
                     (char*)"Make values Unsigned", &status);
      ubzero = 1;
      fits_write_key(fptr, TUINT, (char*)"BSCALE", &ubzero, 
                     (char*)"Make values Unsigned", &status);
      }


   fits_write_key(fptr, TSTRING, (char*)"EXTNAME", (void*)image->GetName(), 
                     (char*)"Extension name", &status);

   if (status != 0)
      TFError::SetError("CreateFitsImage", "Cannot create image in file %s. FITS error: %d",
                        fptr->Fptr->filename, status);

   return status;
}


//_____________________________________________________________________________
TFIOElement * MakeImage(fitsfile * fptr, int * status)
{
// Creates an TFImage and fills it with data from a FITS image.
// fptr may point to the primary header without image. In this case
// the function creates a TFIOElement. 
// In any case the name of the returned element are set to "no name" and
// should be overwritten if possible

   if (*status != 0) return NULL;

   int numDim;
   int dataType;
   long axisSize[9];   // the maximum number of dimension in a FITS file is 9

   // try to get number of dimension and the size of the image in each 
   // dimension
   *status = fits_get_img_param(fptr, 9, &dataType, &numDim, axisSize, status);

   if (*status != 0 || numDim == 0)
      {
      // this is proberly a primary header without data
      *status = 0;
      return new TFIOElement("no name");
      }

   TFIOElement * image = NULL;
   // we make a TFDoubleImg if keywords BSCALE and BZEOR is defined
   char strAttr[30];
   fits_read_keyword(fptr, (char*)"BZERO", strAttr, NULL, status);
   fits_read_keyword(fptr, (char*)"BSCALE", strAttr, NULL, status);
   if (*status == 0)
      {
      // the keywords exist, but we use TFDoublemg only if scale
      // it != 1.0
      double scale;
      sscanf(strAttr, "%lf", &scale);
      if (fabs(scale - 1) > 1E-10)
         return ReadDoubleImage(fptr, numDim, axisSize, status);
      }

   *status = 0;
   switch (dataType)
      {
      case BYTE_IMG:
         image = ReadByteImage(fptr, numDim, axisSize, status);
         break;

      case SHORT_IMG:
         image = ReadShortImage(fptr, numDim, axisSize, status);
         break;

      case LONG_IMG:
         image = ReadIntImage(fptr, numDim, axisSize, status);
         break;

      case FLOAT_IMG:
         image = ReadFloatImage(fptr, numDim, axisSize, status);
         break;

      case DOUBLE_IMG:
         image = ReadDoubleImage(fptr, numDim, axisSize, status);
         break;

      default:
         // should never happen
         TFError::SetError("MakeImage", "Unknown FITS image type: %d", dataType); 

         *status = -1;   
      }


   return image;
}
//_____________________________________________________________________________
static TFIOElement * ReadByteImage(fitsfile *fptr, int numDim, 
                                   long * fitsSize, int * status)
{
   if (*status != 0) return NULL;

   UInt_t tfSize[9];
   long   firstPixel[9];
   long   numPixel = 1;
   for (int dim = 0; dim < numDim; dim++)
      {
      numPixel *= fitsSize[dim];
      firstPixel[dim] = 1;
      tfSize[dim] = fitsSize[numDim - dim - 1];
      }


   char offset[30];
   fits_read_keyword(fptr, (char*)"BZERO", offset, NULL, status);

   if (*status != 0 || strcmp(offset, "-128") != 0)
      {
      // this is a unsigned char column
      *status = 0;

      // is there a NULL value defined?
      char           strNullVal[20];
      unsigned char  nullVal = 0;
      int            anyNull;

      fits_read_keyword(fptr, (char*)"BLANK", strNullVal, NULL, status);
      if (*status == 0)
         nullVal = (char)atoi(strNullVal);
      else
         *status = 0;

      TFUCharImg * image = new TFUCharImg("no name", numDim, tfSize);

      fits_read_pix(fptr, TBYTE, firstPixel, numPixel, &nullVal, 
                    image->GetDataArray(), &anyNull, status);

      if (*status != 0) 
         {
         TFError::SetError("ReadByteImage", errMsg, *status, fptr->Fptr->filename); 
         delete image;
         image = NULL;
         }
      else if (anyNull)
         image->SetNull(nullVal);

      return image;
      }
   else
      {
      // this is a sigend char column
      // is there a NULL value defined?
      char        strNullVal[20];
      short       nullVal = 0;
      int         anyNull;

      fits_read_keyword(fptr, (char*)"BLANK", strNullVal, NULL, status);
      if (*status == 0)
         nullVal = (short)atoi(strNullVal) - 128;
      else
         *status = 0;

      // we cannot read directly signed char. Therefore we read short
      short * buffer = new short[numPixel];
      TFCharImg * image = new TFCharImg("no name", numDim, tfSize);

      fits_read_pix(fptr, TSHORT, firstPixel, numPixel, &nullVal, 
                    buffer, &anyNull, status);

      if (*status != 0) 
         {
         TFError::SetError("ReadByteImage", errMsg, *status, fptr->Fptr->filename); 
         delete image;
         image = NULL;
         }
      else 
         {
         char * imgBuffer = image->GetDataArray();
         for (UInt_t pix = 0; pix < numPixel; pix++)
            imgBuffer[pix] = (char)buffer[pix];
            
         if (anyNull)
            image->SetNull((char)nullVal);
         }
      delete [] buffer;

      return image;
      }

}
//_____________________________________________________________________________
static TFIOElement * ReadShortImage(fitsfile *fptr, int numDim, 
                                   long * fitsSize, int * status)
{
   if (*status != 0) return NULL;

   UInt_t tfSize[9];
   long   firstPixel[9];
   long   numPixel = 1;
   for (int dim = 0; dim < numDim; dim++)
      {
      numPixel *= fitsSize[dim];
      firstPixel[dim] = 1;
      tfSize[dim] = fitsSize[numDim - dim - 1];
      }


   char offset[30];
   fits_read_keyword(fptr, (char*)"BZERO", offset, NULL, status);

   if (*status == 0 && strcmp(offset, "32768") == 0)
      {
      // this is a unsigned short column

      // is there a NULL value defined?
      char           strNullVal[20];
      unsigned short  nullVal = 0;
      int            anyNull;

      fits_read_keyword(fptr, (char*)"BLANK", strNullVal, NULL, status);
      if (*status == 0)
         nullVal = (unsigned short)(atoi(strNullVal) + 32768);
      else
         *status = 0;

      TFUShortImg * image = new TFUShortImg("no name", numDim, tfSize);

      fits_read_pix(fptr, TUSHORT, firstPixel, numPixel, &nullVal, 
                    image->GetDataArray(), &anyNull, status);

      if (*status != 0) 
         {
         TFError::SetError("ReadShortImage", errMsg, *status, fptr->Fptr->filename); 
         delete image;
         image = NULL;
         }
      else if (anyNull)
         image->SetNull(nullVal);

      return image;
      }
   else
      {
      // this is a sigend short column
      *status = 0;

      // is there a NULL value defined?
      char        strNullVal[20];
      short       nullVal = 0;
      int         anyNull;

      fits_read_keyword(fptr, (char*)"BLANK", strNullVal, NULL, status);
      if (*status == 0)
         nullVal = (short)atoi(strNullVal);
      else
         *status = 0;

      // we cannot read directly signed char. Therefore we read short
      TFShortImg * image = new TFShortImg("no name", numDim, tfSize);

      fits_read_pix(fptr, TSHORT, firstPixel, numPixel, &nullVal, 
                    image->GetDataArray(), &anyNull, status);

      if (*status != 0) 
         {
         TFError::SetError("ReadShortImage", errMsg, *status, fptr->Fptr->filename); 
         delete image;
         image = NULL;
         }
      else if (anyNull)
         image->SetNull(nullVal);

      return image;
      }

}
//_____________________________________________________________________________
static TFIOElement * ReadIntImage(fitsfile *fptr, int numDim, 
                                   long * fitsSize, int * status)
{
   if (*status != 0) return NULL;

   UInt_t tfSize[9];
   long   firstPixel[9];
   long   numPixel = 1;
   for (int dim = 0; dim < numDim; dim++)
      {
      numPixel *= fitsSize[dim];
      firstPixel[dim] = 1;
      tfSize[dim] = fitsSize[numDim - dim - 1];
      }


   char offset[30];
   fits_read_keyword(fptr, (char*)"BZERO", offset, NULL, status);

   if (*status == 0 && strcmp(offset, "2147483648") == 0)
      {
      // this is a unsigned integer column

      // is there a NULL value defined?
      char           strNullVal[20];
      unsigned int   nullVal = 0;
      int            anyNull;

      fits_read_keyword(fptr, (char*)"BLANK", strNullVal, NULL, status);
      if (*status == 0)
         nullVal = (unsigned int)( atoll(strNullVal) + 2147483648LL);
      else
         *status = 0;

      TFUIntImg * image = new TFUIntImg("no name", numDim, tfSize);

      fits_read_pix(fptr, TUINT, firstPixel, numPixel, &nullVal, 
                    image->GetDataArray(), &anyNull, status);

      if (*status != 0) 
         {
         TFError::SetError("ReadIntImage", errMsg, *status, fptr->Fptr->filename); 
         delete image;
         image = NULL;
         }
      else if (anyNull)
         image->SetNull(nullVal);

      return image;
      }
   else
      {
      // this is a sigend short column
      *status = 0;

      // is there a NULL value defined?
      char        strNullVal[20];
      int         nullVal = 0;
      int         anyNull;

      fits_read_keyword(fptr, (char*)"BLANK", strNullVal, NULL, status);
      if (*status == 0)
         nullVal = atoi(strNullVal);
      else
         *status = 0;

      // we cannot read directly signed char. Therefore we read short
      TFIntImg * image = new TFIntImg("no name", numDim, tfSize);

      fits_read_pix(fptr, TINT, firstPixel, numPixel, &nullVal, 
                    image->GetDataArray(), &anyNull, status);

      if (*status != 0) 
         {
         TFError::SetError("ReadIntImage", errMsg, *status, fptr->Fptr->filename); 
         delete image;
         image = NULL;
         }
      else if (anyNull)
         image->SetNull(nullVal);

      return image;
      }

}
//_____________________________________________________________________________
static TFIOElement * ReadFloatImage(fitsfile *fptr, int numDim, 
                                   long * fitsSize, int * status)
{
   if (*status != 0) return NULL;

   UInt_t tfSize[9];
   long   firstPixel[9];
   long   numPixel = 1;
   for (int dim = 0; dim < numDim; dim++)
      {
      numPixel *= fitsSize[dim];
      firstPixel[dim] = 1;
      tfSize[dim] = fitsSize[numDim - dim - 1];
      }

   float nullVal = FLT_MAX;
   int anyNull;

   TFFloatImg * image = new TFFloatImg("no name", numDim, tfSize);

   fits_read_pix(fptr, TFLOAT, firstPixel, numPixel, &nullVal, 
                  image->GetDataArray(), &anyNull, status);

   if (*status != 0) 
      {
      TFError::SetError("ReadFloatImage", errMsg, *status, fptr->Fptr->filename); 
      delete image;
      image = NULL;
      }
   else if (anyNull)
      image->SetNull(nullVal);

   return image;
}
//_____________________________________________________________________________
static TFIOElement * ReadDoubleImage(fitsfile *fptr, int numDim, 
                                   long * fitsSize, int * status)
{
   if (*status != 0) return NULL;

   UInt_t tfSize[9];
   long   firstPixel[9];
   long   numPixel = 1;
   for (int dim = 0; dim < numDim; dim++)
      {
      numPixel *= fitsSize[dim];
      firstPixel[dim] = 1;
      tfSize[dim] = fitsSize[numDim - dim - 1];
      }

   double nullVal = DBL_MAX;
   int anyNull;

   TFDoubleImg * image = new TFDoubleImg("no name", numDim, tfSize);

   fits_read_pix(fptr, TDOUBLE, firstPixel, numPixel, &nullVal, 
                  image->GetDataArray(), &anyNull, status);

   if (*status != 0) 
      {
      TFError::SetError("ReadDoubleImage", errMsg, *status, fptr->Fptr->filename); 
      delete image;
      image = NULL;
      }
   else if (anyNull)
      image->SetNull(nullVal);

   return image;
}
//_____________________________________________________________________________
template <class N, class I>
int WriteImage(fitsfile * fptr, I * image, int dataType, long * firstPixel,
               long numPixel, int status)
{
// writes the values into the FITS image and sets NULL values correctly if
// NULL values are defined in the image

   if (image->NullDefined())
      {
      N nullValue = image->GetNull();
      fits_update_key(fptr, dataType, (char*)"BLANK", &nullValue, 
                      (char*)"NULL value", &status);
      fits_write_pixnull(fptr, dataType, firstPixel, numPixel, 
                         image->GetDataArray(), &nullValue, &status);
      }
   else
      fits_write_pix(fptr, dataType, firstPixel, numPixel, 
                     image->GetDataArray(), &status);

   return status;
}
//_____________________________________________________________________________
int SaveImage(fitsfile* fptr, TFBaseImage* image)
{
// Save an image in a FITS file

   int status = 0;
   long   firstPixel[9];
   for (int dim = 0; dim < 9; dim++)
      firstPixel[dim] = 1;
   long numPixel = image->GetNumPixel();


   if (image->IsA() == TFBoolImg::Class())
      {
      unsigned char * buffer = new unsigned char[numPixel];
      bool * imgBuffer = ((TFBoolImg*)image)->GetDataArray();
      for (UInt_t pix = 0; pix < numPixel; pix++)
            buffer[pix] = imgBuffer[pix];
      fits_write_pix(fptr, TBYTE, firstPixel, numPixel, 
                     buffer, &status);
      delete [] buffer;
      }

   else if (image->IsA() == TFCharImg::Class())
      {
      short * buffer = new short[numPixel];
      char * imgBuffer = ((TFCharImg*)image)->GetDataArray();
      for (UInt_t pix = 0; pix < numPixel; pix++)
            buffer[pix] = imgBuffer[pix];

      if (((TFCharImg*)image)->NullDefined())
         {
         short nullValue = ((TFCharImg*)image)->GetNull();
         fits_update_key(fptr, TSHORT, (char*)"BLANK", &nullValue, 
                         (char*)"NULL value", &status);
         fits_write_pixnull(fptr, TSHORT, firstPixel, numPixel, 
                            buffer, &nullValue, &status);
         }
      else
         fits_write_pix(fptr, TSHORT, firstPixel, numPixel, 
                        buffer, &status);


      delete [] buffer;
      }

   else if (image->IsA() == TFUCharImg::Class())
      status = WriteImage<unsigned char>(fptr, (TFUCharImg*)image, TBYTE, firstPixel, 
                          numPixel, status);

   else if (image->IsA() == TFShortImg::Class())
      status = WriteImage<short>(fptr, (TFShortImg*)image, TSHORT, firstPixel, 
                          numPixel, status);

   else if (image->IsA() == TFUShortImg::Class())
      status = WriteImage<unsigned short>(fptr, (TFUShortImg*)image, TUSHORT, firstPixel, 
                          numPixel, status);

   else if (image->IsA() == TFIntImg::Class())
      status = WriteImage<int>(fptr, (TFIntImg*)image, TINT, firstPixel, 
                          numPixel, status);

   else if (image->IsA() == TFUIntImg::Class())
      status = WriteImage<unsigned int>(fptr, (TFUIntImg*)image, TUINT, firstPixel, 
                          numPixel, status);
 
   else if (image->IsA() == TFFloatImg::Class())
      {
      float nullval = ((TFFloatImg*)image)->GetNull();
      fits_write_pixnull(fptr, TFLOAT, firstPixel, numPixel, 
                     ((TFFloatImg*)image)->GetDataArray(), 
                     ((TFFloatImg*)image)->NullDefined() ? &nullval : NULL,
                     &status);
      }
 
   else if (image->IsA() == TFDoubleImg::Class())
      {
      double nullval = ((TFFloatImg*)image)->GetNull();
      fits_write_pixnull(fptr, TDOUBLE, firstPixel, numPixel, 
                     ((TFDoubleImg*)image)->GetDataArray(), 
                     ((TFDoubleImg*)image)->NullDefined() ? &nullval : NULL,
                     &status);
      }
 
   else
      {
      TFError::SetError("SaveImage", "Unknown image class: %s", 
                        image->IsA()->GetName()); 
      return -1;   
      }

   
   return status;
}

