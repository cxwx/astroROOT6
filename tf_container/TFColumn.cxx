// ////////////////////////////////////////////////////////////////////////////
//
//  File:      TFColumn.cpp
//
//  Version:   1.0
//
//  Author:    Reiner Rohlfs (GADC)
//
//  History:   1.0   18.07.03  first released version
//
// ////////////////////////////////////////////////////////////////////////////
#include <algorithm>
#include "TFError.h"
#include "TFTable.h"
#include "TFColumn.h"

#ifndef TF_CLASS_IMP
#define TF_CLASS_IMP
ClassImp(TFBaseCol)
ClassImp(TFNullIter)

ClassImp2T(TFColumn, T, F)
ClassImpT(TFBinVector, T)
ClassImp2T(TFArrColumn, T, F)
ClassImp(TFStringCol)
#endif    // TF_CLASS_IMP

//_____________________________________________________________________________
// TFBaseCol:
//    TFBaseCol is the abstract base class of all columns of the TFTable.
//
//    A column can have any number of rows. A cell per row can be a 
//    vector of any number of elements (see TF*ArrCol). Variable bin size
//    is implemented. All values of a column have the same data type. 
//    There are several derived template classes for different data types.
// 
//    A column has a name TNamed::fName and a unit TNamed::fTitle.
//    Each cell can be a NULL value.
//    TFBaseCol is derived from TFHeader and therefor a column can 
//    have attributes.
//
//    The number of rows cannot manipulated directly. The column must be 
//    inserted into a table (TFTable::AddColumn() ) and than the number of
//    rows of all inserted columns can be changed. This ensured that all
//    columns of a table have the same number of rows.  
//
//
// TFNullIter:
//    TFNullIter is an iterator to retrieve all rows of a column which
//    have a NULL value. (The operator* () and the operator->() return the
//    0 based row index )
//
// TFStringCol:
//    A "specialisation" of the template class TFColumn for TStrings
//    This column can be used like all other columns.

TFBaseCol::TFBaseCol()
{
// Don't use this constructor. A column should have a name.

}
//_____________________________________________________________________________
TFBaseCol::TFBaseCol (const TFBaseCol & col)
   : TNamed(col), TFHeader(col)
{
// Standard copy constructor.
   fNull = col.fNull;
}
//_____________________________________________________________________________
TFBaseCol::TFBaseCol(const char * name)
   : TNamed(name, "")
{
// TFBaseCol constructor. Never change the name after the column is inserted
// into a table!

}
//_____________________________________________________________________________
TFBaseCol::TFBaseCol(const TString &name)
   : TNamed(name,  TString())
{
// TFBaseCol constructor. Never change the name after the column is inserted
// into a table!

}
//_____________________________________________________________________________
TFBaseCol & TFBaseCol::operator = (const TFBaseCol & col)
{
// assign operator.

   if (&col != this)
      {
      TNamed::operator=(col);
      TFHeader::operator=(col);
      fNull = col.fNull;
      }
   return *this;
}
//_____________________________________________________________________________
bool TFBaseCol::operator == (const TFHeader & col) const
{
   return IsA() == col.IsA()                 &&
          fName == ((TFBaseCol&)col).fName   &&
          TFHeader::operator==(col)          &&
          fNull == ((TFBaseCol&)col).fNull;
}
//_____________________________________________________________________________
void TFBaseCol::InsertRows(UInt_t numRows, UInt_t pos)
{
// Protected function to ensure that all columns of a table have the same
// number of rows. Insert rows into a table while this column is part
// of the table to insert rows into a column.

   std::set <ULong64_t> tmp;
   std::set <ULong64_t>::iterator i_n1, i_n2;

   i_n1 = i_n2 = fNull.lower_bound((ULong64_t)pos << 32);
   // copy all values >= pos into tmp and increase the value by numRows
   for ( ;i_n1 != fNull.end(); i_n1++)
      tmp.insert((*i_n1) + ((ULong64_t)numRows << 32));
   
   // delete all values >= pos and replace them by the increased values in tmp
   fNull.erase(i_n2, fNull.end());
   fNull.insert(tmp.begin(), tmp.end());

}
//_____________________________________________________________________________
void TFBaseCol::DeleteRows(UInt_t numRows, UInt_t pos)
{
// Protected function to ensure that all columns of a table have the same
// number of rows. Delete rows of a table while this column is part
// of the table to delete rows of a column.

   std::set <ULong64_t> tmp;
   std::set <ULong64_t>::iterator i_n1;

   // copy all values >= (pos + numRows) into tmp and decrease the value
   // by numRows
   for (i_n1 = fNull.lower_bound((ULong64_t)(pos + numRows) << 32); 
        i_n1 != fNull.end(); i_n1++)
      tmp.insert((*i_n1) - ((ULong64_t)numRows << 32));

   // delete all values >= pos  and insert the decreased values of tmp
   fNull.erase(fNull.lower_bound(pos), fNull.end());
   fNull.insert(tmp.begin(), tmp.end());
}

//_____________________________________________________________________________
//_____________________________________________________________________________
void TFStringCol::MakeBranch(TTree* tree, TFNameConvert * nameConvert) const
{
// Adds one string - branch to the tree.
// This function  is called by TFTable::MakeTree() and is not designed to be
// used directly by an application.

   TFErrorType errT = TFError::GetErrorType();
   TFError::SetErrorType(kExceptionErr);

   try {
      TFUIntAttr & attr = dynamic_cast<TFUIntAttr&>(GetAttribute("max size"));
      fLength = attr;
      }
   catch (TFException) {
      // we have to find the maximum size of the strings
      fLength = 0;
      for (UInt_t row = 0; row < fData.size(); row++)
         if (fLength < fData[row].Length())
            fLength = fData[row].Length();
      }

   TFError::SetErrorType(errT);


   fCharBuffer = new char [fLength + 1];

   char branch[80];                                          
   sprintf(branch, "%s[%d]/C", nameConvert->Conv(GetName()), fLength + 1);
   tree->Branch(nameConvert->Conv(GetName()), fCharBuffer, (const char*)branch);

}
//_____________________________________________________________________________
void * TFStringCol::GetStringBranchBuffer(UInt_t lenght)
{  
// allocates memory for the branch buffer depending on the maximum lenght
// of a string = lenght and returens this buffer

   fLength = lenght;
   fCharBuffer = new char [fLength + 1];
   return fCharBuffer;
}
//_____________________________________________________________________________
void TFStringCol::FillBranchBuffer(UInt_t row) const
{
// Copies one string into the buffer to be filled into a tree.
// This function  is called by TFTable::MakeTree() and is not designed to be
// used directly by an application.

   strncpy(fCharBuffer, fData[row].Data(), fLength);
   fCharBuffer[fLength] = 0;
}

