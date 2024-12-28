//_____________________________________________________________________________
//
//  File:      TFHeader.h
//
//  Version:   1.0
//
//  Author:    Reiner Rohlfs (GADC)
//
//  History:   1.0   11.07.03  first released version
//
//_____________________________________________________________________________
#ifndef ROOT_TFHeader
#define ROOT_TFHeader

#include <list>

#ifndef ROOT_TNamed
#include "TNamed.h"
#endif

#ifndef ROOT_TFFormat
#include "TFFormat.h"
#endif

class TFAttrIter;

//_____________________________________________________________________________

class TFBaseAttr : public TNamed
{
protected:
   TString  fComment;         // the comment of this attribute

public:
   TFBaseAttr() {};
   TFBaseAttr(const TFBaseAttr & attribute);

   TFBaseAttr(const char * name, 
              const char * unit = "", 
              const char * comment = "" )
      : TNamed(name, unit), fComment(comment) {}

   virtual ~TFBaseAttr() {}

   virtual TObject *    Clone(const char *newname="") const = 0;
   virtual TFBaseAttr & operator = (const TFBaseAttr & attribute);
   virtual bool         operator == (const TFBaseAttr & attribute) const;
   virtual const char * GetComment() const  {return fComment.Data();}
   virtual const char * GetUnit() const     {return fTitle.Data();}
   virtual void         SetComment(const char * comment)    {fComment = comment;}
   virtual void         SetUnit(const char * unit)          {fTitle = unit;}
   virtual char *       GetStringValue(char * str, Int_t width = 0, 
                                       const char * format = NULL) const = 0;
   virtual void         SetString(const char * str) = 0;

   ClassDef(TFBaseAttr,1) // Abstract base attribute for TFHeader
};

//_____________________________________________________________________________

template <class T, class F = DefaultFormat<T> > 
    class TFAttr : public TFBaseAttr
{
protected:
    T     fValue;   // The value of this attribute

public:
   TFAttr() {}

   TFAttr(const char * name,  T value,
          const char * unit    = "", 
          const char * comment = "" )
      : TFBaseAttr(name, unit, comment)  {fValue = value;}

   TFAttr(const TFAttr<T, F> & attr) 
      : TFBaseAttr(attr)                 {fValue = attr.fValue;}

               
   virtual TObject *      Clone(const char *newname="") const 
                                    {return new TFAttr<T, F>(*this);}
   virtual TFAttr<T, F> & operator = (const TFAttr<T, F> & attribute);
   virtual bool           operator == (const TFBaseAttr & attribute) const;

   virtual                operator T () const          {return fValue;}
   virtual TFAttr<T, F> & operator = (const T & value) {fValue = value; return *this;}
   virtual T              GetValue() const             {return fValue;}
   virtual void           SetValue(T value)            {fValue = value;}
   virtual char *         GetStringValue(char * str, Int_t width = 0, 
                                       const char * format = NULL) const
                                 { return F::Format(str, width, format, fValue); }
   virtual void           SetString(const char * str)  {F::SetString(str, fValue);}

   ClassDef(TFAttr, 1)  // An attribute for TFHeader
};


typedef    TFAttr<Bool_t, BoolFormat>     TFBoolAttr;
typedef    TFAttr<Int_t, IntFormat>       TFIntAttr;
typedef    TFAttr<UInt_t, UIntFormat>     TFUIntAttr;
typedef    TFAttr<Double_t, DoubleFormat> TFDoubleAttr;
typedef    TFAttr<TString, StringFormat>  TFStringAttr;


template <class T, class F>
inline TFAttr<T, F> & TFAttr<T, F>::operator = (const TFAttr<T, F> & attribute)
{
   TFBaseAttr::operator =(attribute);
   fValue = attribute.fValue;
   return *this;
}

template <class T, class F> 
inline bool TFAttr<T, F>::operator == (const TFBaseAttr & attribute) const
{
   return TFBaseAttr::operator==(attribute) &&
          IsA() == attribute.IsA()          &&
          fValue == ((TFAttr<T, F>&)attribute).fValue;
}

//_____________________________________________________________________________

class TFHeader
{
protected:
   std::list<TFBaseAttr*>  fAttr;  // a list of attributes

public:
   TFHeader()  {}
   TFHeader(const TFHeader & header);
   virtual ~TFHeader();

   virtual  TFHeader &     operator = (const TFHeader & header);
   virtual  bool           operator == (const TFHeader & header) const;
   virtual  void           AddAttribute(const TFBaseAttr  & attr, Bool_t replace = kTRUE);
   virtual  TFBaseAttr &   GetAttribute(const char * key, UInt_t index = 0) const;
   virtual  void           DelAttribute(const char * key, Int_t index = -1);
   virtual  UInt_t         GetNumAttributes(const char * key = NULL) const;
   virtual  void           PrintH(const Option_t* option = "") const;

   virtual  TFAttrIter     MakeAttrIterator() const;
   ClassDef(TFHeader,1) // A header with a list of attributes
};

//_____________________________________________________________________________
class TFAttrIter
{
   const std::list<TFBaseAttr*> * fAttr;  // the list of attributes of the header 
   std::list<TFBaseAttr*>::const_iterator i_attr;

   TFBaseAttr  * fAttribute;              // the actual attribute of the iterator
   
   TFAttrIter(const std::list<TFBaseAttr*> * attr)  
      : fAttr(attr) {Reset(); fAttribute = NULL;}

public:

    Bool_t       Next();
    TFBaseAttr & operator * ()   {return *fAttribute;}
    TFBaseAttr * operator -> ()  {return fAttribute;}
    void         Reset()         {i_attr = fAttr->begin();}

friend TFAttrIter TFHeader::MakeAttrIterator() const;

   ClassDef(TFAttrIter, 0) // A TFHeader - iterator for attributes.
};

#endif
