// ////////////////////////////////////////////////////////////////////////////
//
//  File:      TFRowIterator.cpp
//
//  Version:   1.0
//
//  Author:    Reiner Rohlfs (GADC)
//
//  History:   1.0   12.08.03  first released version
//
// ////////////////////////////////////////////////////////////////////////////
#include <stdlib.h>
#include <unistd.h>

#include "TROOT.h"

#include "TFTable.h"
#include "TFColumn.h"
#include "TFRowIterator.h"
#include "TFError.h"

#include <string>
#include <list>

#ifndef TF_CLASS_IMP
#define TF_CLASS_IMP
ClassImp(TFRowIter)
ClassImp(TFFlt)
#endif    // TF_CLASS_IMP

//_____________________________________________________________________________
// TFRowIter:
//    This iterator returns one row index of a TFTable or TFGroup after
//    the other. Without sorting and without filter every row index is
//    returned in increasing order. This iterator should be used to filter 
//    rows and to sort the rows of a table. See functions Sort() and Filter()
//    for more information how to sort the rows and how to filter rows.
//
// TFFlt:
//    Internal class, used by the Filter() function. Should not used directly
//    by an application.

TFFlt g_flt;


static TFBaseCol * CompCol;
extern "C" {
static int Comp(const void * row1, const void * row2)
{
   return CompCol->CompareRows(*(UInt_t*)row1, *(UInt_t*)row2);
}
}
//_____________________________________________________________________________
TFRowIter::TFRowIter(const TFTable * table)
{
// Private constructor. Use TFTable::MakeRowIterator() to create
// a row iterator.

   fTable = table;
   fRow   = NULL;

   ClearFilterSort();
}
//_____________________________________________________________________________
TFRowIter::TFRowIter(const TFRowIter & rowIter)
{
// copy constructor

   fTable      = rowIter.fTable;
   fNextIndex  = rowIter.fNextIndex;
   fMaxIndex   = rowIter.fMaxIndex;

   fRow = new UInt_t [fMaxIndex];
   memcpy(fRow, rowIter.fRow, fMaxIndex * sizeof(UInt_t));

}
//_____________________________________________________________________________
TFRowIter & TFRowIter::operator = (const TFRowIter & rowIter)
{
// assignment operator

   if ( this != &rowIter ) 
      {
      fTable      = rowIter.fTable;
      fNextIndex  = rowIter.fNextIndex;
      fMaxIndex   = rowIter.fMaxIndex;

      delete [] fRow;
      fRow = new UInt_t [fMaxIndex];
      memcpy(fRow, rowIter.fRow, fMaxIndex * sizeof(UInt_t));
      }
   return *this;
}
//_____________________________________________________________________________
void TFRowIter::Sort(const char * colName)
{
// Sort the rows depending on the values of the column "colName".
// If colName is a column with more than one item per row only the
// first item of each row is used to sort the rows. 
// The rows are not sorted in the table but a row - index list is sorted.
// Therefore the table cannot be saved into a file with sorted rows.
// But the operator * () will return the row numbers depending on the 
// sorting of this function.
// If the column colName does not exist in the table of this iterator nothing
// will happen, no sorting, no warning and no error message.

   TFErrorType errT = TFError::GetErrorType();
   TFError::SetErrorType(kExceptionErr);

   try{	
      CompCol = &(fTable->GetColumn(colName));
      }
   catch (TFException) 
      {
      TFError::SetErrorType(errT);
      return;
      }	

   TFError::SetErrorType(errT);

   // now we can sort the rows == sort the index numbers in fRow
   qsort(fRow, fMaxIndex, sizeof(UInt_t), Comp);
}
//_____________________________________________________________________________
void TFRowIter::ClearFilterSort()
{
// Resets the sorting of the Sort() - function and clears all
// filters of the Filter() function.
   delete [] fRow;

   fMaxIndex = fTable->GetNumRows();
   fRow = new UInt_t [fMaxIndex];

   fNextIndex = 0;

   for (UInt_t row = 0; row < fMaxIndex; row++)
      fRow[row] = row;
}
//_____________________________________________________________________________
Bool_t TFRowIter::Next()
{
// Increase the internal row counter and the operator * () will return 
// the next row number. The order of the returned row number depend on 
// the filter ( Filter() - function ) and the sorting ( Sort() - function).
// The function will return kFALSE if there is no further row number. 

   if ( fNextIndex >= fMaxIndex ) 
      return kFALSE;

   ++fNextIndex;
   return kTRUE;
}
//_____________________________________________________________________________
UInt_t TFRowIter::Map(UInt_t index)
{
// Returns the row number ( 0 based) of the original table after the sorting and 
// filter is applied. index is the row index in the virtual sorted and 
// filtered table. TF_MAX_ROWS will be returned if index is greater than the  
// number of filtered rows.

   return index < fMaxIndex  ? fRow[index] : TF_MAX_ROWS;
}
//_____________________________________________________________________________
Bool_t TFRowIter::Filter(const char * filter)
{
// Applies a filter to filter rows of a TFTable or TFGroup. A row that 
// does not pass the filter is not returned from the operator * () and
// the operator ->().
//
// filter is a c - expression without ; at the end. Column names of the
// table can be used as variable names in this filter string. They have the
// same data type as their column. If the column has more than one 
// value per row the first value is used in this filter. Of course, the 
// column names must fulfill the requirement for c - variable names.
// Beside that "row" can be used to define the row number ( 0 based ). 
// row is the row number of the original table without sorting and without
// filter. The variable "row_" is the row number (0 based) of the sorted 
// and already filtered row number before the call of this function.
//
// For each row in the table the column variables in the filter string are 
// assigned to the value of the columns at the given row and "row" and "row_" 
// in the filter string are set to the row number ( 0 based ). Than the filter 
// statement is processed with the ROOT interpreter. If the result is  kTRUE
// the given row will be returned from the operator * () and the 
// operator -> (). If the result is kFALSE the given row will not be returned.
// A second call of Filter() will not reset the previous filter but will
// apply the new filter on the already filtered rows.
// 
// The function returns kFALSE if the filter cannot be processed. This means
// either there is a syntax error in the filter string or a used column name
// in the filter string does not exist in the table. The function will write 
// an error message into the error stack ( see TFError ).
// 
// example filter strings (assuming c1, c2 and c3 are column names):
//    "c1 + 2.4 * c2 >= c3"
//    "row > 4 && row < 20"
//    "row_ < 40 || c2 <= c1"
//    

   using namespace std;

   char  hstr[150];
   int   cnt = 0;
   int   err ;
   list<string> assignment;


   g_flt.Reset();
   g_flt.SetNumRows(fMaxIndex);
   g_flt.SetRows(fRow);

   //write the macro into a file: /tmp/tf'pid'_'counter'.C
   char fileName[30];
   char functionName[30];
   static int counter = 0;
   sprintf(functionName, "tf%d_%d", getpid(), ++counter);
   sprintf(fileName, "/tmp/%s.C", functionName);

   FILE * macroFile  = fopen(fileName, "w");
   if (macroFile == NULL)
      {
      TFError::SetError("TFRowIter::Filter", 
                        "Cannot open temporary macro file %s", fileName);
      return kFALSE;
      }

   fprintf(macroFile, "void %s() {\n", functionName);

   TFColIter iter = fTable->MakeColIterator();
   while ( iter.Next() )
      {
      if (strstr(filter, iter->GetName()))
         {
         // this column is in the filter
         g_flt.AddCol(&*iter);
         
         // this line is something like
         //  Int_t colName1; TFIntCol * __colName1__ = (TFIntCol*)g_flt(0);
         fprintf(macroFile, "%s %s; %s *__%s__ = (%s*)g_flt(%d);\n",
                 iter->GetTypeName(), iter->GetName(),
                 iter->GetColTypeName(), iter->GetName(),
                 iter->GetColTypeName(), cnt);

         // this line is something like 
         // colName1 = (*__colName1__)[row];
         sprintf(hstr, "%s = (*__%s__)[row];",
                 iter->GetName(), iter->GetName());
         assignment.push_back(string(hstr));
         cnt++;
         }
      }
   
   // set the row variable
   fprintf(macroFile, "UInt_t row_ = g_flt.GetNumRows(); UInt_t row;\n");

   // the loop over all rows
   fprintf(macroFile, "while(row_){row_--;row=g_flt.Map(row_);\n");
   for (list<string>::iterator i_str = assignment.begin();
        i_str != assignment.end(); i_str++)
      fprintf(macroFile, "%s\n", i_str->c_str());


   // finish the loop and the macro
   fprintf(macroFile, "g_flt[row_]=%s;}}\n", filter);
 
   fclose(macroFile);

   // now process the macro and delete the temporary file
   gROOT->ProcessLine(Form(".x %s", fileName), &err);
//   gROOT->Macro(fileName, &err);
   unlink(fileName);
  
   // was there an error ?
   if (err)
      {
      TFError::SetError("TFRowIter::Filter", 
               "Interpreter error while processing this filter: %s", filter);
      return kFALSE;
      }

   // everything was OK remove the rows in fRow which we don't want
   // any more
   UInt_t from, to = 0;
   for (from = 0; from < fMaxIndex; from++)
      if (g_flt[from])
         fRow[to++] = fRow[from];
   fMaxIndex = to;

   return kTRUE;
}
