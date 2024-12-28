// ////////////////////////////////////////////////////////////////////////////
//
//  File:      TFError.cpp
//
//  Version:   1.0
//
//  Author:    Reiner Rohlfs (GADC)
//
//  History:   1.0   11.07.03  first released version
//
// ////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdarg.h>

#include "Rtypes.h"

#include "TFError.h"

ClassImp(TFError)
ClassImp(TFErrMsg)
ClassImp(TFException)

//_____________________________________________________________________________
// TFError:
//    The TFError class has only static member function and can store several
//    error messages. The oldest error message will be deleted as soon as the 
//    maximum number of error message is reached. But the number of stored 
//    messages can be set ( see SetMaxErrors() - function ). The default value 
//    is 20 error messages.
//
//    Several of the TF - classes write error messages into this storage. At the
//    end of an application or at any time the error messages can be printed
//    with PrintErrors().
//
//
// TFErrMsg:
//    TFErrMsg is one error message used by the TFError class.
//    This class is not designed to be used by an application but only by
//    TFError
//
//
// TFException:
//    Some of the TF - methods throw an exception if for example
//    TFError::SetErrorType(kExceptionErr) was set. As default 
//    the functions do not throw an exception, but return an error status. 
//    The application program should catch them and can either print the 
//    error message with the PrintError() function or add the error message 
//    to the TFError class with the AddToError() function.
//

Int_t       TFError::fMaxErrors  = 20;
Int_t       TFError::fNumErrors  = 0;
TFErrMsg *  TFError::fErrMsgs    = NULL;
TFErrorType TFError::fErrorType  = kStoreErr;

//_____________________________________________________________________________
void TFErrMsg::Add(TFErrMsg * errMsg)
{
// Adds one error message to the linked list

   if (fNext)
      fNext->Add(errMsg);
   else
      fNext = errMsg;
}
//_____________________________________________________________________________
TFErrMsg * TFErrMsg::Remove()
{
// Removes and deletes this error message and returns the next message in 
// the linked list

   TFErrMsg * rc = fNext;
   fNext = NULL;
   delete this;
   return rc;
}
//_____________________________________________________________________________
//_____________________________________________________________________________
void TFError::AddError(TString & function, TString & errorMsg)
{
// Adds one error message. The function name and a message should be 
// specified. The default length of the function name at the printout
// functions GetError() and PrintErrors(), is 24 characters.
// Independent of a previous call of SetErrorType() this function will 
// always store the error message and will never throw an 
// TFException - ecxeption.

   TFErrMsg * errMsg = new TFErrMsg;
   errMsg->fFunction = function;
   errMsg->fMsg = errorMsg;

   if (fErrMsgs)
      {
      fErrMsgs->Add(errMsg);
      if (fMaxErrors == fNumErrors)
         fErrMsgs = fErrMsgs->Remove();
      else
         fNumErrors++;
      }
   else
      {
      fErrMsgs = errMsg;
      fNumErrors = 1;
      }

}
//_____________________________________________________________________________
void TFError::SetError(const char * function, const char * errorMsg, ...)
{
// Set one error message. The function name and a message should be 
// specified. The default length of the function name at the printout
// functions GetError() and PrintErrors(), is 24 characters.
// The maximum length of the errorMsg is 1999 characters. 
// Depending on a previous call of SetErrorType() this function will store
// the error message, will throw an TFException - ecxeption or will do both.


   char hstr[2000];

   va_list vaList;
   va_start(vaList, errorMsg);
   vsprintf(hstr, errorMsg, vaList);
   va_end(vaList);

   if (fErrorType & kStoreErr)
      {
      TFErrMsg * errMsg = new TFErrMsg;
      errMsg->fFunction = function;
      errMsg->fMsg = hstr;

      if (fErrMsgs)
         {
         fErrMsgs->Add(errMsg);
         if (fMaxErrors == fNumErrors)
            fErrMsgs = fErrMsgs->Remove();
         else
            fNumErrors++;
         }
      else
         {
         fErrMsgs = errMsg;
         fNumErrors = 1;
         }
      }

   if (fErrorType & kExceptionErr)
      throw TFException(function, hstr);

}
//_____________________________________________________________________________
void TFError::RemveLastError()
{
// Removes the last, the youngest, error message from the list of messages

   TFErrMsg * prev = NULL; 
   TFErrMsg * errMsg = fErrMsgs;

   if (fErrMsgs == NULL)
      return;

   while (errMsg->fNext)
      {
      prev = errMsg;
      errMsg = errMsg->fNext;
      }
   
   delete errMsg;
   if (prev)
      prev->fNext = NULL;
   else
      fErrMsgs = NULL;
   fNumErrors--;

}
//_____________________________________________________________________________
void TFError::ClearErrors()
{
// Removes all error messages

   fNumErrors = 0; 
   delete fErrMsgs;
   fErrMsgs = NULL;
}
//_____________________________________________________________________________
void TFError::SetMaxErrors(Int_t num)
{
// Set the maximum number of stored error messages. The default value is 20
// The function will remove the oldest error messages if the number is set
// to be less than the actual stored error messages.

   if (num < 1)   num = 1;
   fMaxErrors = num;
   
   while (fNumErrors > fMaxErrors)
      {
      fErrMsgs = fErrMsgs->Remove();
      fNumErrors--;
      }
}
//_____________________________________________________________________________
void TFError::PrintErrors()
{
// Prints all error messages on standard output
// The oldest error is printed first

   TFErrMsg * errMsg = fErrMsgs;
   if (errMsg)
      printf("Error stack:\n");

   while (errMsg)
      {
      printf("%24s : %s\n", errMsg->fFunction.Data(), errMsg->fMsg.Data() );
      errMsg = errMsg->fNext;
      };
}
//_____________________________________________________________________________
char * TFError::GetError(int num, char * errStr)
{
// Gets one error message as string. The function and the error message
// is copied into the string errStr and the pointer errStr is returned.
// num defines the 0 based index of the message. The errStr will be empty
// if num >=  NumErrors(). The oldes error has num == 0.
// errStr has to be long enough to hold the function name and the error
// message. 2100 bytes should always be enough
   TFErrMsg * errMsg;

   errMsg = fErrMsgs;
   while (errMsg && num)
      {
      errMsg = errMsg->fNext;
      num--;
      }

   if (errMsg)
      sprintf(errStr, "%24s : %s", errMsg->fFunction.Data(), errMsg->fMsg.Data() );
   else
      errStr[0] = 0;

   return errStr;
}
//_____________________________________________________________________________
//_____________________________________________________________________________
TFException::TFException(const char * function, const char * errorMsg, ...)
{
// The function name and a message should be specified. The default length
// of the function name at the printout is 24 characters.
// The maximum length of the errorMsg is 1999 characters. 

   char hstr[2000];

   va_list vaList;
   va_start(vaList, errorMsg);
   vsprintf(hstr, errorMsg, vaList);
   va_end(vaList);

   fFunction = function;
   fMsg      = hstr;
}
//_____________________________________________________________________________
void TFException::PrintError()
{
// Prints the function name and the error messages on standard output

   printf("%24s : %s\n", fFunction.Data(), fMsg.Data() );
}

