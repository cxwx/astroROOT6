// ////////////////////////////////////////////////////////////////////////////
//
//  File:      TFNameConvert.cxx
//
//  Version:   1.0
//
//  Author:    Reiner Rohlfs (GADC)
//
//  History:   1.0   16.02.05  first released version
//
// ////////////////////////////////////////////////////////////////////////////
#include <ctype.h>
#include <string.h>

#include "TFNameConvert.h"

ClassImp(TFNameConvert)

//_____________________________________________________________________________




//_____________________________________________________________________________
TFNameConvert::TFNameConvert()
{
   fFrom = NULL;
   fResult = NULL;

   SetCaseMode(kNoChange);
   SetCharConversion(NULL, ' ');
}

//_____________________________________________________________________________
TFNameConvert::TFNameConvert(TFUpLow upLow, const char * from, char to)
{
   fFrom = NULL;
   fResult = NULL;

   SetCaseMode(upLow);
   SetCharConversion(from, to);
}

//_____________________________________________________________________________
TFNameConvert::TFNameConvert(const char * from, char to)
{
   fFrom = NULL;
   fResult = NULL;

   SetCaseMode(kNoChange);
   SetCharConversion(from, to);
}

//_____________________________________________________________________________
TFNameConvert::~TFNameConvert()
{
   delete [] fFrom;
   delete [] fResult;
}
//_____________________________________________________________________________
const char * TFNameConvert::Conv(const char * name)
{
   if (fFrom == NULL && fUpLow == kNoChange)
      return name;   

   delete [] fResult;
   fResult = new char [strlen(name) + 1];

   int index = 0;      
   while (*name)
      {
      if (fUpLow == kToLower)
         fResult[index] = tolower(*name);
      else if (fUpLow == kToUpper)
         fResult[index] = toupper(*name);
      else
         fResult[index] = *name;

   
      if (fFrom && strchr(fFrom, fResult[index]) ) 
        fResult[index] = fTo;

      name++;
      index++;
      }

   fResult[index] = 0;

   return fResult;
}

//_____________________________________________________________________________
void TFNameConvert::SetCharConversion(const char * from, char to)
{
   delete [] fFrom;

   if (from == NULL)
      {
      fFrom = NULL;
      fTo   = ' ';
      return;
      }
   
   fFrom = new char [strlen(from) + 1];
   strcpy(fFrom, from);

   fTo = to;
}

