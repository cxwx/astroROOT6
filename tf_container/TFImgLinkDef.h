/////////////////////////////////////////////////////////////////////
//
//  File:      TFImgLinkDef.h
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

#pragma link C++ function TFReadImage;

#pragma link C++ class TFBaseImage+;
#pragma link C++ class TFImage<Bool_t,   BoolFormat>+;  
#pragma link C++ class TFImage<Char_t,   CharFormat>+;  
#pragma link C++ class TFImage<UChar_t,  UCharFormat>+; 
#pragma link C++ class TFImage<Short_t,  ShortFormat>+; 
#pragma link C++ class TFImage<UShort_t, UShortFormat>+;
#pragma link C++ class TFImage<Int_t,    IntFormat>+;   
#pragma link C++ class TFImage<UInt_t,   UIntFormat>+;  
#pragma link C++ class TFImage<Float_t,  FloatFormat>+; 
#pragma link C++ class TFImage<Double_t, DoubleFormat>+;

#pragma link C++ class TFImageSplice<Bool_t>;
#pragma link C++ class TFImageSplice<Char_t>;
#pragma link C++ class TFImageSplice<UChar_t>;
#pragma link C++ class TFImageSplice<Short_t>;
#pragma link C++ class TFImageSplice<UShort_t>;
#pragma link C++ class TFImageSplice<Int_t>;
#pragma link C++ class TFImageSplice<UInt_t>;
#pragma link C++ class TFImageSplice<Float_t>;
#pragma link C++ class TFImageSplice<Double_t>;

#pragma link C++ typedef TFBoolImg;  
#pragma link C++ typedef TFCharImg;  
#pragma link C++ typedef TFUCharImg; 
#pragma link C++ typedef TFShortImg; 
#pragma link C++ typedef TFUShortImg;
#pragma link C++ typedef TFIntImg;   
#pragma link C++ typedef TFUIntImg;  
#pragma link C++ typedef TFFloatImg; 
#pragma link C++ typedef TFDoubleImg;

#endif
