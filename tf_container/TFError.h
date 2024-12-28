// ////////////////////////////////////////////////////////////////////////////
//
//  File:      TFError.h
//
//  Version:   1.0
//
//  Author:    Reiner Rohlfs (GADC)
//
//  History:   1.0   11.07.03  first released version
//
// ////////////////////////////////////////////////////////////////////////////
#ifndef ROOT_TFError
#define ROOT_TFError

#ifndef ROOT_TString
#include "TString.h"
#endif

enum TFErrorType {kNoErr = 0, kStoreErr = 1, kExceptionErr = 2, kAllErr = 3};

//_____________________________________________________________________________

class TFErrMsg
{
public:
   TString     fFunction;     // function that creates this error message
   TString     fMsg;          // the error message
   TFErrMsg    * fNext;       // the next message in a list of messages

   TFErrMsg()     {fNext = NULL;}
   ~TFErrMsg()    {delete fNext;} 

   virtual  void        Add(TFErrMsg * errMsg);
   virtual  TFErrMsg *  Remove();
      
private:
   ClassDef(TFErrMsg,0) // One error message (internal class)

};

//_____________________________________________________________________________

class TFError
{
   static Int_t         fMaxErrors;    // maximum number of stored errors 
   static Int_t         fNumErrors;    // actual number of stored errors 
  
   static TFErrMsg      * fErrMsgs;    // root of a list of error messages
   static TFErrorType   fErrorType;    // type of error handling: store, exception

public:
   TFError() {}

   static void    AddError(TString & function, TString & errorMsg);
   static void    SetError(const char * function, const char * errorMsg, ...);
   static void    RemveLastError();

   static void    ClearErrors();
   static Bool_t  IsError()                           {return NumErrors() > 0;}
   static Int_t   NumErrors()                         {return fNumErrors;}

   static void          SetErrorType(TFErrorType eType = kAllErr)
                                                      {fErrorType = eType;}
   static TFErrorType   GetErrorType()                {return fErrorType;}

   static void    PrintErrors();
   static char *  GetError(int num, char * errStr);

   static void    SetMaxErrors(Int_t num);
   static Int_t   GetMaxErrors()                      {return fMaxErrors;}

private:
   ClassDef(TFError,0)  // Class to store error messages and to print them

};

//_____________________________________________________________________________
//_____________________________________________________________________________

class TFException 
{ 
protected:
   TString  fFunction;  // function that creates this exception
   TString  fMsg;       // the error message

public:
   TFException() {}
   TFException(const char * function, const char * errorMsg, ...);

   void  PrintError();
   void  AddToError()    {TFError::AddError(fFunction, fMsg);}

   ClassDef(TFException,0)  // Basic exception
};


#endif
