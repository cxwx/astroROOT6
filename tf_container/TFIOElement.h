/////////////////////////////////////////////////////////////////////
//
//  File:      TFIOElement.h
//
//  Version:   1.0
//
//  Author:    Reiner Rohlfs
//
//  History:
//
/////////////////////////////////////////////////////////////////////
#ifndef ROOT_TFIOElement
#define ROOT_TFIOElement


#ifndef ROOT_TNamed
#include "TNamed.h"
#endif

#ifndef ROOT_TFHeader
#include "TFHeader.h"
#endif

#ifndef ROOT_TFVirtualIO
#include "TFVirtualIO.h"
#endif


//_____________________________________________________________________________

class TFIOElement : public TNamed, public TFHeader
{
protected:
   TFVirtualIO  * fio;        //!interface to the file
   FMode        fFileAccess;  //!access to the file read, readwrite

public:
   TFIOElement();
   TFIOElement(const TFIOElement & ioelement);
   TFIOElement(const char * name);

   TFIOElement(const char * name,   const char * fileName);

   virtual ~TFIOElement();

   virtual  TFIOElement &  operator = (const TFIOElement & ioelement);
   virtual  bool           operator == (const TFHeader & ioelement) const;

            void           SetIO(TFVirtualIO * io)   {fio = io;}
            void           SetFileAccess(FMode mode) {fFileAccess = mode;}
                                
 
   virtual  Bool_t         IsFileConnected() const {return fio != NULL && fio->IsOpen();}
   virtual  const char *   GetFileName() const     {return fio ? fio->GetFileName() : NULL;}
   virtual  Int_t          GetCycle() const        {return fio ? fio->GetCycle() : 0;}


   virtual  void           SetCompressionLevel(Int_t level)
                                 {if (fio) fio->SetCompressionLevel(level);}
   virtual  Int_t          GetCompressionLevel() 
                                 {return fio ? fio->GetCompressionLevel() : 0;}
   virtual  void           CloseElement();
   virtual  Int_t          SaveElement(const char * fileName = NULL, Int_t compLevel = -1);
   virtual  Int_t          DeleteElement(Bool_t updateMemory = kFALSE);
   virtual  void           Print(const Option_t* option = "") const;

protected:
   virtual  void           UpdateMemory() {}

   virtual  void           NewFile(const char * fileName);

   ClassDef(TFIOElement,1) // Element with header and no data, can be saved into a file

};

//_____________________________________________________________________________

class TFFileIter
{
   TFVirtualFileIter  * fIter;   //!interface to the file

public:
   TFFileIter(const char * fileName, FMode mode = kFRead);
   ~TFFileIter() {delete fIter;}

   Bool_t        IsFileConnected() const {return fIter != NULL && fIter->IsOpen();}
   Bool_t        Next()             {return fIter->Next();}
   void          Reset()            {fIter->Reset();}
   TFIOElement & operator * ()      {return fIter->operator*();}
   TFIOElement * operator -> ()     {return fIter->operator->();}


   ClassDef(TFFileIter,0) // Iterarator of elements in one file
};


extern TFIOElement *  TFRead(const char * fileName, const char * name,  
                             UInt_t cycle = 0, FMode mode = kFRead, 
                             TClass * classType = NULL);

extern TFIOElement *  TFCreate(const char * templateFName, const char * fileName = NULL);

#endif
