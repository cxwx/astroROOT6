// ////////////////////////////////////////////////////////////////////////////
//
//  File:      TFFormat.h
//
//  Version:   1.0
//
//  Author:    Reiner Rohlfs (GADC)
//
//  History:   1.0   18.07.03  first released version
//
// ////////////////////////////////////////////////////////////////////////////
#ifndef ROOT_TFFormat
#define ROOT_TFFormat

#include <stdlib.h>
#include <vector>

#ifndef ROOT_TFError
#include "TFError.h"
#endif

template <class T> class DefaultFormat
{
public:
    static char * Format(char * str, Int_t width, const char * format, T value)
            {
            if (width == 0)
               sprintf(str, (format == NULL) ? "" : format , value);
            else
               sprintf(str, (format == NULL) ? "" : format , width, value);
            return str;
            }
    static void SetString(const char * str, T & value) {}
    static const char * GetTypeName() {return "unknown";}
    static const char * GetBranchType() {return "";}
    static double       ToDouble(const T & value) 
                   {TFError::SetError("DefaultFormat::ToDouble", 
                                         "Cannot convert any value to double"); return 0;}
    static void         SetDouble(Double_t dbl, T & value) {}
};

//_________________________________________________________________________
class BoolFormat
{
public:
    static char * Format(char * str, Int_t width, const char * format, Bool_t value)
            {
            if (width == 0)
               sprintf(str, "%s", value ? "true" : "false");
            else if (abs(width) < 5)
               sprintf(str, "%*s", width, value ? "T" : "F");
	    else
               sprintf(str, "%*s", width, value ? "true" : "false");
            return str;
            }
    static void SetString(const char * str, std::vector<Bool_t>::reference value)
            { value = str[0] == 'T' || str[0] == 't'; }
    static void SetString(const char * str, Bool_t & value)
            { value = str[0] == 'T' || str[0] == 't'; }
    static const char * GetTypeName() {return "Bool_t";}
    static const char * GetBranchType() {return "/b";}
    static double       ToDouble(Bool_t value) {return value;}
//    static void         SetDouble(Double_t dbl, Bool_t & value) {value = abs(value) >= 0.5;}
    static void         SetDouble(Double_t dbl, Bool_t & value) {value = dbl >= 0.5 or dbl <= -0.5;}
};

//_________________________________________________________________________
class BoolCharFormat
{
public:
    static char * Format(char * str, Int_t width, const char * format, Char_t value)
            {
            if (width == 0)
               sprintf(str, "%s", value ? "true" : "false");
            else if (abs(width) < 5)
               sprintf(str, "%*s", width, value ? "T" : "F");
            else
               sprintf(str, "%*s", width, value ? "true" : "false");
            return str;
            }
    static void SetString(const char * str, Char_t & value)
            { value = str[0] == 'T' || str[0] == 't'; }
    static const char * GetTypeName() {return "Bool_t";}
    static const char * GetBranchType() {return "/b";}
    static double       ToDouble(Char_t value) {return value;}
    static void         SetDouble(Double_t dbl, Char_t & value) {value = dbl >= 0.5 or dbl <= -0.5;}
};

//_________________________________________________________________________
class CharFormat
{
public:
    static char * Format(char * str, Int_t width, const char * format, Char_t value)
            {
            if (width == 0)
               sprintf(str, (format == NULL) ? "%hhd" : format, value);
            else
               sprintf(str, (format == NULL) ? "%*hhd" : format, width, value);
            return str;
            }
    static void SetString(const char * str, Char_t & value)
            { sscanf(str, "%d", &value);  }
    static const char * GetTypeName() {return "Char_t";}
    static const char * GetBranchType() {return "/B";}
    static double       ToDouble(Char_t value) {return value;}
    static void         SetDouble(Double_t dbl, Char_t & value) {value = (Char_t)(dbl + ((dbl > 0) ? 0.5: -0.5));}
};

//_________________________________________________________________________
class UCharFormat
{
public:
    static char * Format(char * str, Int_t width, const char * format, UChar_t value)
            {
            if (width == 0)
               sprintf(str, (format == NULL) ? "%hhu" : format, value);
            else
               sprintf(str, (format == NULL) ? "%*hhu" : format, width, value);
            return str;
            }
    static void SetString(const char * str, UChar_t & value)
            { sscanf(str, "%u", &value);  }
    static const char * GetTypeName() {return "UChar_t";}
    static const char * GetBranchType() {return "/b";}
    static double       ToDouble(UChar_t value) {return value;}
    static void         SetDouble(Double_t dbl, UChar_t & value) {value = (UChar_t)(dbl + 0.5);}
};

//_________________________________________________________________________
class ShortFormat
{
public:
    static char * Format(char * str, Int_t width, const char * format, Short_t value)
            {
            if (width == 0)
               sprintf(str, (format == NULL) ? "%hd" : format, value);
            else
               sprintf(str, (format == NULL) ? "%*hd" : format, width, value);
            return str;
            }
    static void SetString(const char * str, Short_t & value)
            { sscanf(str, "%d", &value);  }
    static const char * GetTypeName() {return "Short_t";}
    static const char * GetBranchType() {return "/S";}
    static double       ToDouble(Short_t value) {return value;}
    static void         SetDouble(Double_t dbl, Short_t & value) {value = (Short_t)(dbl + ((dbl > 0) ? 0.5: -0.5));}
};

//_________________________________________________________________________
class UShortFormat
{
public:
    static char * Format(char * str, Int_t width, const char * format, UShort_t value)
            {
            if (width == 0)
               sprintf(str, (format == NULL) ? "%hu" : format, value);
            else
               sprintf(str, (format == NULL) ? "%*hu" : format, width, value);
            return str;
            }
    static void SetString(const char * str, UShort_t & value)
            { sscanf(str, "%u", &value);  }
    static const char * GetTypeName() {return "UShort_t";}
    static const char * GetBranchType() {return "/s";}
    static double       ToDouble(UShort_t value) {return value;}
    static void         SetDouble(Double_t dbl, UShort_t & value) {value = (UShort_t)(dbl + 0.5);}
};

//_________________________________________________________________________
class IntFormat
{
public:
    static char * Format(char * str, Int_t width, const char * format, Int_t value)
            {
            if (width == 0)
               sprintf(str, (format == NULL) ? "%d" : format, value);
            else
               sprintf(str, (format == NULL) ? "%*d" : format, width, value);
            return str;
            }
    static void SetString(const char * str, Int_t & value)
            { sscanf(str, "%d", &value);  }
    static const char * GetTypeName() {return "Int_t";}
    static const char * GetBranchType() {return "/I";}
    static double       ToDouble(Int_t value) {return value;}
    static void         SetDouble(Double_t dbl, Int_t & value) {value = (Int_t)(dbl + ((dbl > 0) ? 0.5: -0.5));}
};

//_________________________________________________________________________
class UIntFormat
{
public:
    static char * Format(char * str, Int_t width, const char * format, UInt_t value)
            {
            if (width == 0)
               sprintf(str, (format == NULL) ? "%u" : format, value);
            else
               sprintf(str, (format == NULL) ? "%*u" : format, width, value);
            return str;
            }
    static void SetString(const char * str, UInt_t & value)
            { sscanf(str, "%u", &value);  }
    static const char * GetTypeName() {return "UInt_t";}
    static const char * GetBranchType() {return "/i";}
    static double       ToDouble(UInt_t value) {return value;}
    static void         SetDouble(Double_t dbl, UInt_t & value) {value = (UInt_t)(dbl + 0.5);}
};

//_________________________________________________________________________
class FloatFormat
{
public:
    static char * Format(char * str, Int_t width, const char * format, Float_t value)
            {
            if (width == 0)
               sprintf(str, (format == NULL) ? "%g" : format, value);
            else
               sprintf(str, (format == NULL) ? "%*g" : format, width, value);
            return str;
            }
    static void SetString(const char * str, Float_t & value) 
            { sscanf(str, "%g", &value);  }
    static const char * GetTypeName() {return "Float_t";}
    static const char * GetBranchType() {return "/F";}
    static double       ToDouble(Float_t value) {return value;}
    static void         SetDouble(Double_t dbl, Float_t & value) {value = dbl;}
};

//_________________________________________________________________________
class DoubleFormat
{
public:
    static char * Format(char * str, Int_t width, const char * format, Double_t value)
            {
            if (width == 0)
               sprintf(str, (format == NULL) ? "%g" : format, value);
            else
               sprintf(str, (format == NULL) ? "%*g" : format, width, value);
            return str;
            }
    static void SetString(const char * str, Double_t & value) 
            { sscanf(str, "%lg", &value);  }
    static const char * GetTypeName() {return "Double_t";}
    static const char * GetBranchType() {return "/D";}
    static double       ToDouble(Double_t value) {return value;}
    static void         SetDouble(Double_t dbl, Double_t & value) {value = dbl;}
};
//_________________________________________________________________________
class StringFormat
{
public:
    static char * Format(char * str, Int_t width, const char * format, const TString & value)
            {
            if (width == 0)
               sprintf(str, (format == NULL) ? "%s" : format, value.Data());
            else
               sprintf(str, (format == NULL) ? "%*s" : format, width, value.Data());
            return str;
            }
    static void SetString(const char * str, TString & value ) {value = str;}
    static const char * GetTypeName()     {return "TString";}
    static const char * GetBranchType()   {return "";}
    static double       ToDouble(const TString & value)
                                            {double d; sscanf(value.Data(), "%lf",&d); return d;}
    static void         SetDouble(Double_t dbl, TString & value) {value.Resize(0); value += dbl;}
};

#endif

