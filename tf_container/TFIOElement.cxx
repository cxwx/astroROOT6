// ///////////////////////////////////////////////////////////////////
//
//  File:      TFIOElement.cpp
//
//  Version:   1.0
//
//  Author:    Reiner Rohlfs
//
//  History:
//
// ///////////////////////////////////////////////////////////////////
#include <fcntl.h>
#include <unistd.h>

#include "TClass.h"

#include "TFIOElement.h"
#include "TFRootIO.h"
#include "TFAsroIO.h"
#include "TFFitsIO.h"
#include "TFError.h"

ClassImp(TFIOElement)
ClassImp(TFFileIter)


static const char * errmsg_io[] = {
"Cannot write element %s to file %s opened as readonly.",
"Cannot delete element %s to file %s opened as readonly."
};


//_____________________________________________________________________________
// TFIOElement:
//    The TFIOElement class is the base class to save data structures into a 
//    ASRO, a ROOT or a FITS file. The file extension define the used file type:
//    *.fits, *.fts, *fit and *.fits.gz define a FITS file.
//    *.root define a ROOT file and 
//    *.asro define a ASRO file. 
//    If the filename has none of the above file extensions .asro is 
//    automatically added to the file name and a ASRO file is opened or created.
//
//    The TFIOElement class does not have data itself, but a header to store 
//    header information and always a name. Most of the time derived classes 
//    are used to write data into a file and read data from a file. For example 
//    tables or images.
//
//    To create a new element either in memory or in a ASRO in a ROOT or in a
//    FITS file the constructors can be used. To create an element in memory 
//    that reads its data and header from a file the global function
//    TFRead has to be used. Beside TFIOElements this function can read 
//    also any derived class from TFIOElement.
//
//    All changes of a TFIOElement or its derived classes are done only in 
//    memory and not automatically updated in the associated file. To 
//    save the changes the member function SaveElement() has to be called.
//
//
// TFFileIter:
//    The TFFileIter can be used to iterate through all extensions in a 
//    ASRO file, in a ROOT file and in the FITS file. 



//_____________________________________________________________________________
static int FileType(TString & flName, bool create)
{
// depending on the extension of the file name this function determins the
// file type:
//    *.fits, *.fts, *fit and *.fits.gz is a FITS file.  return value: 1
//    *.root is a ROOT file                              return value: 0
//    *.asro is a ASRO file                              return value: 2
//  else:
//    if create == true assume it is an FITS file        return value: 1
//    else
//      try to read the first 4 bytes:
//        if file starts with "root"  it is a ROOT file  return value: 0
//        if file starts with "ASRO" it is an ASRO file  return value: 2
//        else  assume it an FITS file                   return value: 1
//

   if ( flName.EndsWith(".asro") ) 
      return 2;

   if ( flName.EndsWith(".root") ) 
      return 0;
   
   if ( flName.EndsWith(".fits")    ||
        flName.EndsWith(".fts")     ||
        flName.EndsWith(".fit")     ||
        flName.EndsWith(".fits.gz")    )
      return 1;

   if (create)
   // everything else is a FITS file
      return 1;

   // try to open the file and read the first 4 bytes
   int fl = open(flName.Data(), O_RDONLY);
   if (fl >= 0)
      {
      char id[4];
      int numRead = read(fl, id, 4);
      close(fl);
      if (numRead == 4)
         {
         if (strncmp("root", id, 4) == 0)
            return 0;
         if (strncmp("ASRO", id, 4) == 0)
            return 2;
         }
      }

   // everything else is a FITS file
   return 1;
}

TFIOElement *  TFRead(const char * fileName, const char * name,  
                      UInt_t cycle, FMode mode, TClass * classType)
{
// This global functions reads a TFIOElement or a derived object from a 
// ASRO file, a ROOT file or a FITS file, depending on the file extension 
// and returns its pointer. The returned element has to be deleted by the 
// calling function.
//
// fileName: defines the file. Absolute path, relative path or just a 
//       filename are accepted names.
//       If mode == kFRead the file has to have read permission. If mode ==
//       kFReadWrite it also has to have write permission.
// name: Is the identifier of the element. If more than one element with
//       the same name exist in the file the first is returned if 
//       cycle is set to the default value of 0. Otherwise the element with
//       the specified cycle number is looked for in the file.
// cycle: If it is not the default value of 0 only the element with the 
//       specified  cycle number will be read.
// mode: has to be either kFRead or kFReadWrite. 
//       kfRead: The file and the required element must already exist. The
//               element cannot be updated.
//       kfReadWrite: The file and the required element must already exist.
//               The element can be updated with the member function 
//               SaveElement()
// classType: If this parameter is defined (not NULL) only elements with the 
//       specified classType are returned. Use the static member function
//       Class() to pass the required value.
//
// If there is an error during opening or reading the file or if the required
// element is not in the file the functions will throw a TFFileException or 
// will return a NULL pointer.
//
   TString flName = fileName;

   int fileType = FileType(flName, false);

   if (fileType == 0)
      return TFRootIO::TFRead(name, flName, mode, classType, cycle);     
   else if (fileType == 1)
      return TFFitsIO::TFRead(flName, name, cycle, mode, classType);
   else 
      return TFAsroIO::TFRead(flName, name, cycle, mode, classType);

}

//_____________________________________________________________________________
TFFileIter::TFFileIter(const char * fileName, FMode mode)
{
// File iterator to open all extension in a file, one after the other. 
// As soon as the next extension is opened with the Next() function the
// previous element is closed and cannot be accessed any more.

   TString flName = fileName;
   int fileType = FileType(flName, false);

   if (fileType == 0)
      fIter = new TFRootFileIter(flName, mode);     
   else if (fileType == 1)
      fIter = new TFFitsFileIter(flName, mode);
   else if (fileType == 2)
      fIter = new TFAsroFileIter(flName, mode);
   else
      fIter = NULL; 

}

//_____________________________________________________________________________
TFIOElement::TFIOElement()
{
// Default constructor. The element is not associated with a element 
// in a file.
// Don't use this constructor. The element should have at least a name.

   fio            = NULL;
   fFileAccess    = kFUndefined;
}
//_____________________________________________________________________________
TFIOElement::TFIOElement(const char * name) 
        : TNamed(name, "") 
{
// The element is not associated with an element in a file.
// It exist only in memory. name is the id of the new element.

   fio            = NULL;
   fFileAccess    = kFUndefined;
}
//_____________________________________________________________________________
TFIOElement::TFIOElement(const char * name, const char * fileName) 
        : TNamed(name, "") 
{
// A new element is created. It is associated with an element in a 
// file. name is the id of the new element. 
// fileName is the file name of the ASRO, ROOT or FITS file with this new 
// element. The file  may already exist or will be created. Other objects 
// with the same name can exist without problem in the same file. Each object 
// with the same name has a unique cycle number. Use GetCycle() to ask for 
// this cycle number. Use the function TFRead() to read and update an already 
// existing element from a ASRO, a ROOT or a FITS file.

   NewFile(fileName);
}
//_____________________________________________________________________________
TFIOElement::TFIOElement(const TFIOElement & ioelement)
      : TNamed(ioelement), TFHeader(ioelement) 
{
// Copy constructor. But even if the input element is associated with an 
// element in a file, this new table is NOT associated with this or 
// an other element in a file. It will exist only in memory, but can be
// saved in a new file using the SaveElement() method. 

   fio            = NULL;
   fFileAccess    = kFUndefined;
}
//_____________________________________________________________________________
TFIOElement & TFIOElement::operator = (const TFIOElement & ioelement)
{  
// Even if the input element is associated with an element
// in a  file, this element is NOT associated with this or an other
// element in a file. If this element was associates with an element 
// in a file before this function the file is closed without update.

   if (this != &ioelement)
      {
      TNamed::operator=(ioelement);
      TFHeader::operator =(ioelement);

      // close the old file
      CloseElement();

      fio      = NULL;
      }

   return *this;
}
//_____________________________________________________________________________
bool TFIOElement::operator == (const TFHeader & ioelement) const
{  
// Two elements are identical if their names are identical and their header 
// are identical. The order of the attributes in the header may be different 
// and the elements are still considered to be identical.

   return IsA() == ioelement.IsA()                 &&
          fName == ((TFIOElement&)ioelement).fName &&
          TFHeader::operator==(ioelement);
}
//_____________________________________________________________________________
TFIOElement::~TFIOElement()
{
// The associated file is closed, the element is NOT updated. Use
// SaveElement() to update the changes in the file.

   CloseElement();
}
//_____________________________________________________________________________
void TFIOElement::NewFile(const char * fileName)
{
// protected function to create a new element in a file 

   TString flName = fileName;

   int fileType = FileType(flName, true);

   if (fileType == 0)
      fio = new TFRootIO(this, flName);     
   else if (fileType == 1)
      fio = new TFFitsIO(this, flName);
   else if (fileType == 2)
      fio = new TFAsroIO(this, flName);

   if (!fio->IsOpen())
      CloseElement();
   else
      fFileAccess    = kFReadWrite;

}
//_____________________________________________________________________________
Int_t TFIOElement::SaveElement(const char * fileName, Int_t compLevel)
{
// Updates the element in the file with the data of this element in
// memory. If fileName is defined the old file is closed and the element
// is written into this new file named fileName.
// compLevel defines the compression level in the ASRO or ROOT file. To set
// compLevel and to update the element in the same file set fileName 
// to an empty string "".
// This function without any parameter has to be used to update the
// file with any change of the element in memory.

   if (fileName && fileName[0] != 0)
      {
      // read everything from the old file and close it
      UpdateMemory();
      CloseElement();

      NewFile(fileName);

      if (!fio || !fio->IsOpen())
         return -1;

      fio->CreateElement();
      }

   if (!fio || !fio->IsOpen())
      return 0;

   Int_t err = 0;
   if (fFileAccess == kFRead)
      {
      TFError::SetError("TFIOElement::SaveElement", errmsg_io[0], 
                        GetName(), fio->GetFileName());
      return -1;
      }

   else if (fFileAccess == kFReadWrite)
      err = fio->SaveElement(compLevel);

   return err;
}
//_____________________________________________________________________________
void TFIOElement::CloseElement()
{
// Closes the element in the associated file. It does not update the
// element in the file. The element still exist in memory after
// a call of this function.

   delete fio;
   fio    = NULL;

   fFileAccess    = kFUndefined;
}
//_____________________________________________________________________________
Int_t TFIOElement::DeleteElement(Bool_t updateMemory)
{
// Deletes the element in the file, but not in memory.
// Closes the file and deletes the file if this element is the
// last element in the file.
// Updated the element in memory before the file is deleted if
// updateMemory is set to kTRUE. The default is kFALSE.

   if (fio == NULL)
      return 0;

   if (updateMemory)
     UpdateMemory();

   Int_t err = 0;
   if (fFileAccess == kFRead)
      {
      TFError::SetError("TFIOElement::DeleteElement", errmsg_io[1], 
                        GetName(), fio->GetFileName());
      return -1;
      }
   else if (fFileAccess == kFReadWrite)
      err = fio->DeleteElement();

   CloseElement();

   return err;
}
//_____________________________________________________________________________
void TFIOElement::Print(const Option_t* option) const
{
//  Prints Class name, name of object and file name and cycle number 
//  of object if if is connected to a file.
//  Calls Print function of TFHeader to print the header attributes

   printf("Container type: %s         Name: %s\n", 
          IsA()->GetName(), GetName());
   if (IsFileConnected())
      printf("File name:      %s Cycle Number: %u\n", 
             GetFileName(), GetCycle());

   TFHeader::PrintH(option);
}

