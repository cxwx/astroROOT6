// ////////////////////////////////////////////////////////////////////////////
//
//  File:      TFFitsIO.h
//
//  Version:   1.0
//
//  Author:    Reiner Rohlfs (GADC)
//
//  History:   1.0   13.08.03  first released version
//
// ////////////////////////////////////////////////////////////////////////////
#ifndef ROOT_TFFitsIO
#define ROOT_TFFitsIO


#ifndef ROOT_TFVirtualIO
#include "TFVirtualIO.h"
#endif


class TFFitsIO : public TFVirtualIO
{
protected:
   void  * fFptr;
   int   fCycle;

public:
   TFFitsIO() {}
   TFFitsIO( TFIOElement * element, const char * fileName);
   TFFitsIO( TFIOElement * element, void * fptr, int cycle);
 
   ~TFFitsIO();

   static   TFIOElement *  TFRead(const char * fileName, const char * name, 
                                  Int_t cycle = 0, FMode mode = kFRead, 
                                  TClass * classType = NULL);

   virtual  Bool_t         IsOpen();
   virtual  const char *   GetFileName();
   virtual  Int_t          GetCycle();

   virtual  void           SetCompressionLevel(Int_t level) {}
   virtual  Int_t          GetCompressionLevel() {return 0;} 

   virtual  void           CreateElement();
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

   ClassDef(TFFitsIO,0) //interface to FITS files to store TFIOElements

};

//_____________________________________________________________________________

class TFFitsFileIter : public TFVirtualFileIter
{
protected:
   void  * fFptr;
   int   fStatus;
   int   fCycle;
   FMode fMode;

public:
   TFFitsFileIter(const char * fileName, FMode mode = kFRead);
   ~TFFitsFileIter();

   virtual  Bool_t        IsOpen()  {return fFptr != NULL;}
   virtual  Bool_t        Next();
   virtual  void          Reset();
      
  
   ClassDef(TFFitsFileIter,0) // an iterator for all HDUs of one FITS file
};


#endif // ROOT_TFVirtualIO
