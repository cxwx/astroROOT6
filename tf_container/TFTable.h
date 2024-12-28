// ////////////////////////////////////////////////////////////////////////////
//
//  File:      TFTable.h
//
//  Version:   1.0
//
//  Author:    Reiner Rohlfs (GADC)
//
//  History:   1.0   16.07.03  first released version
//
// ////////////////////////////////////////////////////////////////////////////
#ifndef ROOT_TFTable
#define ROOT_TFTable

#ifndef ROOT_TTree
#include "TTree.h"
#endif


#ifndef ROOT_TFIOElement
#include "TFIOElement.h"
#endif

#ifndef ROOT_TFHeader
#include "TFHeader.h"
#endif

#ifndef ROOT_TFColWrapper
#include "TFColWrapper.h"
#endif


class TFBaseCol;
class TFColIter;
class TFRowIter;
class TGraphErrors;
class TFNameConvert;

const UInt_t TF_MAX_ROWS = 0xffffffff;


//_____________________________________________________________________________

class TFTable : public TFIOElement
{
protected:
           UInt_t   fNumRows;     //  number of rows of this table
   mutable ColList  fColumns;     //! sorted list of all columns of this table
   mutable Bool_t   fReadAll;     //! kTRUE if all columns read from file
   mutable UInt_t   fAlreadyRead; //! number of columns already read from file

public:
   TFTable();
   TFTable(const TFTable & table);
   TFTable(TTree * tree);
   TFTable(const char * name, UInt_t numRows = 0);
   TFTable(const char * name, const char * fileName);

   virtual ~TFTable();

   virtual  TFTable &   operator = (const TFTable & table);
   virtual  bool        operator == (const TFHeader & ioelement) const;


   virtual  Int_t       AddColumn(TFBaseCol * column, Bool_t replace = kFALSE);
   virtual  TFBaseCol & AddColumn(const char * name, TClass * colDataType, Bool_t replace = kFALSE);
   virtual  void        DeleteColumn(const char * name);

   virtual  TFBaseCol & GetColumn(const char * name) const;
   virtual  TFBaseCol & operator[] (const char * name) const   {return GetColumn(name);}

   virtual  TFColIter   MakeColIterator() const;
   virtual  TFRowIter   MakeRowIterator() const;

   virtual  void        InsertRows(UInt_t numRows = 1, UInt_t pos = TF_MAX_ROWS);
   virtual  void        DeleteRows(UInt_t numRows = 1, UInt_t pos = TF_MAX_ROWS);

   virtual  UInt_t      GetNumRows() const        {return fNumRows;}
   virtual  UInt_t      GetNumColumns() const;

   virtual  void        Reserve(UInt_t rows);

   virtual  Int_t       SaveElement(const char * fileName = NULL, Int_t compLevel = -1);
   virtual  Int_t       DeleteElement(Bool_t updateMemory = kFALSE);

   virtual  TTree *     MakeTree(TFNameConvert * nameConvert = NULL) const;
   virtual  TGraphErrors * MakeGraph(const char * xCol, const char * yCol,
                                     const char * xErrCol = NULL, const char * yErrCol = NULL,
                                     TGraphErrors * graph = NULL);
   virtual  void        Print(const Option_t* option = "") const;

protected:
   virtual  void        UpdateMemory() {ReadAllCol();}

private:
            I_ColList   ReadCol(const char * name) const;
            void        ReadAllCol() const;


   ClassDef(TFTable,1)  // Table with columns, can be saved into a file
};

//_____________________________________________________________________________

class TFRowIter
{
   const TFTable  * fTable;      // the table of this iterator
         UInt_t   * fRow;        // the selected rows in sorted order
         UInt_t   fNextIndex;    // the next row index of fRow for the operator functions
         UInt_t   fMaxIndex;     // number of row indexes in fRow

   TFRowIter(const TFTable * table);

public:
   TFRowIter(const TFRowIter & rowIter);
   ~TFRowIter()                           {delete [] fRow;}

   TFRowIter & operator = (const TFRowIter & rowIter);

   Bool_t      Filter(const char * filter);
   void        Sort(const char * colName);
   void        ClearFilterSort();
   UInt_t      Map(UInt_t index);
   Bool_t      Next();
   UInt_t      operator * ()              {return fRow[fNextIndex-1];}
   void        Reset()                    {fNextIndex = 0;}

friend TFRowIter TFTable::MakeRowIterator() const;
friend class TFGroupIter;

private:

   ClassDef(TFRowIter, 0)  // A TFTable - iterator for rows
};

//_____________________________________________________________________________

class TFColIter
{
   ColList    * fColList;  // list of all columns 
   I_ColList  fi_Col;      // iterator pointing to the next column
   TFBaseCol  * fCol;      // actual column

   TFColIter(ColList * col ) : fColList(col), fCol(NULL)
                 {Reset();}

public:
   TFColIter(const TFColIter & colIter)
      {fColList = colIter.fColList; fi_Col = colIter.fi_Col; fCol = colIter.fCol;}

   TFColIter & operator = (const TFColIter & colIter);

   Bool_t      Next()         {if (fi_Col == fColList->end()) return kFALSE;
                               fCol = &(fi_Col->GetCol()); ++fi_Col; return kTRUE;}   
   TFBaseCol & operator * ()  {return *fCol;}
   TFBaseCol * operator -> () {return fCol;}
   void        Reset()        {fi_Col = fColList->begin();}

friend TFColIter TFTable::MakeColIterator() const;

   ClassDef(TFColIter, 0) // A TFTable - iterator for columns.
};

//_____________________________________________________________________________
//_____________________________________________________________________________

inline TFColIter & TFColIter::operator = (const TFColIter & colIter)
{
   fColList = colIter.fColList;
   fi_Col   = colIter.fi_Col;
   fCol     = colIter.fCol;
   return *this;
}

//_____________________________________________________________________________
//_____________________________________________________________________________


extern TFTable *  TFReadTable(const char * fileName, const char * name,  
                              UInt_t cycle = 0, FMode mode = kFRead);


#endif

