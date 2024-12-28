//  File:      TFNameConvert.h
//
//  Version:   1.0
//
//  Author:    Reiner Rohlfs
//
//  History:   1.0 first version. 
//                 TFNameConvert was previously defined in
//                 TFIOElement.h
//
/////////////////////////////////////////////////////////////////////
#ifndef ROOT_TFNameConvert
#define ROOT_TFNameConvert

#ifndef ROOT_Rtypes
#include <Rtypes.h>
#endif


//_____________________________________________________________________________

class TFNameConvert
{
public:
   enum TFUpLow { kToUpper, kToLower, kNoChange};

protected:

   char        * fFrom;    // convert these characters to m_to
   char        fTo;        // convert all chars of m_from to this char
   TFUpLow     fUpLow;     // convert to uppeer, lower case or no converion
   char        * fResult;  //!holdes the result string 

public:

   TFNameConvert();
   TFNameConvert(TFUpLow upLow, const char * from = NULL, char to = '_');
   TFNameConvert(const char * from, char to);
   virtual ~TFNameConvert();

   // default is no vonversion
   virtual const char * Conv(const char * name);

   virtual void         SetCaseMode(TFUpLow upLow = kToLower) {fUpLow = upLow;}
   virtual void         SetCharConversion(const char * from, char to);


ClassDef(TFNameConvert, 0)
};


#endif

