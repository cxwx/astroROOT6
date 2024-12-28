/////////////////////////////////////////////////////////////////////
//
//  File:      TFLinkDef.h
//
//  Version:   1.0
//
//  Author:    Reiner Rohlfs (GADC)
//
//  History:   1.0   14.07.03  first released version
//
/////////////////////////////////////////////////////////////////////
#ifdef __CINT__

#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

#pragma link C++ global g_flt;

#pragma link C++ function TFRead;
#pragma link C++ function TFReadTable;
#pragma link C++ function TFReadGroup;

#pragma link C++ enum  FMode;
#pragma link C++ enum  TFDataType;

#pragma link C++ class TFError;
#pragma link C++ class TFErrMsg;
#pragma link C++ class TFException;

#pragma link C++ class TFHeader+;
#pragma link C++ class TFBaseAttr+;
#pragma link C++ class TFAttr<Bool_t, BoolFormat>+;
#pragma link C++ class TFAttr<Int_t, IntFormat>+;
#pragma link C++ class TFAttr<UInt_t, UIntFormat>+;
#pragma link C++ class TFAttr<Double_t, DoubleFormat>+;
#pragma link C++ class TFAttr<TString, StringFormat>+;
#pragma link C++ class TFAttrIter;

#pragma link C++ class TFIOElement+;
#pragma link C++ class TFVirtualIO;
#pragma link C++ class TFRootFileItem;
#pragma link C++ class TFRootFiles;
#pragma link C++ class TFRootIO;
#pragma link C++ class TFFitsIO;

#pragma link C++ class TFAsroFileItem;
#pragma link C++ class TFAsroFiles;
#pragma link C++ class TFAsroIO;
#pragma link C++ class TFAsroKey+;
#pragma link C++ class TFAsroValue+;
#pragma link C++ class TFAsroFile+;

#pragma link C++ class TFFileIter;
#pragma link C++ class TFVirtualFileIter;
#pragma link C++ class TFRootFileIter;
#pragma link C++ class TFFitsFileIter;
#pragma link C++ class TFAsroFileIter;

#pragma link C++ class TFTable+;
#pragma link C++ class TFColIter;
#pragma link C++ class TFRowIter;
#pragma link C++ class TFFlt;
#pragma link C++ class TFNullIter;

#pragma link C++ class TFGroup+;
#pragma link C++ class TFElementPtr+;
#pragma link C++ class TFFilePath+;
#pragma link C++ class TFGroupIter;
#pragma link C++ class TFSelector;
#pragma link C++ class TFNameSelector;
#pragma link C++ class TFToken;
#pragma link C++ class TFNameConvert;

#pragma link C++ typedef TFBoolAttr;  
#pragma link C++ typedef TFIntAttr;   
#pragma link C++ typedef TFUIntAttr;  
#pragma link C++ typedef TFDoubleAttr;
#pragma link C++ typedef TFStringAttr;


#endif
