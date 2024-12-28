// ///////////////////////////////////////////////////////////////////
//
//  File:      TFFitsIO.cxx
//
//  Version:   1.0
//
//  Author:    Reiner Rohlfs (GADC)
//
//  History:   1.0   13.08.03  first released version
//
// ///////////////////////////////////////////////////////////////////
#ifdef _SOLARIS
#include <ieeefp.h>
#endif

#include "TFFitsIO.h"
#include "TFError.h"
#include "TFIOElement.h"
#include "TFGroup.h"
#include "TFImage.h"

#include "fitsio.h"

#ifndef TF_CLASS_IMP
#define TF_CLASS_IMP
ClassImp(TFFitsIO)
ClassImp(TFFitsFileIter)
#endif

//_____________________________________________________________________________
// TFFitsIO and TFFitsFileIter
//    These classes are internal classes and should not by used directly 
//    by an application or in an interactive session.
//    These classes are used by TFIOElement to read and store data into
//    FITS files.

static const char * errMsg[] = {
"Cannot open the file %s. FITS error: %d",
"Cannot write/update keyword %s of %s in file %s. FITS error: %d",
"HDU %s does not exist in file %s",
"HDU number %d does not exist in file %s",
"HDU number %d in file %s has name %s, but expecting name %s",
"Found %s in file %s, but expected a %s",
"Can neither create nor open the file %s",
"Neither name nor HDU number is defined. Cannot open an element in file %s"
};

static void HeaderFits2Root(fitsfile * fptr, TFIOElement * element, int * status);
static void HeaderRoot2Fits(TFIOElement * element, fitsfile * fptr, int * status);
       TFIOElement * MakeTable(fitsfile * fptr, int * status);
       int           CreateFitsTable(fitsfile* fptr, TFTable * table);
       int           SaveTable(fitsfile* fptr, TFTable* table);

       TFIOElement * MakeImage(fitsfile * fptr, int * status);
       int           CreateFitsImage(fitsfile* fptr, TFBaseImage* image);
       int           SaveImage(fitsfile* fptr, TFBaseImage* image);


//_____________________________________________________________________________
TFFitsIO::TFFitsIO( TFIOElement * element, const char * fileName)
  : TFVirtualIO(element)
{
// opens a file or creates a new file if the file does not exist and creates
// a new HDU in the FITS file.


   int status = 0;
// tries to create a FITS file. It will fail if it already exist
   fits_create_file((fitsfile**)&fFptr, fileName, &status);

   // try to open a file if the creation failed
   if (status != 0)
      {
      status = 0;
      fits_open_file((fitsfile**)&fFptr, fileName, READWRITE, &status);
      if (status != 0)
         {
         TFError::SetError("TFFitsIO::TFFitsIO", errMsg[6], fileName); 
         fFptr = NULL;
         }
      }
   else
      {
      // create a primary array, which we will not use
      long   axis[1] = {0};
      fits_create_img((fitsfile*)fFptr, SHORT_IMG, 0, axis, &status);

      }
}
//_____________________________________________________________________________
TFFitsIO::TFFitsIO( TFIOElement * element, void * fptr, int cycle)
  : TFVirtualIO(element)
{
// constructor

   fFptr  = fptr;
   fCycle = cycle;
}

TFFitsIO::~TFFitsIO()
{
// destructor

   int status = 0;
   if (fFptr)
      fits_close_file((fitsfile*)fFptr, &status);
}
//_____________________________________________________________________________
TFIOElement * TFFitsIO::TFRead(const char * fileName, const char * name,
                               Int_t cycle, FMode mode, TClass * classType)
{
// tries to open an element in a FITS file and converts it into TFIOElement
// or one of its derived classes.
// In case there is a problem an Error is written to TFError and NULL is 
// returned.

   // open the fits file
   int status = 0;
   fitsfile * fptr;
   fits_open_file(&fptr, fileName, mode == kFRead ? READONLY : READWRITE, 
                  &status);
   if (status != 0)   
      {
      TFError::SetError("TFFitsIO::TFRead", errMsg[0], fileName, status); 
      return NULL;
      }

   // move to the required HDU
   if (cycle <= 0)
      {
      if (name == NULL || name[0] == 0)
         {
         TFError::SetError("TFFitsIO::TFRead", errMsg[7], fileName); 
         return NULL;
         }

      // move by name
      char * tmpName = new char[strlen(name) + 1];
      strcpy(tmpName, name);

      fits_movnam_hdu(fptr, ANY_HDU, tmpName, -cycle, &status);
      delete [] tmpName;

      if (status != 0)   
         {
         status = 0;
         // try GROUPING
         char gr[10];
         strcpy(gr, "GROUPING");
         fits_movnam_hdu(fptr, ANY_HDU, gr, 0, &status);
         if ( status != 0 ) 
            {
            TFError::SetError("TFFitsIO::TFRead", errMsg[2], name, fileName); 
            fits_close_file(fptr, &status);
            return NULL;
            }
         }
   
      // get the current hdu number = cycle
      int hduNum;
      fits_get_hdu_num(fptr, &hduNum);
      cycle = hduNum;
      }
   else
      {
      // move to HDU number (the correct name will be tested later)
      fits_movabs_hdu(fptr, cycle, NULL, &status);
      if (status != 0)   
         {
         TFError::SetError("TFFitsIO::TFRead", errMsg[3], cycle, fileName); 
         status = 0;
         fits_close_file(fptr, &status);
         return NULL;
         }
      }

   // get the type of element
   TFIOElement * element;
   char elementType[20];
   char keyname[20];
   strcpy(keyname, "XTENSION");
   fits_read_keyword(fptr, keyname, elementType, NULL, &status);
   if (status == 0 && strcmp(elementType , "'BINTABLE'") == 0)
      {
      element = MakeTable(fptr, &status);
      }
   else if ( (status == 0 && strstr(elementType , "IMAGE") != NULL ) ||
               status == 202  /* XTENSION keyword does not exist */      )
      {
      status = 0;
      element = MakeImage(fptr, &status);
      }
   else
      {
      element = new TFIOElement("no name");
      status = 0;
      }
   
   // check if the type of element is as it should be
   if (element && status == 0 && classType)
      {
      if (element->IsA() != classType)
         if (classType != TFBaseImage::Class() || 
             !element->IsA()->InheritsFrom(classType))
            {
            TFError::SetError("TFFitsIO::TFRead", errMsg[5], 
                              element->ClassName(), fileName,
                              classType->GetName()); 
            delete element;
            fits_close_file(fptr, &status);
            return NULL;
            }
      }

   // copy the header and set the element name
   HeaderFits2Root(fptr, element, &status);

   // check if the name of the elemement is as it should be
   if (element && status == 0 && name && name[0] != 0 )
      {
      char elementName[110];
      strcpy(elementName, element->GetName());
      // remove the hdu numer at the end of the name
      char * pos = strrchr(elementName, '_');
      if (pos) *pos = 0;
      if (strcmp(elementName, name) != 0)
         {
         // we accept if a TFGroup expects as name GROUPING
         if (!(strcmp(name, "GROUPING") == 0 && 
               element->IsA()->InheritsFrom(TFGroup::Class()) ) )
            {
            TFError::SetError("TFFitsIO::TFRead", errMsg[4], cycle, fileName,
                              element->GetName(), name); 
            delete element;
            fits_close_file(fptr, &status);
            return NULL;
            }
         }
      }

   if (element)
      {
      element->SetIO(new TFFitsIO(element, fptr, cycle));
      element->SetFileAccess(mode);
      }
   return element;

}
//_____________________________________________________________________________
Bool_t TFFitsIO::IsOpen()
{
// return kTRUE if an element is successfull open

  return fFptr != NULL;
}
//_____________________________________________________________________________
const char * TFFitsIO::GetFileName()
{
// returns the filename of an element. 
// returns NULL if the element is not open or in case of a cfitsio error

   if (fFptr == NULL)
      return NULL;

   int status = 0;
   static char fileName[512];
   fits_file_name((fitsfile*)fFptr, fileName, &status);
   return status == 0 ?  fileName : NULL;
}
//_____________________________________________________________________________
Int_t TFFitsIO::GetCycle()
{
// returns the cycle number == HDU number. 
// First element in FITS file (primary header) has cycle 1

  return fCycle;
}
//_____________________________________________________________________________
void TFFitsIO::CreateElement()
{
// creates a new element
   if (fFptr == NULL)
      return;

   fitsfile * fptr = (fitsfile*)fFptr;
   int status = 0;

   if (fElement->IsA()->InheritsFrom(TFBaseImage::Class()))
      status = CreateFitsImage(fptr, (TFBaseImage*)fElement);

   if (fElement->IsA()->InheritsFrom(TFTable::Class()))
      status = CreateFitsTable(fptr, (TFTable*)fElement);

   if (status == 0)
      {         
      // get the current hdu number = cycle
      int hduNum;
      fits_get_hdu_num(fptr, &hduNum);
      fCycle = (UInt_t)hduNum;
      }   
   else
      {
      status = 0;
      fits_close_file(fptr, &status);
      fFptr = NULL;
      }

}
//_____________________________________________________________________________
Int_t TFFitsIO::DeleteElement()
{
// deletes this HDU and deletes the file if if is the last HDU
// Note: cfitsio will replace the first HDU == primary array by
// an empty one if this primary array should be deleted.

   if (fFptr == NULL)
      return 0;

   fitsfile * fptr = (fitsfile*)fFptr;
   int status = 0;
   
   int numHdus;
   fits_get_num_hdus(fptr, &numHdus, &status);
   if (status != 0)  return -1;
   
   if (numHdus == 1)
      {
      // There is only one HDU left, delete the file
      fits_delete_file(fptr, &status);
      }
   else
      {
      fits_delete_hdu(fptr, NULL, &status);
      // fptr points now to an other HDU which we dont want.
      // We close it.
      fits_close_file(fptr, &status);
      }   
   fFptr == NULL;

   return 0;
}
//_____________________________________________________________________________
Int_t TFFitsIO::SaveElement(Int_t compLevel)
{
// saves the current element into the FITS file

   if (fFptr == NULL)
      return 0;

   fitsfile * fptr = (fitsfile*)fFptr;
   int status = 0;

   if (fElement->IsA()->InheritsFrom(TFBaseImage::Class()))
      status = SaveImage(fptr, (TFBaseImage*)fElement);

   else if (fElement->IsA()->InheritsFrom(TFTable::Class()))
      status = SaveTable(fptr, (TFTable*)fElement);

   HeaderRoot2Fits(fElement, fptr, &status);

   fits_write_chksum(fptr, &status);
//   if (status == 232)
//      status = 0;   // this happens with a new table

   return status == 0 ? 0 : -1;
}
//_____________________________________________________________________________
//_____________________________________________________________________________
TFFitsFileIter::TFFitsFileIter(const char * fileName, FMode mode)
   : TFVirtualFileIter(fileName)
{
// constructor 

   int status = 0;
   fStatus = 0;
   fCycle  = 0;
   fMode   = mode;
   
   fitsfile * fptr;
   fits_open_file(&fptr, fileName, mode == kFRead ? READONLY : READWRITE, 
                  &status);
   if (status == 0)
      fFptr = fptr;
   else
      {
      fFptr = NULL;
      TFError::SetError("TFFitsFileIter::TFFitsFileIter", errMsg[0], 
                        fileName, status); 
      }
}
//_____________________________________________________________________________
TFFitsFileIter::~TFFitsFileIter()
{
// destructor

   int status = 0;
   if (fFptr)
      fits_close_file((fitsfile*)fFptr, &status);
}
//_____________________________________________________________________________
Bool_t TFFitsFileIter::Next()
{
// closes the previous element and opens the next in the FITS file.
// return kFALSE if there is no further HDU in the file.

   delete fElement;
   fElement = NULL;

   if (fFptr == NULL || fStatus != 0)
      return kFALSE;

     
   fitsfile * fptr = (fitsfile*)fFptr;

   int status = 0;

   // get the type of element
   char elementType[20];
   char keyname[40];
   strcpy(keyname, "XTENSION");
   fits_read_keyword(fptr, keyname, elementType, NULL, &status);
   if (status == 0 && strcmp(elementType , "'BINTABLE'") == 0)
      {
      fElement = MakeTable(fptr, &status);
      }
   else if ( (status == 0 && strstr(elementType , "IMAGE") != NULL ) ||
               status == 202  /* XTENSION keyword does not exist */      )
      {
      status = 0;
      fElement = MakeImage(fptr, &status);
      }
   else
      {
      fElement = new TFIOElement("no name");
      status = 0;
      }

   if (fElement == NULL)
      return kFALSE;
   
   // open the element again for the new element
   fitsfile * fptr2;
   char filename[512];
   sprintf(filename, "%s[%d]", fFileName.Data(), fCycle);
   fits_open_file(&fptr2, filename, fMode == kFRead ? READONLY : READWRITE,
                  &status);
   
   fCycle++;
   fElement->SetIO(new TFFitsIO(fElement, fptr2, fCycle));

   HeaderFits2Root(fptr, fElement, &status);

   fits_movrel_hdu(fptr, 1, NULL, &fStatus);

   return kTRUE;
}
//_____________________________________________________________________________
void TFFitsFileIter::Reset()
{
// Reset the iterator.

   if (fFptr == NULL)
      return;

   fCycle = 0;
   fStatus = 0;
   fits_movabs_hdu((fitsfile*)fFptr, 1, NULL, &fStatus);
}
//_____________________________________________________________________________
static void HeaderFits2Root(fitsfile * fptr, TFIOElement * element, int * status)
{
   if (*status != 0)  return;

   char name[30], value[100], comment[100], unit[40];

   int nkeys;
   fits_get_hdrspace(fptr, &nkeys, NULL, status);
   for (int num = 0; num < nkeys && *status == 0; num++)
      {
      fits_read_keyn(fptr, num + 1, name, value, comment, status);
      if (strncmp(name, "TTYPE", 5) == 0  || 
          strncmp(name, "TFORM", 5) == 0  || 
          strncmp(name, "TNULL", 5) == 0  ||
          strncmp(name, "TZERO", 5) == 0  ||
          strncmp(name, "TSCAL", 5) == 0  ||
          strncmp(name, "NAXIS", 5) == 0  ||
          strcmp (name, "XTENSION") == 0  ||
          strcmp (name, "TFIELDS") == 0   ||
          strcmp (name, "BITPIX") == 0    ||
          strcmp (name, "PCOUNT") == 0    ||
          strcmp (name, "GCOUNT") == 0    ||
          strcmp (name, "BLANK") == 0     ||
          strcmp (name, "BSCALE") == 0    ||
          strcmp (name, "BZERO") == 0     ||
          strcmp (name, "CHECKSUM") == 0  ||
          strcmp (name, "DATASUM") == 0      )
         continue;

      fits_read_key_unit(fptr, name, unit, status);

      if (value[0] == '\'' && value[strlen(value) - 1] == '\'')
         {
         int len = strlen(value) - 2;
         while (value[len] == ' ') len--;
         value[len+1] = 0;
         if (strcmp("EXTNAME", name) != 0 && strcmp("GRPNAME", name) != 0)
            element->AddAttribute(TFStringAttr(name, TString(value+1), unit, comment), kFALSE);

         // set the element name
         if ( (strcmp("EXTNAME", name) == 0 && strcmp("GROUPING", value+1) != 0)  || // not a group table
              (strcmp("GRPNAME", name) == 0)  )                                      // a group table
            {
            int hduNum = 0;
            fits_get_hdu_num(fptr, &hduNum);
            char elementName[110];
            sprintf(elementName, "%s_%d", value+1, hduNum);
            element->SetName(elementName);
            }
         }
      else if (strcmp(value, "T") == 0)
         element->AddAttribute(TFBoolAttr(name, kTRUE, unit, comment), kFALSE);
      else if (strcmp(value, "F") == 0)
         element->AddAttribute(TFBoolAttr(name, kFALSE, unit, comment), kFALSE);
      else if (strlen(value) < 9 && value[0] != 0 && strchr(value, '.') == NULL)
         element->AddAttribute(TFIntAttr(name, atoi(value), unit, comment), kFALSE);
      else if (value[0] != 0)
         {
         Double_t val;
         sscanf(value, "%lg", &val);
         element->AddAttribute(TFDoubleAttr(name, val, unit, comment), kFALSE);
         }
      else
         element->AddAttribute(TFStringAttr(name, TString(value), unit, comment), kFALSE);
      }

}
//_____________________________________________________________________________
static void HeaderRoot2Fits(TFIOElement * element, fitsfile * fptr, int * status)
{
   if (*status != 0)
      return;

   TFAttrIter i_attr = element->MakeAttrIterator();
   while (i_attr.Next())
      {
      char * comment = (char*)i_attr->GetComment();
      if (comment[0] == 0) comment = NULL;

      if (i_attr->IsA() == TFBoolAttr::Class())
         {
         int val = (int)((TFBoolAttr&)(*i_attr)).GetValue();
         fits_update_key(fptr, TLOGICAL, (char*)i_attr->GetName(), &val, 
                         comment, status);
         }

      else if (i_attr->IsA() == TFIntAttr::Class())
         {
         int val = ((TFIntAttr&)(*i_attr)).GetValue();
         fits_update_key(fptr, TINT, (char*)i_attr->GetName(), &val, 
                         comment, status);
         }

      else if (i_attr->IsA() == TFUIntAttr::Class())
         {
         unsigned int val = ((TFUIntAttr&)(*i_attr)).GetValue();
         fits_update_key(fptr, TUINT, (char*)i_attr->GetName(), &val, 
                         comment, status);
         }

      else if (i_attr->IsA() == TFDoubleAttr::Class())
         {
         double val = ((TFDoubleAttr&)(*i_attr)).GetValue();
         fits_update_key(fptr, TDOUBLE, (char*)i_attr->GetName(), &val, 
                         comment, status);
         }

      else if (i_attr->IsA() == TFStringAttr::Class())
         {
         const char * val = ((TFStringAttr&)(*i_attr)).GetValue().Data();
         fits_update_key(fptr, TSTRING, (char*)i_attr->GetName(), (void*)val, 
                         comment, status);
         }

      if (i_attr->GetUnit()[0] != 0)
         fits_write_key_unit(fptr, (char*)i_attr->GetName(), 
                             (char*)i_attr->GetUnit(), status);

      if (*status != 0)
         {
         TFError::SetError("HeaderRoot2Fits", errMsg[1], i_attr->GetName(),
                           element->GetName(), fptr->Fptr->filename, *status); 
         *status = 0;
         }
      }

}

// this unused function ensures that the cfitsio function ffgiwcs and ffgtwcs are linked
// into the libastro.so library.
static void never_used()
{
   int status;
   fitsfile * fptr = NULL;
   char * header;

   ffgiwcs(fptr, &header, &status);
   ffgtwcs(fptr, 1, 2, &header, &status);
}
