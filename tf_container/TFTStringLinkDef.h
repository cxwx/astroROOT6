/////////////////////////////////////////////////////////////////////
//
//  File:      TFStringLinkDef.h
//
//  Version:   1.0
//
//  Author:    Reiner Rohlfs (GADC)
//
//  History:   1.0   11.08.03  first released version
//
/////////////////////////////////////////////////////////////////////
#ifdef __CLING__

#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

#pragma link C++ function operator<=(const TString& , const TString&);
#pragma link C++ function operator>=(const TString& , const TString&);
#pragma link C++ function operator!=(const TString& , const TString&);
#pragma link C++ function operator> (const TString& , const TString&);
#pragma link C++ function operator< (const TString& , const TString&);

#pragma link C++ function operator<=(const TString& , const char*);
#pragma link C++ function operator>=(const TString& , const char*);
#pragma link C++ function operator!=(const TString& , const char*);
#pragma link C++ function operator> (const TString& , const char*);
#pragma link C++ function operator< (const TString& , const char*);
#pragma link C++ function operator==(const TString& , const char*);



#endif
