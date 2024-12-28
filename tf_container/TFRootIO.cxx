// ///////////////////////////////////////////////////////////////////
//
//  File:      TFFitsIO.cxx
//
//  Version:   1.1.1
//
//  Author:    Reiner Rohlfs (GADC)
//
//  History:   1.0   18.08.03  first released version
//             1.1.1 01.07.04  The ROOT file can be opened in read 
//                             only mode.
//
// ///////////////////////////////////////////////////////////////////
#include "TSystem.h"
#include "TFile.h"
#include "TKey.h"
#include "TClass.h"


#include "TFRootIO.h"
#include "TFIOElement.h"
#include "TFTable.h"
#include "TFColumn.h"
#include "TFError.h"

#define MAX_UNIQUE_NAMES    0x7fffffff

ClassImp(TFRootFileItem)
ClassImp(TFRootFiles)
ClassImp(TFRootIO)
ClassImp(TFRootFileIter)


std::map<Long_t, TFRootFileItem> TFRootFiles::fFiles;   


static const char * errMsg[] = {
"Cannot create / open the file %s.",
"There are already %d elements with the same name (%s) in the file %s."
   "Cannot write next element in same file.",
"The File %s does not exist (Open error).",
"The IOElement %s does not exist in file %s.",
"Cannot open file %s",
"Tried to close file %s more often than to open it"
};


//_____________________________________________________________________________
// An internal class not designed to be used directly by an application or 
// in an interactive session.

//_____________________________________________________________________________
TFile * TFRootFiles::OpenFile(const char * fileName, FMode mode)
{
   Long_t id;

   if (gSystem->GetPathInfo(fileName, &id, (Long_t*)NULL, NULL, NULL) == 1)
      {
      // file properly does not exist yet
      TFile * file = new TFile(fileName, "NEW");
      gSystem->GetPathInfo(fileName, &id, (Long_t*)NULL, NULL, NULL);
      fFiles[id] = TFRootFileItem(file);
      return file;
      }
   else
      {
      TFRootFileItem & fItem = fFiles[id];
      fItem.fNumOpen++;
      if (!fItem.fFile)
         fItem.fFile = new TFile(fileName, mode == kFRead ? "READ" : "UPDATE");
      return fItem.fFile;
      }

   return NULL;
}

//_____________________________________________________________________________
void TFRootFiles::CloseFile(TFile * file)
{
   if (file == NULL)
      return;

   for (std::map<Long_t, TFRootFileItem>::iterator i_f = fFiles.begin();
        i_f != fFiles.end(); i_f++)
      if (i_f->second.fFile == file)
         {
         i_f->second.fNumOpen -= 1;
         if (i_f->second.fNumOpen == 0)
            {
            delete i_f->second.fFile;
            fFiles.erase(i_f);
            }
         return;
         }

   // this line should never be reached
   TFError::SetError("TFRootFiles::CloseFile", errMsg[5], file->GetName()); 

}

//_____________________________________________________________________________
static TDirectory * GetFreeDir(TFile * file, const char * name, 
                               Int_t & cycle)
{
// Creates a new directory in file with name name_cycle
// NULL is returned if already MAX_UNIQUE_NAMES subdirectories for 
// the same name exist in the file 
   char subDir[100];

   TList * keys = file->GetListOfKeys();
   for (cycle = 1; cycle < MAX_UNIQUE_NAMES; cycle++)
      {
      sprintf(subDir, "%s_%d", name, cycle);
      if (keys->FindObject(subDir) == NULL)
         return file->mkdir(subDir);
      }

   return NULL;
}

//_____________________________________________________________________________
static Bool_t DirExist(TFile * file, const char * name, Int_t cycle)
{
// Checks if the subdirectory name_cycle exist and changes to
// this directory. Returns kFALSE if this failed.
   char subDir[100];
   sprintf(subDir, "%s_%d", name, cycle);

   TList * keys = file->GetListOfKeys();

   TKey * key = (TKey*)keys->FindObject(subDir);
   if (key && strcmp(key->GetClassName(), "TDirectory") == 0  &&
       file->cd(subDir) )
      return kTRUE;

   return kFALSE;
}

//_____________________________________________________________________________
TFIOElement * TFRootIO::TFRead(const char * name, const char * fileName, 
                               FMode mode, TClass * classType,
                               Int_t cycle)
{
   TFile * file;
   TFIOElement * element;
   EAccessMode accessMode;

   accessMode = mode == kFRead ? kReadPermission : 
                    static_cast<EAccessMode>(kWritePermission | kReadPermission);
   if (gSystem->AccessPathName(fileName,  accessMode) )
      {
      TFError::SetError("TFIOElement::RWMode", errMsg[2], fileName); 
      return NULL;
      }

   file = OpenFile(fileName, mode);

   if (file == NULL || !(file->IsOpen()) || file->IsZombie())
      {
      // the file exist, but nevertheless there is an error
      TFError::SetError("TFIOElement::RWMode", errMsg[4], fileName); 
      CloseFile(file);
      return NULL;
      }

   TDirectory * tmpDir = gDirectory;

   if (cycle > 0)
      {
      // check if the subdirectory for this element exist
      if (DirExist(file, name, cycle) )
         {
         // we found this element in the file, read it
         element = (TFIOElement*)gDirectory->Get(name);
         if (element && (classType == NULL || element->IsA() == classType))
            {
            // the element in the file is the required class
            element->SetIO(new TFRootIO(element, file, gDirectory, cycle));
            gDirectory = tmpDir;
            return element;
            }
         delete element;
         }
      }
   else
      {
      // look for this element with any cycle number 
      TIter nextKey(file->GetListOfKeys());
      size_t nameLength = strlen(name);
      while (TKey * key = (TKey*)nextKey())
         {
         if (strncmp(name, key->GetName(), nameLength) ||
             *(key->GetName() + nameLength) != '_'     ||
             strcmp(key->GetClassName(), "TDirectory")    )
            continue;

         if (file->cd(key->GetName()) )
            {
            element = (TFIOElement*)gDirectory->Get(name);
            if (element && (classType == NULL || element->IsA()->InheritsFrom(classType)))
               {
               cycle = atoi(key->GetName() + nameLength + 1);
               element->SetIO(new TFRootIO(element, file, gDirectory, cycle));
               gDirectory = tmpDir;
               return element;
               }
            delete element;
            file->cd();
            }
         }
      }

   TFError::SetError("TFIOElement::RWMode", errMsg[3], name, fileName); 

   gDirectory = tmpDir;
   CloseFile(file);

   return NULL;
}

//_____________________________________________________________________________
//_____________________________________________________________________________
TFRootIO::TFRootIO() 
{
  fFile      = NULL;
  fDir       = NULL;
  fCycle     = 0;
  fCompLevel = 1;
}

//_____________________________________________________________________________
TFRootIO::TFRootIO(TFIOElement * element, TFile * file, TDirectory * dir, Int_t cycle)
   : TFVirtualIO(element)
{
   fFile      = file;
   fDir       = dir;
   fCycle     = cycle;
   fCompLevel = 1;
}   

//_____________________________________________________________________________
TFRootIO::TFRootIO( TFIOElement * element, const char * fileName)
  : TFVirtualIO(element)
{
   fFile      = NULL;
   fDir       = NULL;
   fCompLevel = 1;
   fCycle     = 1; 

   fFile = OpenFile(fileName, kFReadWrite);

   if (fFile == NULL || !(fFile->IsOpen()) || fFile->IsZombie())
      {
      // cannot create / open  the file
      TFError::SetError("TFIOElement::NewFile", errMsg[0], fileName); 
      CloseFile(fFile);
      fFile = NULL;
      return;
      }

   // look for a not used subdirectory
   fDir = GetFreeDir(fFile, fElement->GetName(), fCycle);
   if (fDir == NULL)
      {
      // there is no not used subdirectory
      TFError::SetError("TFIOElement::NewFile", errMsg[1], 
                        MAX_UNIQUE_NAMES, fElement->GetName(), fileName); 
      CloseFile(fFile);
      fFile = NULL;
      }
}
 
//_____________________________________________________________________________
TFRootIO::~TFRootIO() 
{
   CloseFile(fFile);
}

//_____________________________________________________________________________
Int_t TFRootIO::DeleteElement()
{
   if (!fFile)
      return -1;

   fDir->Delete("T*;*");
   char subDir[100];
   sprintf(subDir, "%s_%d;*", fElement->GetName(), fCycle);
   fFile->Delete(subDir);

   if (fFile->GetListOfKeys()->GetSize() == 0)
      {
      // there are no more elements in this file. We delete it
      char fileName[512];
      strcpy(fileName, fFile->GetName());

      CloseFile(fFile);

      fFile  = NULL;
      fDir   = NULL;
      fCycle = 0;

      gSystem->Unlink(fileName);
      }

   return 0;
}

//_____________________________________________________________________________
Int_t TFRootIO::SaveElement(Int_t compLevel)
{
   if (!fFile)
      return 0;

   TDirectory * tmpDir = gDirectory;

   if (fDir->cd())
      {
      fFile->SetCompressionLevel(compLevel >= 0 ? compLevel : fCompLevel);
      fElement->Write(fElement->GetName(), TObject::kOverwrite);
      fFile->cd();
      fFile->Write();
      }
   gDirectory = tmpDir;

   return 0;
}

//_____________________________________________________________________________
Int_t TFRootIO::DeleteColumn(const char * name)
{
   Int_t rc = -1;

   if (fFile && fFile->IsOpen())
      {
      TDirectory * tmpDir = gDirectory;
      if (fDir->cd("columns") &&
          gDirectory->GetListOfKeys()->FindObject(name) )
         {
         char str[100];
         sprintf(str, "%s;*", name);
         gDirectory->Delete(str);
         rc = 0;
         }
      gDirectory = tmpDir;
      }

   return rc;
}

//_____________________________________________________________________________
UInt_t TFRootIO::GetNumColumns()
{
   UInt_t num = 0;
   if (fFile)
      {
      TDirectory * tmpDir = gDirectory;
      if (fDir->cd("columns"))
         num = gDirectory->GetListOfKeys()->GetSize();
      gDirectory = tmpDir;
      }

   return num;
}

//_____________________________________________________________________________
TFBaseCol * TFRootIO::ReadCol(const char * name)
{
   TFBaseCol * col = NULL;

   if (fFile)
      {
      TDirectory * tmpDir = gDirectory;
      if (fDir->cd("columns"))
         col = (TFBaseCol *)gDirectory->Get(name);
      gDirectory = tmpDir;
      }
   return col;
}

//_____________________________________________________________________________
void TFRootIO::ReadAllCol(ColList & columns)
{
   if (fFile)
      {
      TDirectory * tmpDir = gDirectory;
      if (fDir->cd("columns"))
         {
         TKey * key;
         TIter next(gDirectory->GetListOfKeys());
         while (key = (TKey *)next())
            {
            if (columns.find(TFColWrapper(*key)) == columns.end())
               columns.insert(TFColWrapper((TFBaseCol&)*gDirectory->Get(key->GetName())));
            }
         }
      gDirectory = tmpDir;
      }
}

//_____________________________________________________________________________
Int_t TFRootIO::SaveColumns(ColList & columns, Int_t compLevel)
{
   if (!fFile)
      return 0;

   TDirectory * tmpDir = gDirectory;

   if (fDir->cd())
      {
      if (!fDir->cd("columns"))
         fDir->mkdir("columns")->cd();

      // write all columns into the "columns" directory
      for (I_ColList i_c = columns.begin(); i_c != columns.end(); i_c++)
         i_c->GetCol().Write(i_c->GetCol().GetName(), TObject::kOverwrite);

      fFile->cd();
      fFile->Write();
      }
   gDirectory = tmpDir;

   return 0;
}

//_____________________________________________________________________________
void TFRootIO::GetColNames(std::map<TString, TNamed> & columns)
{
}


//_____________________________________________________________________________
//_____________________________________________________________________________
TFRootFileIter::TFRootFileIter(const char * fileName, FMode mode)
   : TFVirtualFileIter(fileName)
{
   fFile      = NULL;
   fKeyIter   = NULL;
   fMode      = mode;

   EAccessMode accessMode;
   accessMode = mode == kFRead ? kReadPermission : 
                    static_cast<EAccessMode>(kWritePermission | kReadPermission);
   if (gSystem->AccessPathName(fileName,  accessMode) )
      {
      TFError::SetError("TFRootFileIter::TFRootFileIter", errMsg[2], fileName); 
      return;
      }

   fFile = OpenFile(fileName, mode);

   if (fFile == NULL || !(fFile->IsOpen()) || fFile->IsZombie())
      {
      // the file exist, but nevertheless there is an error
      TFError::SetError("TFRootFileIter::TFRootFileIter", errMsg[4], fileName); 
      CloseFile(fFile);
      fFile = NULL;
      return;
      }


   fKeyIter = new TIter(fFile->GetListOfKeys());
}

//_____________________________________________________________________________
TFRootFileIter::~TFRootFileIter()
{
   CloseFile(fFile);
   delete fKeyIter;
}

//_____________________________________________________________________________
Bool_t TFRootFileIter::Next()
{
   delete fElement;
   fElement = NULL;

   if (fKeyIter == NULL || fFile == NULL)
      return kFALSE;
   
   TFIOElement * element;

   while (TKey * key = (TKey*)(*fKeyIter)())
      {
      if (strcmp(key->GetClassName(), "TDirectory"))
         continue;

      if (fFile->cd(key->GetName()) )
         {
         char name[100];
         strcpy(name, key->GetName());
         char * pos = strrchr(name, '_');
         if (pos)
            {
            *pos = 0;
            element = (TFIOElement*)gDirectory->Get(name);
            if (element)
               {
               // OK, we have the next element
               Int_t cycle = atoi(pos + 1);
               if (cycle > 0)
                  {
                  // open the same file again for the new element
                  TFile * fl = OpenFile(fFileName.Data(), fMode);
                  fElement = element;
                  element->SetIO(new TFRootIO(element, fl, gDirectory, cycle));
                  return kTRUE;
                  }
               delete element;
               }
            }
         fFile->cd();
         }
      }

   return kFALSE;
}

//_____________________________________________________________________________
void TFRootFileIter::Reset()
{
   if (fKeyIter)
      fKeyIter->Reset();
}

