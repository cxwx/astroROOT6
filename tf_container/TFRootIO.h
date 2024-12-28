// ///////////////////////////////////////////////////////////////////
//
//  File:      TFRootIO.h
//
//  Version:   1.0
//
//  Author:    Reiner Rohlfs (GADC)
//
//  History:   1.0   18.08.03  first released version
//
// ///////////////////////////////////////////////////////////////////
#ifndef ROOT_TFRootIO
#define ROOT_TFRootIO

#include <map>

#ifndef ROOT_TFVirtualIO
#include "TFVirtualIO.h"
#endif

#ifndef ROOT_TFile
#include "TFile.h"
#endif


//_____________________________________________________________________________

class TFRootFileItem
{
public:
   TFRootFileItem() {fFile = NULL; fNumOpen = 0;}
   TFRootFileItem(TFile * file) {fFile = file; fNumOpen = 1;}

   TFile * fFile;
   Int_t  fNumOpen;

   ClassDef(TFRootFileItem,0) // one item in TFROOTFiles
};

//_____________________________________________________________________________

class TFRootFiles
{
private:
   static std::map<Long_t, TFRootFileItem> fFiles;   

protected:
   static TFile * OpenFile(const char * fileName, FMode mode = kFRead);
   static void CloseFile(TFile * file);

   ClassDef(TFRootFiles,0) // static functions to open and close ROOT files
};

//_____________________________________________________________________________

class TFRootIO : public TFVirtualIO, protected TFRootFiles
{
protected:
   TFile       * fFile;       //! the file of this element
   TDirectory  * fDir;        //! the subdirectory in fFile of this element
   Int_t       fCycle;        //! cycle number in file
   Int_t       fCompLevel;    //! compression level for this element

public:
   TFRootIO();
   TFRootIO(TFIOElement * element, const char * fileName);
   TFRootIO(TFIOElement * element, TFile * file, TDirectory * dir, Int_t cycle);
 
   ~TFRootIO();

   static   TFIOElement *  TFRead(const char * name, const char * fileName,
                                  FMode mode = kFRead, TClass * classType= NULL, 
                                  Int_t cycle = 0);

   virtual  Bool_t         IsOpen()      {return fFile != NULL;}
   virtual  const char *   GetFileName() {return fFile ? fFile->GetName() : NULL;}
   virtual  Int_t          GetCycle()    {return fCycle;}

   virtual  void           SetCompressionLevel(Int_t level) {fCompLevel = level;}
   virtual  Int_t          GetCompressionLevel()            {return fCompLevel;}

   virtual  void           CreateElement() {}
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

   ClassDef(TFRootIO,0) //interface to ROOT files to store TFIOElements

};

//_____________________________________________________________________________

class TFRootFileIter : public TFVirtualFileIter, protected TFRootFiles
{
protected:
   TFile       * fFile;       //! the file of this element
   TIter       * fKeyIter;    //! an iterator of element - directories
   FMode       fMode;         //! the open mode of the file

public:
   TFRootFileIter(const char * fileName, FMode mode = kFRead);
   ~TFRootFileIter();

   virtual  Bool_t        IsOpen()  {return fFile != NULL && fKeyIter != NULL;}
   virtual  Bool_t        Next();
   virtual  void          Reset();
      
  
   ClassDef(TFRootFileIter,0) // an iterator for all elements of one ROOT file
};


#endif // ROOT_TFRootIO
