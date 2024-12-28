/////////////////////////////////////////////////////////////////////
//
//  File:      TFColLinkDef.h
//
//  Version:   1.0
//
//  Author:    Reiner Rohlfs (GADC)
//
//  History:   1.0   11.08.03  first released version
//
/////////////////////////////////////////////////////////////////////
#ifdef __CINT__

#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

#pragma link C++ class TFSetDbl;
#pragma link C++ class TFBaseCol+;
#pragma link C++ class TFColumn<Char_t, BoolCharFormat>+;
#pragma link C++ class TFColumn<Char_t, CharFormat>+;
#pragma link C++ class TFColumn<UChar_t, UCharFormat>+;
#pragma link C++ class TFColumn<Short_t, ShortFormat>+;
#pragma link C++ class TFColumn<UShort_t, UShortFormat>+;
#pragma link C++ class TFColumn<Int_t, IntFormat>+;
#pragma link C++ class TFColumn<UInt_t, UIntFormat>+;
#pragma link C++ class TFColumn<Float_t, FloatFormat>+;
#pragma link C++ class TFColumn<Double_t, DoubleFormat>+;
#pragma link C++ class TFColumn<TString, StringFormat>+;
#pragma link C++ class TFColumn<TFElementPtr, ElementPtrFormat>+;
#pragma link C++ class TFStringCol+;

#pragma link C++ class TFBinVector<Char_t>+;
#pragma link C++ class TFBinVector<UChar_t>+;
#pragma link C++ class TFBinVector<Short_t>+;
#pragma link C++ class TFBinVector<UShort_t>+;
#pragma link C++ class TFBinVector<Int_t>+;
#pragma link C++ class TFBinVector<UInt_t>+;
#pragma link C++ class TFBinVector<Float_t>+;
#pragma link C++ class TFBinVector<Double_t>+;

#pragma link C++ class TFArrColumn<Char_t,   BoolCharFormat>+;
#pragma link C++ class TFArrColumn<Char_t,   CharFormat>+;
#pragma link C++ class TFArrColumn<UChar_t,  UCharFormat>+;
#pragma link C++ class TFArrColumn<Short_t,  ShortFormat>+;
#pragma link C++ class TFArrColumn<UShort_t, UShortFormat>+;
#pragma link C++ class TFArrColumn<Int_t,    IntFormat>+;
#pragma link C++ class TFArrColumn<UInt_t,   UIntFormat>+;
#pragma link C++ class TFArrColumn<Float_t,  FloatFormat>+;
#pragma link C++ class TFArrColumn<Double_t, DoubleFormat>+;


//#pragma link C++ typedef TFBoolCol;  
#pragma link C++ typedef TFCharCol;  
#pragma link C++ typedef TFUCharCol; 
#pragma link C++ typedef TFShortCol; 
#pragma link C++ typedef TFUShortCol;
#pragma link C++ typedef TFIntCol;   
#pragma link C++ typedef TFUIntCol;  
#pragma link C++ typedef TFFloatCol; 
#pragma link C++ typedef TFDoubleCol;

#pragma link C++ typedef TFBoolArrCol;  
#pragma link C++ typedef TFCharArrCol;  
#pragma link C++ typedef TFUCharArrCol; 
#pragma link C++ typedef TFShortArrCol; 
#pragma link C++ typedef TFUShortArrCol;
#pragma link C++ typedef TFIntArrCol;   
#pragma link C++ typedef TFUIntArrCol;  
#pragma link C++ typedef TFFloatArrCol; 
#pragma link C++ typedef TFDoubleArrCol;


#endif
