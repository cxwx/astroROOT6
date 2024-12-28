// ///////////////////////////////////////////////////////////////////
//
//  File:      TFAsroIO.h
//
//  Version:   1.0
//
//  Author:    Reiner Rohlfs (GADC)
//
//  History:   1.0   15.07.03  first released version
//
// ///////////////////////////////////////////////////////////////////
#ifndef ROOT_TFAsroIO
#define ROOT_TFAsroIO

#include <map>

#ifndef ROOT_TFVirtualIO
#include "TFVirtualIO.h"
#endif

#ifndef ROOT_TFAsroFile
#include "TFAsroFile.h"
#endif

//_____________________________________________________________________________

class TFAsroFileItem
{
public:
   TFAsroFileItem()                      {fasroFile = NULL; fNumOpen = 0;}
   TFAsroFileItem(TFAsroFile * asroFile, Bool_t readOnly) 
                    {fasroFile = asroFile; fNumOpen = 1; fReadOnly = readOnly;}

   TFAsroFile *  fasroFile;    // low level ASRO file handler 
   Int_t         fNumOpen;     // number of open element in file
   Bool_t        fReadOnly;    // file is open as read only

   ClassDef(TFAsroFileItem,0) // one item in TFAsroFiles
};

//_____________________________________________________________________________

class TFAsroFiles
{
private:
   static std::map<Long_t, TFAsroFileItem> fFiles;   // all open ASRO files

protected:
   static TFAsroFile *  OpenFile(const char * fileName, Bool_t readOnly);
   static void          CloseFile(TFAsroFile * asroFile);

   ClassDef(TFAsroFiles,0) // static functions to open and close ASRO files
};

//_____________________________________________________________________________

class TFAsroIO : public TFVirtualIO, protected TFAsroFiles
{
protected:
   TFAsroFile     * fFile;       //! the file handler of this element
   Int_t          fCycle;        //! cycle number in file
   Int_t          fCompLevel;    //! compression level for this element

public:
   TFAsroIO();
   TFAsroIO(TFIOElement * element, const char * fileName);
   TFAsroIO(TFIOElement * element, TFAsroFile * file, Int_t cycle);
 
   ~TFAsroIO();

   static   TFIOElement *  TFRead(const char * fileName, const char * name,
                                  Int_t cycle = 0, FMode mode = kFRead,
                                  TClass * classType= NULL);

   virtual  Bool_t         IsOpen()      {return fFile != NULL && fFile->IsOpen(); }
   virtual  const char *   GetFileName() {return fFile == NULL ? NULL : fFile->GetFileName();}
   virtual  Int_t          GetCycle()    {return fCycle;}

   virtual  void           SetCompressionLevel(Int_t level) {fCompLevel = level;}
   virtual  Int_t          GetCompressionLevel()            {return fCompLevel;}

   virtual  void           CreateElement()  {}
   virtual  Int_t          DeleteElement();
   virtual  Int_t          SaveElement(Int_t compLevel = -1);

   // TFTable interface funmctions
   virtual  UInt_t         GetNumColumns();
   virtual  TFBaseCol *    ReadCol(const char * name);
   virtual  void           ReadAllCol(ColList & columns);
   virtual  Int_t          SaveColumns(ColList & columns, Int_t compLevel = -1);
   virtual  Int_t          DeleteColumn(const char * name);
   virtual  void           GetColNames(std::map<TString, TNamed> & columns);

private:

   ClassDef(TFAsroIO,0) // interface to ASRO files to store TFIOElements

};

//_____________________________________________________________________________

class TFAsroFileIter : public TFVirtualFileIter, protected TFAsroFiles
{
protected:
   TFAsroFile        * fFile;       //! the file of this element
   TFAsroElementIter * fAsroIter;   // the file iterator
   FMode             fMode;         // the file access mode kFRead or kFReadWrite 

public:
   TFAsroFileIter(const char * fileName, FMode mode = kFRead);
   ~TFAsroFileIter();

   virtual  Bool_t        IsOpen() {return fFile != NULL && fAsroIter != NULL;}
   virtual  Bool_t        Next();
   virtual  void          Reset();
  
   ClassDef(TFAsroFileIter,0) // an iterator for all elements of one Asro file
};


#endif // ROOT_TFAsroIO
