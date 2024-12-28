// ////////////////////////////////////////////////////////////////////////////
//
//  File:      TFAsroIO.cxx
//
//  Version:   1.0
//
//  Author:    Reiner Rohlfs (GADC)
//
//  History:   1.0   16.07.03  first released version
//
// ////////////////////////////////////////////////////////////////////////////
#include "TSystem.h"
#include "TClass.h"


#include "TFAsroIO.h"
#include "TFIOElement.h"
#include "TFTable.h"
#include "TFColumn.h"
#include "TFError.h"

// should be the same value as in TFAsrofile.cxx file
#define MAX_UNIQUE_NAMES    0x7fffffff

#ifndef TF_CLASS_IMP
#define TF_CLASS_IMP
ClassImp(TFAsroFileItem)
ClassImp(TFAsroFiles)
ClassImp(TFAsroIO)
ClassImp(TFAsroFileIter)
#endif    // TF_CLASS_IMP


std::map<Long_t, TFAsroFileItem> TFAsroFiles::fFiles;   


static const char * errMsg[] = {
"Cannot create / open the file %s.",
"There are already %d elements with the same name (%s) in the file %s."
   "Cannot write next element in same file.",
"The File %s does not exist (Open error).",
"The IOElement %s does not exist in file %s.",
"Cannot open file %s",
"Tried to close the file %s more often than to open it",
"Cannot delete element %s in file %s",
"Cannot save/update element %s in file %s",
"Cannot read column %s of table %s in file %s",
"Cannot save/update columns of table %s in file %s",
"Cannot delete column %s of table %s in file %s"
};

//_____________________________________________________________________________
// TFAsroIO, TFAsroFileIter, TFAsroFileItem and TFAsroFiles are internal 
// classes. 
// Theys should not be used directly by an applications or in an 
// interactive session!


//_____________________________________________________________________________
TFAsroFile * TFAsroFiles::OpenFile(const char * fileName, Bool_t readOnly)
{
// opens or creates an ASRO file and returns the file handler.
// If the file is already open for an other or the same element the
// fNumOpen counter is increamented and the file handler is returned.

   Long_t id;

   if (gSystem->GetPathInfo(fileName, &id, (Long_t*)NULL, NULL, NULL) == 1)
      {
      // file properly does not exist yet
      if (readOnly)
         return NULL;

      TFAsroFile * asroFile = new TFAsroFile(fileName, &readOnly);
      if (asroFile->IsOpen())
         {
         gSystem->GetPathInfo(fileName, &id, (Long_t*)NULL, NULL, NULL);
         fFiles[id] = TFAsroFileItem(asroFile, readOnly);
         return asroFile;
         }
      else
         delete asroFile;
      }
   else
      {
      TFAsroFileItem & fItem = fFiles[id];
      fItem.fNumOpen++;
      if (fItem.fasroFile == NULL)
         {
         fItem.fasroFile = new TFAsroFile(fileName, &readOnly);
         if (fItem.fasroFile->IsOpen())
            {
            // the file is successful open
            fItem.fReadOnly = readOnly;
            return fItem.fasroFile;
            }
         else
            // we are not able to open the file as needed
            CloseFile(fItem.fasroFile);
         }
      else if (!readOnly && fItem.fReadOnly)
         {
         // the file is already as read only open, but this time we
         // need read and write permission
         CloseFile(fItem.fasroFile);
         }
      else
         // it is already open. Either we don't need read write or it
         // is a read and write open
         return fItem.fasroFile;
         
      }

   return NULL;
}
//_____________________________________________________________________________
void TFAsroFiles::CloseFile(TFAsroFile * asroFile)
{
// decrements the fNumOpen counter and closes the file if the counter is 0

   if (asroFile == NULL)
      return;

   for (std::map<Long_t, TFAsroFileItem>::iterator i_f = fFiles.begin();
        i_f != fFiles.end(); i_f++)
      if (i_f->second.fasroFile == asroFile)
         {
         i_f->second.fNumOpen -= 1;
         if (i_f->second.fNumOpen == 0)
            {
            delete asroFile;
            fFiles.erase(i_f);
            }
         return;
         }

   // this line should never be executed
   TFError::SetError("TFAsroFiles::CloseFile", errMsg[5], asroFile->GetFileName()); 

}
//_____________________________________________________________________________
//_____________________________________________________________________________
TFIOElement * TFAsroIO::TFRead(const char * fileName, const char * name,
                               Int_t cycle, FMode mode, TClass * classType)
{
// Opens and reads one element in an ASRO file.

   TFAsroFile * file;
   TFIOElement * element;
   EAccessMode accessMode;

   accessMode = mode == kFRead ? kReadPermission : 
                    static_cast<EAccessMode>(kWritePermission | kReadPermission);
   if (gSystem->AccessPathName(fileName,  accessMode) )
      {
      TFError::SetError("TFAsroIO::TFRead", errMsg[2], fileName); 
      return NULL;
      }

   file = OpenFile(fileName, mode == kFRead);

   if (file == NULL)
      {
      // the file exist, but nevertheless there is an error
      TFError::SetError("TFAsroIO::TFRead", errMsg[4], fileName); 
      CloseFile(file);
      return NULL;
      }

   if (cycle == 0)
      cycle = file->GetNextCycle(name, 0);

   // try to read the requested element
   element = (TFIOElement*)file->Read(name, "", cycle);
   if (element && (classType == NULL || element->IsA() == classType))
      {
      // the element in the file is the required class
      element->SetIO(new TFAsroIO(element, file, cycle));
      element->SetFileAccess(mode);
      return element;
      }
   delete element;

   TFError::SetError("TFAsroIO::TFRead", errMsg[3], name, fileName); 

   CloseFile(file);

   return NULL;
}
//_____________________________________________________________________________
TFAsroIO::TFAsroIO() 
{
  fFile      = NULL;
  fCycle     = 0;
  fCompLevel = 1;
}
//_____________________________________________________________________________
TFAsroIO::TFAsroIO(TFIOElement * element, TFAsroFile * file, Int_t cycle)
   : TFVirtualIO(element)
{
   fFile      = file;
   fCycle     = cycle;
   fCompLevel = 1;
}   
//_____________________________________________________________________________
TFAsroIO::TFAsroIO( TFIOElement * element, const char * fileName)
  : TFVirtualIO(element)
{
// opens a file or creates a new file if the file does not exist and creates
// a new element in the ASRO file.

   fFile      = NULL;
   fCompLevel = 1;
   fCycle     = 0; 

   fFile = OpenFile(fileName, kFALSE);

   if (fFile == NULL)
      {
      // cannot create / open  the file
      TFError::SetError("TFAsroIO::TFAsroIO", errMsg[0], fileName); 
      CloseFile(fFile);
      fFile = NULL;
      return;
      }

   // look for a not used cycle
   fCycle = fFile->GetFreeCycle(element->GetName());
   if (fCycle == 0)
      {
      // there is no free cycle any more
      TFError::SetError("TFAsroIO::TFAsroIO", errMsg[1], 
                        MAX_UNIQUE_NAMES, fElement->GetName(), fileName); 
      CloseFile(fFile);
      fFile = NULL;
      }
}
//_____________________________________________________________________________
TFAsroIO::~TFAsroIO() 
{
   CloseFile(fFile);
}
//_____________________________________________________________________________
Int_t TFAsroIO::DeleteElement()
{
   if (fFile == NULL)
      return 0;

   if (fFile->Delete(fElement->GetName(), "", fCycle) == false)
      {
      TFError::SetError("TFAsroIO::DeleteElement", errMsg[6], 
                        fElement->GetName(), GetFileName() ); 
      return -1;
      }

   if (fFile->GetNumItems() == 0)
      {
      // there are no more elements in this file. We delete it
      char fileName[512];
      strcpy(fileName, fFile->GetFileName());

      CloseFile(fFile);

      fFile  = NULL;
      fCycle = 0;

      gSystem->Unlink(fileName);
      }

   return 0;
}
//_____________________________________________________________________________
Int_t TFAsroIO::SaveElement(Int_t compLevel)
{
   if (fFile == NULL)
      return 0;
 
   if (compLevel < 0)
      compLevel = fCompLevel;

   bool ok = true;  
   ok &= fFile->InitWrite();
   ok &= fFile->Write(fElement, compLevel, fElement->GetName(), "", fCycle);
   ok &= fFile->FinishWrite();

   if (ok)  return 0;
   
   TFError::SetError("TFAsroIO::SaveElement", errMsg[7], 
                     fElement->GetName(), GetFileName() ); 
   return -1;
}
//_____________________________________________________________________________
UInt_t TFAsroIO::GetNumColumns()
{
   if (fFile)
      return fFile->GetNumSubs(fElement->GetName(), fCycle);

   return 0;
}
//_____________________________________________________________________________
TFBaseCol * TFAsroIO::ReadCol(const char * name)
{
   TFBaseCol * col = NULL;

   if (fFile)
      col = (TFBaseCol*)fFile->Read(fElement->GetName(), name, fCycle);

   return col;
}
//_____________________________________________________________________________
void TFAsroIO::ReadAllCol(ColList & columns)
{
   if (fFile)
      {
      TFAsroColIter * i_col = fFile->MakeColIter(fElement->GetName(), fCycle);
      while(i_col->Next())
         {
         const char * colName = i_col->GetColName();
         TNamed name(colName, "");
         if (columns.find(TFColWrapper(name)) == columns.end())
            {
            TFBaseCol * col = (TFBaseCol*)fFile->Read(fElement->GetName(), colName, fCycle);
            if (col)
               columns.insert(TFColWrapper(*col));
            }
         }
      delete i_col;
      }
}
//_____________________________________________________________________________
Int_t TFAsroIO::SaveColumns(ColList & columns, Int_t compLevel)
{
   if (fFile == NULL)
      return 0;

   if (compLevel < 0)
      compLevel = fCompLevel;

   bool ok = fFile->InitWrite();
   const char * elName = fElement->GetName();
   for (I_ColList i_c = columns.begin(); i_c != columns.end(); i_c++)
      ok &= fFile->Write(&(i_c->GetCol()), compLevel,
                   elName, i_c->GetCol().GetName(), fCycle);
   ok &= fFile->FinishWrite();

   if (ok)  return 0;
   
   TFError::SetError("TFAsroIO::SaveColumns", errMsg[9], 
                     fElement->GetName(), GetFileName() ); 
   return -1;
}
//_____________________________________________________________________________
Int_t TFAsroIO::DeleteColumn(const char * name)
{
   if (fFile == NULL)
      return 0;

   if (fFile->Delete(fElement->GetName(), name, fCycle) == 0)
      return 0;

   TFError::SetError("TFAsroIO::DeleteColumn", errMsg[10], 
                     name, fElement->GetName(), GetFileName() ); 
   return -1;
}
//_____________________________________________________________________________
void TFAsroIO::GetColNames(std::map<TString, TNamed> & columns)
{
   if (fFile)
      {
      TFAsroColIter * i_col = fFile->MakeColIter(fElement->GetName(), fCycle);
      while(i_col->Next())
         {
         const char * className = i_col->GetClassName();

         // get the data type of one element
         TClass cl(className);
         TFBaseCol * col = (TFBaseCol*)cl.New();

         columns[i_col->GetColName()] = TNamed(className, col->GetTypeName() );
         delete col;
         }
      delete i_col;
      }
}
//_____________________________________________________________________________
//_____________________________________________________________________________
TFAsroFileIter::TFAsroFileIter(const char * fileName, FMode mode)
   : TFVirtualFileIter(fileName)
{
   fFile      = NULL;
   fAsroIter  = NULL;
   fMode      = mode;

   EAccessMode accessMode;
   accessMode = mode == kFRead ? kReadPermission : 
                    static_cast<EAccessMode>(kWritePermission | kReadPermission);
   if (gSystem->AccessPathName(fileName,  accessMode) )
      {
      TFError::SetError("TFAsroFileIter::TFAsroFileIter", errMsg[2], fileName); 
      return;
      }

   fFile = OpenFile(fileName, mode == kFRead);

   if (fFile == NULL)
      {
      // the file exist, but nevertheless there is an error
      TFError::SetError("TFAsroFileIter::TFAsroFileIter", errMsg[4], fileName); 
      CloseFile(fFile);
      return;
      }
   
   fAsroIter = fFile->MakeElementIter();
}
//_____________________________________________________________________________
TFAsroFileIter::~TFAsroFileIter()
{
   CloseFile(fFile);
   delete fAsroIter;
}
//_____________________________________________________________________________
Bool_t TFAsroFileIter::Next()
{
   delete fElement;
   fElement = NULL;

   if (!fAsroIter || !fFile)
      return kFALSE;

   if (fAsroIter->Next())
      {
      fElement = (TFIOElement*)fFile->Read(fAsroIter->GetKey());
      if (fElement)
         {
         // open the same file again for the new element
         TFAsroFile * fl = OpenFile(fFileName.Data(), fMode == kFRead);
         fElement->SetIO(new TFAsroIO(fElement, fl, 
                                     fAsroIter->GetKey().GetCycle()));
         fElement->SetFileAccess(fMode);
         return kTRUE;
         }
      }   

   return kFALSE;
}
//_____________________________________________________________________________
void TFAsroFileIter::Reset()
{
   if (fAsroIter)
      fAsroIter->Reset();
}
      

