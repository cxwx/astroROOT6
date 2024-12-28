// ////////////////////////////////////////////////////////////////////////////
//
//  File:      TFRowIterator.h
//
//  Version:   1.0
//
//  Author:    Reiner Rohlfs (GADC)
//
//  History:   1.0   21.07.03  first released version
//
// ////////////////////////////////////////////////////////////////////////////
#ifndef ROOT_TFRowIterator
#define ROOT_TFRowIterator

#include <vector>

#ifndef ROOT_TFTable
#include "TFTable.h"
#endif


class TFFlt
{
   std::vector <UChar_t>     fResult;   // the result of the filter operation
   std::vector <TFBaseCol*>  fCols;     // columns of the filter string
   UInt_t               * fRowIndex;    // the sorted and filtered row index

public:
   TFFlt() {};

   // function to initialize this class
   void   SetNumRows(UInt_t numRows)   {fResult.resize(numRows, 0);}
   void   SetRows(UInt_t * rowIndex)   {fRowIndex = rowIndex;}
   void   AddCol(TFBaseCol * col)      {fCols.push_back(col);}

   // functions for rcint
   UInt_t      GetNumRows()           {return fResult.size();}
   TFBaseCol * operator() (int index) {return fCols[index];}
   UChar_t &   operator[] (int index) {return fResult[index];}
   UInt_t      Map(UInt_t row)        {return fRowIndex[row];}

   // functions to clear the class
   void        Reset()                {fResult.clear(); fCols.clear();}

   ClassDef(TFFlt, 0) // An internal class used by the row iterator
};

extern TFFlt g_flt;

#endif
