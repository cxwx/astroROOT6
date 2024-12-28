// ////////////////////////////////////////////////////////////////////////////
//
//  File:      TFColumn.h
//
//  Version:   1.0
//
//  Author:    Reiner Rohlfs (GADC)
//
//  History:   1.0   18.07.03  first released version
//
// ////////////////////////////////////////////////////////////////////////////
#ifndef ROOT_TFColumn
#define ROOT_TFColumn

#ifndef ROOT_TTree
#include "TTree.h"
#endif

#ifndef ROOT_TFHeader
#include "TFHeader.h"
#endif

#ifndef ROOT_TFColWrapper
#include "TFColWrapper.h"
#endif

#ifndef ROOT_TFTable
#include "TFTable.h"
#endif

#ifndef ROOT_TFNameConvert
#include "TFNameConvert.h"
#endif


#include <vector>
#include <set>

using namespace std;

class TFNullIter;
class TFTable;

template <class T, class F > class TFColumn;
template <class T, class F > class TFArrColumn;
class TFStringCol;

//_____________________________________________________________________________

class TFSetDbl
{
   TFBaseCol   * fCol;     
   UInt_t      fRow;

public:
   TFSetDbl(TFBaseCol * col, UInt_t row) {fCol = col; fRow = row;}
   
   Double_t operator = (Double_t val);
   operator Double_t ();

   ClassDef(TFSetDbl, 0)  // internal class, not to be used by an application
};

//_____________________________________________________________________________

class TFBaseCol : public TNamed, public TFHeader
{
protected:
   set    <ULong64_t> fNull;   // set of (row,bins) which are NULL values
   
public:
   TFBaseCol();
   TFBaseCol(const TFBaseCol & col);
   TFBaseCol(const char * name);
   TFBaseCol(const TString &name);

   virtual ~TFBaseCol()  {};

   virtual TObject *    Clone(const char * name="") const = 0;

   virtual TFBaseCol    & operator = (const TFBaseCol & col);
   virtual bool         operator == (const TFHeader & col) const;

   virtual Bool_t       IsNull(UInt_t row, UInt_t bin = 0) const   {return fNull.find(((ULong64_t)row << 32) + bin) != fNull.end();}
   virtual void         SetNull(UInt_t row, UInt_t bin = 0)        {fNull.insert(((ULong64_t)row << 32) + bin);}
   virtual void         ClearNull(UInt_t row, UInt_t bin = 0)      {fNull.erase(((ULong64_t)row << 32) + bin);}
   virtual Bool_t       HasNull() const                            {return !fNull.empty();}
           TFNullIter   MakeNullIterator() const;

   virtual int          CompareRows(UInt_t row1, UInt_t row2) const = 0;
   virtual Int_t        GetNumBins() const  {return 1;}
   virtual void         SetNumBins(UInt_t bins)             {}      // default does nothing
   virtual UInt_t       GetNumRows() const = 0;
   virtual size_t       GetWidth() const = 0;
   virtual const char * GetUnit() const                     {return fTitle.Data();}
   virtual void         SetUnit(const char * unit)          {fTitle = unit;}
   virtual void         Reserve(UInt_t rows) = 0;
   virtual char *       GetStringValue(UInt_t row, Int_t bin, char * str, Int_t width = 0, 
                                    const char * format = NULL) const = 0;
   virtual void         SetString(UInt_t row, Int_t bin, const char * str) = 0;
   virtual const char * GetTypeName() const = 0;
   virtual const char * GetColTypeName() const = 0;

   virtual void         MakeBranch(TTree* tree, TFNameConvert * nameConvert) const = 0;
   virtual void *       GetBranchBuffer() = 0;
   virtual void         FillBranchBuffer(UInt_t row) const = 0;
   virtual void         CopyBranchBuffer(UInt_t row) = 0;
   virtual void         ClearBranchBuffer() const = 0;


           Double_t     operator[](UInt_t row) const      {return ToDouble(row);}
           TFSetDbl     operator[](UInt_t row)            {return TFSetDbl(this, row);}

           operator     TFColumn<Char_t, BoolCharFormat>    & ();
           operator     TFColumn<Char_t, CharFormat>        & ();
           operator     TFColumn<UChar_t, UCharFormat>      & ();
           operator     TFColumn<Short_t, ShortFormat>      & ();
           operator     TFColumn<UShort_t, UShortFormat>    & ();
           operator     TFColumn<Int_t, IntFormat>          & ();
           operator     TFColumn<UInt_t, UIntFormat>        & ();
           operator     TFColumn<Float_t, FloatFormat>      & ();
           operator     TFColumn<Double_t, DoubleFormat>    & ();
           operator     TFStringCol                         & ();
           operator     TFArrColumn<Char_t, BoolCharFormat> & ();
           operator     TFArrColumn<Char_t, CharFormat>     & ();
           operator     TFArrColumn<UChar_t, UCharFormat>   & ();
           operator     TFArrColumn<Short_t, ShortFormat>   & ();
           operator     TFArrColumn<UShort_t, UShortFormat> & ();
           operator     TFArrColumn<Int_t, IntFormat>       & ();
           operator     TFArrColumn<UInt_t, UIntFormat>     & ();
           operator     TFArrColumn<Float_t, FloatFormat>   & ();
           operator     TFArrColumn<Double_t, DoubleFormat> & ();


protected:
   virtual void         InsertRows(UInt_t numRows, UInt_t pos);
   virtual void         DeleteRows(UInt_t numRows = 1, 
                                   UInt_t pos = TF_MAX_ROWS);

   virtual void         SetDouble(Double_t val, UInt_t row) = 0;
   virtual Double_t     ToDouble(UInt_t row) const = 0;

friend Int_t       TFTable::AddColumn(TFBaseCol * column, Bool_t replace);
friend TFBaseCol & TFTable::AddColumn(const char * name, TClass * colDataType, Bool_t replace);
friend void        TFTable::InsertRows(UInt_t numRows, UInt_t pos);
friend void        TFTable::DeleteRows(UInt_t numRows, UInt_t pos);
friend Double_t    TFSetDbl::operator=(Double_t);
friend TFSetDbl::operator Double_t ();

   ClassDef(TFBaseCol,1) // Abstract base column of TFTable
};


//_____________________________________________________________________________

class TFNullIndex
{
   ULong64_t  fNull;

public:
   TFNullIndex()               {fNull = 0;}
   TFNullIndex(ULong64_t null) {fNull = null;}
    
   TFNullIndex & operator = (ULong64_t nullIndex)  {fNull = nullIndex; return *this;}
   TFNullIndex & operator = (UInt_t    row)        {fNull = (ULong64_t)row << 32; return *this;}

   operator UInt_t() const                         {return (UInt_t)(fNull >> 32);}
   UInt_t   Row() const                            {return (UInt_t)(fNull >> 32);}
   UInt_t   Bin() const                            {return (UInt_t)(fNull);}

};

//_____________________________________________________________________________

class TFNullIter
{
   std::set<ULong64_t>::const_iterator fIter;   // next NULL iterator of a column
   std::set<ULong64_t>::const_iterator fBegin;  // first NULL iterator of a column
   std::set<ULong64_t>::const_iterator fEnd;    // last NULL iterator of a column
   TFNullIndex                    fNull;        // the actual row index of this iterator

   TFNullIter(std::set<ULong64_t>::const_iterator begin,
              std::set<ULong64_t>::const_iterator end) 
         {fBegin = fIter = begin; fEnd = end;}
                   
public:
   TFNullIter(const TFNullIter & nullIter)
      { fIter = nullIter.fIter;     fBegin = nullIter.fBegin;
        fEnd  = nullIter.fEnd;      fNull  = nullIter.fNull; }

   TFNullIter & operator = (const TFNullIter & nullIter);

   Bool_t         Next()         {if (fIter == fEnd) return kFALSE;
                                  fNull = *fIter; ++fIter; return kTRUE;}
   TFNullIndex &  operator * ()  {return fNull;}
   TFNullIndex *  operator -> () {return &fNull;}
   void           Reset()        {fIter = fBegin;}

friend TFNullIter TFBaseCol::MakeNullIterator() const;

   ClassDef(TFNullIter,0) // An iterator for NULL values of columns

};

//_____________________________________________________________________________

template <class T, class F = DefaultFormat<T> >
   class TFColumn : public TFBaseCol
{
protected:
   std::vector <T>      fData;       // all Data of this column
   mutable T            treeBuffer;  //! buffer to fill a TTree

public:
   typedef T value_type;

   TFColumn() {}
   TFColumn(const char * name, int numRows = 0)
      : TFBaseCol(name) , fData(numRows) {}
   TFColumn(const TFColumn<T, F> & column)
      : TFBaseCol(column)
      {fData = column.fData;}


   virtual TFColumn<T, F> & operator = (const TFColumn<T, F> & col) {
                                          if (&col != this) {
                                             TFBaseCol::operator=(col);
                                             fData = col.fData;
                                             }
                                          return *this;
                                          }
   virtual bool         operator == (const TFHeader & col) const  {
                                        return TFBaseCol::operator==(col) &&
                                               IsA() == col.IsA() &&
                                               fData == ((TFColumn<T, F>&)col).fData;}  
            

   virtual TObject * Clone(const char * name="") const {return new TFColumn<T, F>(*this);}

   int CompareRows(UInt_t row1, UInt_t row2) const
      {return (fData[row1] < fData[row2]) ? -1 :
              (fData[row2] < fData[row1]) ?  1 : 0;}


   typename std::vector<T>::const_reference operator[](UInt_t row) const {return fData[row];}
   typename std::vector<T>::reference       operator[](UInt_t row)       {return fData[row];}


   UInt_t  GetNumRows() const     {return fData.size();} 
   size_t  GetWidth() const       {return sizeof(T);}
   void    Reserve(UInt_t rows)   {fData.reserve(rows);}

   char *  GetStringValue(UInt_t row, Int_t bin, char * str, Int_t width = 0, 
                                    const char * format = NULL) const
                           {return F::Format(str, width, format, fData[row]);}
   void    SetString(UInt_t row, Int_t bin, const char * str)
                           {F::SetString(str, fData[row]); }
   const char * GetTypeName()  const   {return F::GetTypeName();}
   const char * GetColTypeName() const {return Class_Name();}

   void    MakeBranch(TTree* tree, TFNameConvert * nameConvert) const
                  {
                     if (F::GetBranchType()[0])
                        {
                        char branch[80];                                          
                        sprintf(branch, "%s%s", nameConvert->Conv(GetName()), F::GetBranchType());
                        tree->Branch(nameConvert->Conv(GetName()), &treeBuffer, (const char*)branch);
                        }
                  }

   void *  GetBranchBuffer()                   {return &treeBuffer;}
   void    FillBranchBuffer(UInt_t row) const  {treeBuffer = fData[row];}
   void    CopyBranchBuffer(UInt_t row)        {fData[row] = treeBuffer;}
   void    ClearBranchBuffer() const {};


protected:
   virtual void     InsertRows(UInt_t numRows, UInt_t pos){ 
                        fData.insert(fData.begin() + pos, numRows, T());
                        TFBaseCol::InsertRows(numRows, pos);
                        }

   virtual void     DeleteRows(UInt_t numRows, UInt_t pos) {
                        fData.erase(fData.begin() + pos, fData.begin() + pos + numRows);
                        TFBaseCol::DeleteRows(numRows, pos);
                        }

   virtual Double_t     ToDouble(UInt_t row) const {return F::ToDouble(fData[row]);}
   virtual void         SetDouble(Double_t val, UInt_t row) {
                                          T b; F::SetDouble(val, b); fData[row]= b;}

   ClassDef(TFColumn, 1) // A column of TFTable
};


//_____________________________________________________________________________

class TFStringCol : public TFColumn<TString, StringFormat>
{
   mutable char   * fCharBuffer;   //! buffer to fill a TTree
   mutable UInt_t fLength;         //! max length of the TStrings

public:
   TFStringCol() {}
   TFStringCol(const char * name, int numRows = 0)
      : TFColumn<TString, StringFormat>(name, numRows) {fCharBuffer = NULL;}
   TFStringCol(const TString & name, int numRows = 0)
      : TFColumn<TString, StringFormat>(name, numRows) {fCharBuffer = NULL;}
   TFStringCol(const TFStringCol & column)
      : TFColumn<TString, StringFormat>(column) {fCharBuffer = NULL;}

   const char * GetColTypeName() const {return Class_Name();}
   void    MakeBranch(TTree* tree, TFNameConvert * nameConvert) const;
   void    CopyBranchBuffer(UInt_t row)        {fData[row] = fCharBuffer;}
   void *  GetStringBranchBuffer(UInt_t lenght);
   void    FillBranchBuffer(UInt_t row) const;

   void    ClearBranchBuffer() const {delete [] fCharBuffer; fCharBuffer = NULL;};

   ClassDef(TFStringCol, 1) // A column of TFTable for strings
};

//_____________________________________________________________________________

template <class T>
   class TFBinVector
{
protected:
   std::vector<T>   fData;

public:
   TFBinVector() {}
   TFBinVector(Int_t num) : fData(num) {}
   TFBinVector(const TFBinVector<T> & bv) : fData(bv.fData) {}

   TFBinVector & operator= (const TFBinVector<T> & bv)
               {fData = bv.fData; return *this;}

   bool        operator == (const TFBinVector<T> & bv) const  
               {return fData == bv.fData;}


   void        resize(Int_t num)    {fData.resize(num);}
   Int_t       size() const         {return fData.size();}

   typename std::vector<T>::const_reference operator[](UInt_t row) const {return fData[row];}
   typename std::vector<T>::reference       operator[](UInt_t row)       {return fData[row];}

   ClassDef(TFBinVector, 1) // Internal class of an array column
};

//_____________________________________________________________________________

template <class T, class F = DefaultFormat<T> >
   class TFArrColumn : public TFBaseCol
{
protected:
   std::vector <TFBinVector<T> > fData;  // all Data of this column

   Int_t           fBins;                // number of bins per row
                                         // < 0 : variable bin number
   mutable T *     treeBuffer;           //! buffer to fill a TTree

public:
   typedef T value_type;

   TFArrColumn() {fBins = 0; treeBuffer = NULL;}
   TFArrColumn(const char * name, int numRows = 0)
      : TFBaseCol(name), fData(numRows) {fBins = 0; treeBuffer = NULL;}
   TFArrColumn(const TFArrColumn<T, F> & column)
      {fData = column.fData; fBins = column.fBins; treeBuffer = NULL;}


   virtual TObject * Clone(const char * name="") const {return new TFArrColumn<T, F>(*this);}

   virtual TFArrColumn<T, F> & operator = (const TFArrColumn<T, F> & col) {
                                            if (&col != this) {
                                              TFBaseCol::operator=(col);
                                              fData = col.fData;
                                              fBins = col.fBins;
                                              }
                                            return *this;
                                            }
   virtual bool         operator == (const TFHeader & col) const  {
                                        return TFBaseCol::operator==(col) &&
                                               IsA() == col.IsA() &&
                                               fBins == ((TFArrColumn<T, F>&)col).fBins &&
                                               fData == ((TFArrColumn<T, F>&)col).fData;}  

   int CompareRows(UInt_t row1, UInt_t row2) const
      {return (fData[row1][0] < fData[row2][0]) ? -1 :
              (fData[row2][0] < fData[row1][0]) ?  1 : 0;}


   typename std::vector<TFBinVector<T> >::const_reference operator[](UInt_t row) const {return fData[row];}
   typename std::vector<TFBinVector<T> >::reference       operator[](UInt_t row)       {return fData[row];}

   Int_t        GetNumBins() const        {return fBins;}
   void         SetNumBins(UInt_t bins)   {if (bins > 0) 
                                             for (int row = 0; row < fData.size(); row++)
                                                fData[row].resize(bins); 
                                             fBins = bins;
                                          }

   void         Reserve(UInt_t rows)      {fData.reserve(rows);}
   UInt_t       GetNumRows() const        {return fData.size();} 
   size_t       GetWidth() const          {return sizeof(T);}

   const char * GetTypeName() const       {return F::GetTypeName();}
   const char * GetColTypeName() const    {return Class_Name();}

   void    MakeBranch(TTree* tree, TFNameConvert * nameConvert) const
                  {
                     if (F::GetBranchType()[0] && fBins > 0)
                        {
                        treeBuffer = new T[fBins];
                        char branch[80];                                          
                        sprintf(branch, "%s[%d]%s", nameConvert->Conv(GetName()), fBins, F::GetBranchType());
                        tree->Branch(nameConvert->Conv(GetName()), (void*)treeBuffer, (const char*)branch);
                        }
                  }

   void *  GetBranchBuffer()   {treeBuffer = new T[fBins]; return treeBuffer;}

   void    FillBranchBuffer(UInt_t row) const
                  {
                     if (F::GetBranchType()[0] && fBins > 0)
                        {
                        for (Int_t num = 0; num < fBins; num++)
                           treeBuffer[num] = fData[row][num];
                        }
                  }
   void    CopyBranchBuffer(UInt_t row)        
                  {
                     for (Int_t num = 0; num < fBins; num++)
                        fData[row][num] = treeBuffer[num];
                  }

   void    ClearBranchBuffer() const {delete [] treeBuffer; treeBuffer = NULL;};

   char *  GetStringValue(UInt_t row, Int_t bin, char * str, Int_t width = 0, 
                                    const char * format = NULL) const
                           {return F::Format(str, width, format, fData[row][bin]);}
   void    SetString(UInt_t row, Int_t bin, const char * str)
                           {F::SetString(str, fData[row][bin]); }

protected:
   virtual void     InsertRows(UInt_t numRows, UInt_t pos) {
                        fData.insert(fData.begin() + pos, numRows, 
                                     TFBinVector<T>(fBins >= 0 ? fBins : 0));
                        TFBaseCol::InsertRows(numRows, pos);
                        }
   virtual void     DeleteRows(UInt_t numRows, UInt_t pos) {
                        fData.erase(fData.begin() + pos, fData.begin() + pos + numRows);
                        TFBaseCol::DeleteRows(numRows, pos);
                        }

   virtual Double_t     ToDouble(UInt_t row) const {return 0.0;}
   virtual void         SetDouble(Double_t val, UInt_t row)  {};

   ClassDef(TFArrColumn, 1) // An array column of TFTable (each bin is an array)
};


//_____________________________________________________________________________
//_____________________________________________________________________________

typedef    TFColumn<Char_t, BoolCharFormat> TFBoolCol;
typedef    TFColumn<Char_t, CharFormat>     TFCharCol;
typedef    TFColumn<UChar_t, UCharFormat>   TFUCharCol;
typedef    TFColumn<Short_t, ShortFormat>   TFShortCol;
typedef    TFColumn<UShort_t, UShortFormat> TFUShortCol;
typedef    TFColumn<Int_t, IntFormat>       TFIntCol;
typedef    TFColumn<UInt_t, UIntFormat>     TFUIntCol;
typedef    TFColumn<Float_t, FloatFormat>   TFFloatCol;
typedef    TFColumn<Double_t, DoubleFormat> TFDoubleCol;

typedef    TFArrColumn<Char_t, BoolCharFormat> TFBoolArrCol;
typedef    TFArrColumn<Char_t, CharFormat>     TFCharArrCol;
typedef    TFArrColumn<UChar_t, UCharFormat>   TFUCharArrCol;
typedef    TFArrColumn<Short_t, ShortFormat>   TFShortArrCol;
typedef    TFArrColumn<UShort_t, UShortFormat> TFUShortArrCol;
typedef    TFArrColumn<Int_t, IntFormat>       TFIntArrCol;
typedef    TFArrColumn<UInt_t, UIntFormat>     TFUIntArrCol;
typedef    TFArrColumn<Float_t, FloatFormat>   TFFloatArrCol;
typedef    TFArrColumn<Double_t, DoubleFormat> TFDoubleArrCol;

//_____________________________________________________________________________
//_____________________________________________________________________________

inline Double_t     TFSetDbl::operator = (Double_t val)  {fCol->SetDouble(val, fRow); return val;}   
inline              TFSetDbl::operator Double_t () {return fCol->ToDouble(fRow);}

inline TFNullIter   TFBaseCol::MakeNullIterator() const  {return TFNullIter(fNull.begin(), fNull.end());}

inline TFBaseCol::operator TFColumn<Char_t, BoolCharFormat>    & () {return dynamic_cast <TFColumn<Char_t, BoolCharFormat>    &>(*this);}
inline TFBaseCol::operator TFColumn<Char_t, CharFormat>        & () {return dynamic_cast <TFColumn<Char_t, CharFormat>        &>(*this);}
inline TFBaseCol::operator TFColumn<UChar_t, UCharFormat>      & () {return dynamic_cast <TFColumn<UChar_t, UCharFormat>      &>(*this);}
inline TFBaseCol::operator TFColumn<Short_t, ShortFormat>      & () {return dynamic_cast <TFColumn<Short_t, ShortFormat>      &>(*this);}
inline TFBaseCol::operator TFColumn<UShort_t, UShortFormat>    & () {return dynamic_cast <TFColumn<UShort_t, UShortFormat>    &>(*this);}
inline TFBaseCol::operator TFColumn<Int_t, IntFormat>          & () {return dynamic_cast <TFColumn<Int_t, IntFormat>          &>(*this);}
inline TFBaseCol::operator TFColumn<UInt_t, UIntFormat>        & () {return dynamic_cast <TFColumn<UInt_t, UIntFormat>        &>(*this);}
inline TFBaseCol::operator TFColumn<Float_t, FloatFormat>      & () {return dynamic_cast <TFColumn<Float_t, FloatFormat>      &>(*this);}
inline TFBaseCol::operator TFColumn<Double_t, DoubleFormat>    & () {return dynamic_cast <TFColumn<Double_t, DoubleFormat>    &>(*this);}
inline TFBaseCol::operator TFStringCol                         & () {return dynamic_cast <TFStringCol                         &>(*this);}
inline TFBaseCol::operator TFArrColumn<Char_t, BoolCharFormat> & () {return dynamic_cast <TFArrColumn<Char_t, BoolCharFormat> &>(*this);}
inline TFBaseCol::operator TFArrColumn<Char_t, CharFormat>     & () {return dynamic_cast <TFArrColumn<Char_t, CharFormat>     &>(*this);}
inline TFBaseCol::operator TFArrColumn<UChar_t, UCharFormat>   & () {return dynamic_cast <TFArrColumn<UChar_t, UCharFormat>   &>(*this);}
inline TFBaseCol::operator TFArrColumn<Short_t, ShortFormat>   & () {return dynamic_cast <TFArrColumn<Short_t, ShortFormat>   &>(*this);}
inline TFBaseCol::operator TFArrColumn<UShort_t, UShortFormat> & () {return dynamic_cast <TFArrColumn<UShort_t, UShortFormat> &>(*this);}
inline TFBaseCol::operator TFArrColumn<Int_t, IntFormat>       & () {return dynamic_cast <TFArrColumn<Int_t, IntFormat>       &>(*this);}
inline TFBaseCol::operator TFArrColumn<UInt_t, UIntFormat>     & () {return dynamic_cast <TFArrColumn<UInt_t, UIntFormat>     &>(*this);}
inline TFBaseCol::operator TFArrColumn<Float_t, FloatFormat>   & () {return dynamic_cast <TFArrColumn<Float_t, FloatFormat>   &>(*this);}
inline TFBaseCol::operator TFArrColumn<Double_t, DoubleFormat> & () {return dynamic_cast <TFArrColumn<Double_t, DoubleFormat> &>(*this);}

inline TFNullIter & TFNullIter::operator = (const TFNullIter & nullIter)
{
   fIter   = nullIter.fIter;
   fBegin  = nullIter.fBegin;
   fEnd    = nullIter.fEnd;
   fNull   = nullIter.fNull;      
   return *this;
}


#endif

