// ///////////////////////////////////////////////////////////////////
//
//  File:      TFNameSelect.cpp
//
//  Version:   1.0
//
//  Author:    Reiner Rohlfs (GADC)
//
//  History:   1.0   08.08.03  first released version
//
// ///////////////////////////////////////////////////////////////////
#include <stdlib.h>

#include "TFGroup.h"

ClassImp(TFToken)
ClassImp(TFNameSelector)

//_____________________________________________________________________________
// TFToken:
//    TFToken is a class used by TFNameSelector and should not directly used by 
//    an application macro.
//
// TFNameSelector
//    The TFNameSelector class can be used to select elements (TFIOElement) in 
//    a TFGroup depending on the name of the elements: Create a group iterator 
//    ( TFGroup::MakeIter() ) and pass one ore more TFNameSelectors to this 
//    iterator ( TFGroupIterator::SetSelector() ) before any call to the 
//    function TFGroupIterator::Next(). Now Next() will "move" the iterator only
//    to TFIOElements that pass the selection criteria. 
//
//    One or more names have to be passed to the constructor of the TFNameSelector.
//    The names can include none, one or several "*" as a placeholder for any 
//    sub-string. The selector will pass element names that matches one of these 
//    names.
//
//    If more than one TFSelector is passed to a group iterator only elements 
//    that pass every selector will be returned. 
//    If one TFNameSelector has more than one name (second constructor) an 
//    element will pass this selector if one of these names matches the name of 
//    the element.

TFNameSelector::TFNameSelector(const char * name)
{
// Constructor for one name. name can include "*".
   fNumNames = 1;
   fNames = new TFToken[1];
   fNames->SetName(name);
}
//_____________________________________________________________________________
TFNameSelector::TFNameSelector(const char ** names, UInt_t numNames)
{
// Constructor for one or more names. names can include "*".
   fNumNames = numNames;
   fNames = new TFToken[fNumNames];

   for (UInt_t nm = 0; nm < numNames; nm++)
      (fNames+nm)->SetName(names[nm]);
}
//_____________________________________________________________________________
TFNameSelector::~TFNameSelector()
{
   delete [] fNames;
}
//_____________________________________________________________________________
Bool_t TFNameSelector::Select(TFElementPtr & item) const
{
// This function is used by the TFGroupIter to test if a element should
// be returned by the operator *() and operator ->()
 
   for (UInt_t nm = 0; nm < fNumNames; nm++)
      if ( (fNames+nm)->Select(item.GetElementName()) )
         return kTRUE;

   return kFALSE;            
}
//_____________________________________________________________________________
//_____________________________________________________________________________
TFToken::TFToken()
{
   fBuffer     = NULL;
   fToken      = NULL;
   fNumToken   = 0;
}
//_____________________________________________________________________________
TFToken::~TFToken()
{
   delete [] fBuffer;
   delete [] fToken;
}
//_____________________________________________________________________________
void TFToken::SetName(const char * name)
{
// Set the name for this token
   size_t   length = strlen(name);

   fBuffer = new char[length +1];
   strcpy(fBuffer, name);

   fLead = name[0] == '*';
   fTrail = length > 0 && name[length -1] == '*';
   UInt_t num = 2;
   while (*name)
      {
      if (*name == '*') num++;
      name++;
      }
   fToken = new char* [num];

   for (fNumToken = 0; ; fNumToken++)
      if ( (fToken[fNumToken] = strtok(fNumToken ? NULL : fBuffer, "*")) == NULL)
         break;
}
//_____________________________________________________________________________
Bool_t TFToken::Select(const char * testName)
{
// Tests if testName matches the name passed by the function SetName()
   if (fNumToken == 0)
      {
      if (fLead || fTrail) 
         return kTRUE;        // ie, only '*'
      else
         return kFALSE;       // empty test tring
      }

   const char * pos1, * pos2;

   pos1 = testName;
   for (UInt_t num = 0; num < fNumToken; num++)
      {
      if (!fTrail && num == fNumToken -1)
         {
         if (strlen(fToken[num]) > strlen(pos1))
            return kFALSE;
         pos1 = testName + strlen(testName) - strlen(fToken[num]);
         }
      if ((pos2 = strstr(pos1, fToken[num])) == NULL)
         return kFALSE;

      if (!fLead && num == 0 && pos1 != pos2)
         return kFALSE;       // 1.  token must start at 1. char
      
      pos1 = pos2 + strlen(fToken[num]);
      }

   return kTRUE;
}
//_____________________________________________________________________________
