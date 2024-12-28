// ///////////////////////////////////////////////////////////////////
//
//  File:      TFVirtualIO.h
//
//  Version:   1.0
//
//  Author:    Reiner Rohlfs (GADC)
//
//  History:   1.0   15.07.03  first released version
//
// ///////////////////////////////////////////////////////////////////
#ifndef ROOT_TFVirtualIO
#define ROOT_TFVirtualIO

#ifndef ROOT_RTypes
#include "Rtypes.h"
#endif

#ifndef ROOT_TString
#include "TString.h"
#endif

#ifndef ROOT_TNamed
#include "TNamed.h"
#endif

#ifndef ROOT_TFColWrapper
#include "TFColWrapper.h"
#endif

#include <map>

class TFIOElement;
class TFBaseCol;

enum  FMode    {kFUndefined = 0, kFRead = 1, kFReadWrite = 2};

//_____________________________________________________________________________

class TFVirtualIO
{
protected:
  TFIOElement * fElement;  // the element associated with this IOfile

public:
   TFVirtualIO() {}
   TFVirtualIO( TFIOElement * element) {fElement = element;}
 
   virtual ~TFVirtualIO() {}

   virtual  Bool_t         IsOpen() = 0;
   virtual  const char *   GetFileName() = 0;
   virtual  Int_t          GetCycle() = 0;

   virtual  void           SetCompressionLevel(Int_t level) = 0;
   virtual  Int_t          GetCompressionLevel() = 0; 

   virtual  void           CreateElement() = 0;
   virtual  Int_t          DeleteElement() = 0;
   virtual  Int_t          SaveElement(Int_t compLevel = -1) = 0;

   // TFTable interface funmctions
   virtual  UInt_t         GetNumColumns() = 0;
   virtual  TFBaseCol *    ReadCol(const char * name) = 0;
   virtual  void           ReadAllCol(ColList & columns) = 0;
   virtual  Int_t          SaveColumns(ColList & columns, Int_t compLevel = -1) = 0;
   virtual  Int_t          DeleteColumn(const char * name) = 0;
   virtual  void           GetColNames(std::map<TString, TNamed> & columns) = 0;

   ClassDef(TFVirtualIO,0) // interface definition to files storing TFIOElements
};

//_____________________________________________________________________________

class TFVirtualFileIter
{
protected:
   TString     fFileName;   // the filename of this iterator
   TFIOElement * fElement;  // the element associated with this Iterator


public:
   TFVirtualFileIter()                    {fElement = NULL;}
   TFVirtualFileIter(const char * fileName) : fFileName(fileName) 
                                          {fElement = NULL;}
   virtual ~TFVirtualFileIter();

   virtual  Bool_t        IsOpen() = 0;
   virtual  Bool_t        Next() = 0;
   virtual  void          Reset() = 0;
   virtual  TFIOElement & operator * ()   {return *fElement;}
   virtual  TFIOElement * operator -> ()  {return fElement;}


   ClassDef(TFVirtualFileIter,0) // interface definition for file iterator

};

#endif 
