// ////////////////////////////////////////////////////////////////////////////
//
//  File:      TFTable.cpp
//
//  Version:   1.0
//
//  Author:    Reiner Rohlfs (GADC)
//
//  History:   1.0   17.07.03  first released version
//
// ////////////////////////////////////////////////////////////////////////////
#include <map>

#include <TGraphErrors.h>
#include <TAxis.h>

#include "TFTable.h"
#include "TFColumn.h"
#include "TFError.h"

#ifndef TF_CLASS_IMP
#define TF_CLASS_IMP
ClassImp(TFTable)
ClassImp(TFColIter)
#endif    // TF_CLASS_IMP

//_____________________________________________________________________________
// TFTable:
//    A TFTable consist of any number of columns. The columns can have different
//    data types. All columns in one table have the same number of rows.
//    The column names in one table have to be unique.
//    A TFTable has a name (see TNamed) and a header (see TFHeader) with
//    Attributes (see TFBaseAttr).
//    A TFTable can be created in memory without a reference in a file.
//    But also this kind of table can be saved later in a ASRO file, in a ROOT 
//    file or in a FITS file.
//    A TFTable can be read from a ASRO file, ROOT file and a FITS file at 
//    creation time (see function TFReadTable)
//    All changes of a TFTable are done only in memory. For example adding a 
//    column, changing a value in the table. To save the changes the member
//    function SaveElement() has to be called.
//    An exception to this rule is the deletion of a column ( DeleteColumn() )
//    This function immediately deletes also the column in the file.
//
//
// TFColIter:
//    This iterator can be used to get all columns of a table if the name of 
//    the columns are unknown. 
//    The constructor is private. Use the function TFTable::MakeColIterator()
//    to create a column iterator for a given table. It is faster to use this
//    iterator than to get all columns of a table with the function 
//    TFTable::GetColumn().


TFTable *  TFReadTable(const char * fileName, const char * name,  
                       UInt_t cycle, FMode mode)
{
   TFIOElement * element = TFRead(fileName, name, cycle, mode, TFTable::Class());

   TFTable * table = dynamic_cast<TFTable *>(element);
   if (table == NULL)
      delete element;

   return table;
}


TFTable::TFTable()   
{
// Default constructor. The table has no columns, no rows and is not
// associated with a table in a file.
// Don't use this constructor. The table should have at least a name.

   fNumRows = 0; 
   fReadAll = kFALSE; 
   fAlreadyRead = 0;
}
//_____________________________________________________________________________
TFTable::TFTable(TTree * tree)
      : TFIOElement(tree->GetName())
{
// the table is created from all branches of the tree which store simple
// variables. The othere branches are skipped.
// Warning:
// This function sets the "branch status process" (tree->SetBranchStatus())
// of all branches which can be copied into a TFTable to 1 and set the 
// status of all other branches to 0.
   fNumRows = 0; 
   fReadAll = kFALSE; 
   fAlreadyRead = 0;

   UInt_t rows;
   if (tree->GetEntries() > TF_MAX_ROWS)
      rows = TF_MAX_ROWS;
   else
      rows = (UInt_t)(tree->GetEntries() + 0.5);
   InsertRows(rows);

   TFErrorType prevErrType = TFError::GetErrorType();
   TFError::SetErrorType(kAllErr);

   // create the columns 
   tree->SetBranchStatus("*", 0);
   TObjArray * branches = tree->GetListOfBranches();
   for (int brLoop = 0; brLoop < branches->GetEntriesFast(); brLoop++)
      {
      TBranch * branch = (TBranch*)((*branches)[brLoop]);
      if (branch == NULL)  continue;

      const char * leaf = branch->GetTitle();
      const char * pos;
      if (strchr(leaf, ':') == NULL         &&
          (pos = strchr(leaf, '/')) != NULL    )
         // we can use this branch
         {
         void * branchBuffer;
         try 
            {
            const char * array = strchr(leaf, '[');
            if (array != NULL)
               {
               // an array of numbers per entry
               int size = atoi(array+1);
               if (size == 0) // proberly an array of variable lenght
                  continue;

               switch (*(pos+1))
                  {
                  case 'C' : {
                     TFStringCol & col = AddColumn(branch->GetName(), TFStringCol::Class());
                     branchBuffer = col.GetStringBranchBuffer(size);
                     }
                     break;
                  case 'b' : {
                     TFUCharArrCol & col = AddColumn(branch->GetName(), TFUCharArrCol::Class());
                     col.SetNumBins(size);
                     branchBuffer = col.GetBranchBuffer();
                     }
                     break;
                  case 'B' : {
                     TFCharArrCol & col = AddColumn(branch->GetName(), TFCharArrCol::Class());
                     col.SetNumBins(size);
                     branchBuffer = col.GetBranchBuffer();
                     }
                     break;
                  case 's' : {
                     TFUShortArrCol & col = AddColumn(branch->GetName(), TFUShortArrCol::Class());
                     col.SetNumBins(size);
                     branchBuffer = col.GetBranchBuffer();
                     }
                     break;
                  case 'S' : {
                     TFShortArrCol & col = AddColumn(branch->GetName(), TFShortArrCol::Class());
                     col.SetNumBins(size);
                     branchBuffer = col.GetBranchBuffer();
                     }
                     break;
                  case 'i' : {
                     TFUIntArrCol & col = AddColumn(branch->GetName(), TFUIntArrCol::Class());
                     col.SetNumBins(size);
                     branchBuffer = col.GetBranchBuffer();
                     }
                     break;
                  case 'I' :  {
                     TFIntArrCol & col = AddColumn(branch->GetName(), TFIntArrCol::Class());
                     col.SetNumBins(size);
                     branchBuffer = col.GetBranchBuffer();
                     }
                     break;
                  case 'D' : {
                     TFDoubleArrCol & col = AddColumn(branch->GetName(), TFDoubleArrCol::Class());
                     col.SetNumBins(size);
                     branchBuffer = col.GetBranchBuffer();
                     }
                     break;
                  case 'F' : {
                     TFFloatArrCol & col = AddColumn(branch->GetName(), TFFloatArrCol::Class());
                     col.SetNumBins(size);
                     branchBuffer = col.GetBranchBuffer();
                     }
                     break;
                  default: 
                     continue;
                  }
               }
            else
               {
               // a single number per entry
               switch (*(pos+1))
                  {
                  case 'b' :
                     branchBuffer = AddColumn(branch->GetName(), TFUCharCol::Class()).GetBranchBuffer();
                     break;
                  case 'B' :
                     branchBuffer = AddColumn(branch->GetName(), TFCharCol::Class()).GetBranchBuffer();
                     break;
                  case 's' :
                     branchBuffer = AddColumn(branch->GetName(), TFUShortCol::Class()).GetBranchBuffer();
                     break;
                  case 'S' :
                     branchBuffer = AddColumn(branch->GetName(), TFShortCol::Class()).GetBranchBuffer();
                     break;
                  case 'i' :
                     branchBuffer = AddColumn(branch->GetName(), TFUIntCol::Class()).GetBranchBuffer();
                     break;
                  case 'I' :
                     branchBuffer = AddColumn(branch->GetName(), TFIntCol::Class()).GetBranchBuffer();
                     break;
                  case 'D' :
                     branchBuffer = AddColumn(branch->GetName(), TFDoubleCol::Class()).GetBranchBuffer();
                     break;
                  case 'F' :
                     branchBuffer = AddColumn(branch->GetName(), TFFloatCol::Class()).GetBranchBuffer();
                     break;
                  default: 
                     continue;
                  }
               }
            }
         catch (TFException) 
            { continue; }
         tree->SetBranchStatus(branch->GetName(), 1);
         branch->SetAddress(branchBuffer);
         }
      }

   TFError::SetErrorType(prevErrType);

   // Fill the table columns
   for (UInt_t row = 0; row < fNumRows; row++)
      {
      tree->GetEntry(row);
      for (I_ColList i_c = fColumns.begin(); i_c != fColumns.end(); i_c++)
         i_c->GetCol().CopyBranchBuffer(row);
      }

   // clear the branch buffer of the array columns
   for (I_ColList i_c = fColumns.begin(); i_c != fColumns.end(); i_c++)
      i_c->GetCol().ClearBranchBuffer();

}
//_____________________________________________________________________________
TFTable::TFTable(const char * name, UInt_t numRows) 
            : TFIOElement(name) 
{
// The table has no columns, numRows rows and is not associated with a table 
// in a file. name is the id of the new table. Default value of numRows is 0.

   fNumRows = numRows; 
   fReadAll = kFALSE; 
   fAlreadyRead = 0;
}
//_____________________________________________________________________________
TFTable::TFTable(const char * name, const char * fileName)  
            : TFIOElement(name, fileName) 
{
// A new table is created. It has no columns, no rows but is associated 
// with a table in a file. name is the id of the new table. 
// fileName is the file name of the file with this new table. 
// The file extension define the used file type:
// *.fits, *.fts, *fit and *.fits.gz define a FITS file.
// *.root define a ROOT file and 
// *.asro and anything else define a ASRO file. 
// The file may already exist or will be created. Other objects with the 
// same name can exist without problem in the same file. Each object with 
// the same name has a unique cycle number. Use GetCycle() to ask for this 
// cycle number. Use the global function TFReadTable() to read and 
// update an already existing table from an ASRO file, a ROOT file or a 
// FITS file.

   fNumRows = 0; 
   fReadAll = kFALSE; 
   fAlreadyRead = 0;

   if (fio)
      fio->CreateElement();
}
//_____________________________________________________________________________
TFTable::TFTable(const TFTable & table) 
            : TFIOElement(table)
{
// Copy constructor. But even if the input table is associated with a table
// in a file, this new table is NOT associated with this or an other
// table in a file. The new table exist only in memory.
   
   fNumRows = table.fNumRows;

   TObject * col;
   table.ReadAllCol();
   for (I_ColList i_c = table.fColumns.begin(); i_c != table.fColumns.end(); i_c++)
      fColumns.insert(TFColWrapper((TFBaseCol&)*i_c->GetCol().Clone()));

   fReadAll = kFALSE;
}
//_____________________________________________________________________________
TFTable::~TFTable()
{
// Also the columns of this table are deleted from memory.

   for (I_ColList i_c = fColumns.begin(); i_c != fColumns.end(); i_c++)
     delete &i_c->GetCol();
}
//_____________________________________________________________________________
TFTable & TFTable::operator=(const TFTable & table)
{
// Even if the input table is associated with a table in a file,
// this table is NOT associated with this or an other table in a
// file. If this table was associates with a table in a file before
// this function the file is closed without update. After this function
// call this table exist only in memory, but can be saved in a file
// with the SaveElement() function.

   if (this != &table)
      {
      TFIOElement::operator=(table);

      for (I_ColList i_c = fColumns.begin(); i_c != fColumns.end(); i_c++)
	      delete &i_c->GetCol();
      fColumns.clear();
         
      fNumRows = table.fNumRows;
      fReadAll = kFALSE;
      fAlreadyRead = 0;

      table.ReadAllCol();

      for (I_ColList i_c = table.fColumns.begin(); i_c != table.fColumns.end(); i_c++)
         fColumns.insert(TFColWrapper((TFBaseCol&)*i_c->GetCol().Clone()));

      }
   return *this;
}
//_____________________________________________________________________________
bool TFTable::operator == (const TFHeader & ioelement) const
{
// Two tables are identical if the number of rows and number of columns are
// identical. The columns have to be identical and the attributes of the
// header have to be identical. But the order of the attributes in the header
// may differ.

   if (TFIOElement::operator==(ioelement) == false  ||
       IsA() != ioelement.IsA() )
      return false;

   TFTable & table = (TFTable&)ioelement;
   if (fNumRows != table.fNumRows  ||
       GetNumColumns() != table.GetNumColumns() )
      return false;

   if (GetNumColumns() == 0)
      return true;

   TFColIter i_col1 = MakeColIterator();
   TFColIter i_col2 = table.MakeColIterator();

   while (i_col1.Next() && i_col2.Next())
      if (!(*i_col1 == *i_col2) )
         return false;

   return true;
}
//_____________________________________________________________________________
Int_t TFTable::AddColumn(TFBaseCol * column, Bool_t replace)
{
// Adds one column to the table. The column names in one table have to 
// be unique. If a column with the same name already exist and replace ==
// kTRUE the old column is deleted and replaced by the new one. If 
// replace == kFALSE (the default value) the new column is not added to 
// the table and an error is written to the TFError class.
// The column is adopted to the table and will be deleted by the table
// when the table is deleted from memory.
// The number of rows of the column will be increased or decreased to the
// number of rows of the table, respectively.

   I_ColList i_col;

   if ((i_col = fColumns.find(TFColWrapper(*column))) != fColumns.end() ||
       (i_col = ReadCol(column->GetName())) != fColumns.end()             )
      {
      if (replace)
         {
         delete &i_col->GetCol();
         fColumns.erase(i_col);
         }
      else
         {
         TFError::SetError("TFTable::AddColumn", 
                           "Column %s already exist in table %s. New column rejected",
                           column->GetName(), GetName() );
         return -1;
         }
      }

   UInt_t prevNumRows = column->GetNumRows();
   if (prevNumRows < fNumRows)
      column->InsertRows(fNumRows - prevNumRows, prevNumRows);   

   else if (column->GetNumRows() > fNumRows)
      column->DeleteRows(column->GetNumRows() - fNumRows, fNumRows);

   fColumns.insert(TFColWrapper(*column));

   return 0;
}
//_____________________________________________________________________________
TFBaseCol & TFTable::AddColumn(const char * name, TClass * colDataType, 
                               Bool_t replace)
{
// Creates a new column of type colDataType and adds it to the table. 
// The column names in one table have to be unique. If a column with 
// the same name already exist and replace == kTRUE the old column is 
// deleted and replaced by the new one. If replace == kFALSE (the default
// value) the new column is not added to the table and either TFException
// is thrown or a reference to the already existing column named "name" 
// is returned, depending on a call of TFError::SetErrorType().  
// The column is adopted to the table and will be deleted by the table
// when the table is deleted from memory.
// The number of rows of the new column will be set to the number of rows 
// of the table.
// Use the static member function Class() of the required column data type
// to pass a TClass variable at the second parameter.

   I_ColList i_col;
   TNamed tmp(name, "");

   if ((i_col = fColumns.find(TFColWrapper(tmp))) != fColumns.end() ||
       (i_col = ReadCol(name)) != fColumns.end()             )
      {
      if (replace)
         {
         delete &i_col->GetCol();
         fColumns.erase(i_col);
         }
      else
         {
         TFError::SetError("TFTable::AddColumn", 
                              "Column %s already exist in table %s. New column rejected",
                              name, GetName() );
         return i_col->GetCol();
         }
      }

   TFBaseCol * column = (TFBaseCol*)colDataType->New();
   column->SetName(name);
   column->InsertRows(fNumRows, 0);   
   fColumns.insert(TFColWrapper(*column));

   return *column;
}
//_____________________________________________________________________________
void TFTable::DeleteColumn(const char * name)
{
// Deletes the column with the id name from this table and 
// from memory.
// If the table is associated with a file the column is also immediately 
// deleted in the file without calling the SaveElement() function.
// If a column with the id "name" does not exist in this table the function
// does nothing, no warning and no error message.

   I_ColList i_col;
   Bool_t   fileDel = kFALSE;
   Bool_t   memDel  = kFALSE;

   // delete column in memory
   TNamed tmp(name, "");
   if ((i_col = fColumns.find(TFColWrapper(tmp))) != fColumns.end())
      {
      delete &i_col->GetCol();
      fColumns.erase(i_col);
      memDel = kTRUE;
      }

   // delete column in the file
   if ( fio ) 
      fileDel = fio->DeleteColumn(name) == 0;

   if(memDel && fileDel && fAlreadyRead > 0)
      fAlreadyRead--;

}
//_____________________________________________________________________________
TFBaseCol & TFTable::GetColumn(const char * name) const
{
// Gets a reference of the column with name "name". The column is still
// adopted to the table and will be deleted from memory when the
// table is deleted.
// If no column with name "name" exist in the table a TFException is thrown 
// or a reference to a NULL pointer is returned, depending on a call of 
// TFError::SetErrorType().  

   TNamed tmp(name, "");
   I_ColList i_col = fColumns.find(TFColWrapper(tmp));
   if (i_col != fColumns.end())
      return i_col->GetCol();
   
   i_col = ReadCol(name);
   if (i_col == fColumns.end())
      {
      TFError::SetError("TFTable::GetColumn", 
        	                   "Column %s does not exist in table %s.",
                            name, GetName() );
      return *((TFBaseCol*)0);
      }

   return i_col->GetCol();
}
//_____________________________________________________________________________
TFColIter TFTable::MakeColIterator() const
{
// Returns an iterator for all columns. This iterator can be used
// to retrieve all columns if their names are unknown.

   ReadAllCol();
   return TFColIter(&fColumns);
}
//_____________________________________________________________________________
TFRowIter TFTable::MakeRowIterator() const
{
// Returns an iterator for all rows of this table. This iterator can
// be used to sort the rows depending on a column and to filter rows.
// The filter can be expressed with a c - statement and can depend on
// values in any column and on row number. See TFRowIter for more
// information on sorting and filter.

   return TFRowIter(this);
}
//_____________________________________________________________________________
void TFTable::InsertRows(UInt_t numRows, UInt_t pos)
{
// Insert rows into the table. All columns of the table are updated.
// pos is the first index of the new rows. If pos is greater than the
// actual number of rows in the table all new rows are inserted at the
// end of the table. The index of the row number is 0 based. 
// The table in the file is not updated. Use the SaveElement() to
// update the table in the ASRO, ROOT or FITS file.

   TFBaseCol * col;

   ReadAllCol();
   
   if (pos > fNumRows)
      pos = fNumRows;

   for (I_ColList i_c = fColumns.begin(); i_c != fColumns.end(); i_c++)
      i_c->GetCol().InsertRows(numRows, pos);

   fNumRows += numRows;
}
//_____________________________________________________________________________
void TFTable::DeleteRows(UInt_t numRows, UInt_t pos)
{
// Delete numRows from all columns. The first removed column has the
// index pos. The index of the row numbers is 0 based. 
// If pos + numRows is greater than the actual number of rows all
// rows beginning with row number pos are deleted. If pos is greater
// or equal than the actual number of rows than no rows are deleted.
// But if pos == TF_MAX_ROWS the last numRows of the table are removed.

   TFBaseCol * col;

   if (pos == TF_MAX_ROWS)
      {
      if(numRows < fNumRows)
         pos = fNumRows - numRows;
      else
         pos = 0;
      }

   if (pos >= fNumRows)
      return;

   if (pos + numRows > fNumRows)
      numRows = fNumRows - pos;

   ReadAllCol();

   for (I_ColList i_c = fColumns.begin(); i_c != fColumns.end(); i_c++)
      i_c->GetCol().DeleteRows(numRows, pos);

   fNumRows -= numRows;
}
//_____________________________________________________________________________
UInt_t TFTable::GetNumColumns() const
{
// Returns the actual number of columns in the table (in memory and in 
// file)

   UInt_t num = 0;
   if (!fReadAll && fio)
      num = fio->GetNumColumns() - fAlreadyRead;
   
   return fColumns.size() + num;
}
//_____________________________________________________________________________
void TFTable::Reserve(UInt_t rows)
{
// To avoid memory movements each time a new row is inserted
// into the table some additional rows are allocated more than 
// necessary. These additional rows are used the next time new
// rows are inserted. "rows" defines the maximum number of rows that 
// can be used before new memory will be allocated. 

   ReadAllCol();
   for (I_ColList i_c = fColumns.begin(); i_c != fColumns.end(); i_c++)
      i_c->GetCol().Reserve(rows);
}
//_____________________________________________________________________________
I_ColList TFTable::ReadCol(const char * name) const
{
// reads one column from the ASRO, ROOT or FITS file into memory.
// private function. Use GetColumn to retrieve a column from this table

   I_ColList i_c = fColumns.end();

   if (!fReadAll && fio)
      {
      TFBaseCol * col = fio->ReadCol(name);
      if (col)
         {
         i_c = fColumns.insert(TFColWrapper(*col)).first;
         fAlreadyRead++;
         }
      }

   return i_c;
}
//_____________________________________________________________________________
void TFTable::ReadAllCol() const
{
// Reads all not yet read columns from the ASRO, ROOT or FITS file into memory
// Private function, called automatically if needed.
   
   if (!fReadAll && fio)
      {
      fio->ReadAllCol(fColumns);
      fReadAll = kTRUE;
      }
}
//_____________________________________________________________________________
Int_t TFTable::SaveElement(const char * fileName, Int_t compLevel)
{
// Updates the table in the ASRO, ROOT or FITS file with the data of this 
// table in memory. If fileName is defined the old file is closed and the 
// table is written into this new file named fileName.
// compLevel defines the compression level in the ASRO and ROOT file. It is
// not used for FITS files. To set compLevel and to update the table in the 
// same file set fileName to an empty string "".
// This function without any parameter has to be used to update the
// ASRO, the ROOT or the FITS file with any change of the table.
// This function does nothing if the table was opened with kFRead 

   if (TFIOElement::SaveElement(fileName, compLevel) != 0)
      return -1;

   Int_t err = 0;
   if (fio && fFileAccess == kFReadWrite )
      {
      err = fio->SaveColumns(fColumns, compLevel);
      fAlreadyRead = fColumns.size();
      }

   return err;
}
//_____________________________________________________________________________
Int_t TFTable::DeleteElement(Bool_t updateMemory)
{
// Deletes the table in the file, but not in memory.
// Closes the file and deletes the file if this table is the
// last element in the file.
// Note: Some columns of the table may not yet be read from the table
//       and are not in memory. Set updateMemory to kTRUE to have access 
//       to all columns of this table after it is deleted in the file.
//       The default value of updateMemeory is kFALSE.

   if (updateMemory)
      ReadAllCol();

   return TFIOElement::DeleteElement();
}
//_____________________________________________________________________________
TTree * TFTable::MakeTree(TFNameConvert * nameConvert) const
{
// creates a TTree from all basic column of this table. Each
// column is copied in one branch of the tree. The tree gets
// the name of this table, each branch gets the name of its 
// column. 
// nameConvert can be NULL. But it will be adopted by this
// function and will be deleted by this function if it is not NULL.
// The calling function has to delete the returning tree.

   if (nameConvert == NULL)
      nameConvert = new TFNameConvert();

   TTree * tree= new TTree(nameConvert->Conv(GetName()), nameConvert->Conv(GetName()) );

   ReadAllCol();
  
   // create the tree with one branch per column
   for (I_ColList i_c = fColumns.begin(); i_c != fColumns.end(); i_c++)
      i_c->GetCol().MakeBranch(tree, nameConvert);

   // fill the tree row by row
   for (UInt_t row = 0; row < fNumRows; row++)
      {
      for (I_ColList i_c = fColumns.begin(); i_c != fColumns.end(); i_c++)
         i_c->GetCol().FillBranchBuffer(row);
      tree->Fill();
      }

   // clear the branch buffer of the array columns
   for (I_ColList i_c = fColumns.begin(); i_c != fColumns.end(); i_c++)
      i_c->GetCol().ClearBranchBuffer();

   delete nameConvert;

   return tree;
}
//_____________________________________________________________________________
TGraphErrors * TFTable::MakeGraph(const char * xCol, const char * yCol,
                                 const char * xErrCol, const char * yErrCol,
                                 TGraphErrors * graph)
{
// A TGraphErrors is created from all data of two columns and two further
// columns can be used for the errors. The column names, used to fill the
// graph has to be passed as the first 4 parameters. 
// It is possible to use columns of different tables: The first time call
// MakeGraph with graph == NULL and some column names may be set to NULL
// as well. Call MakeGraph of the further tables and pass the returned graph of
// the first call as the 5th parameter. At any call column names
// can be set to NULL or "". These are not used to fill the graph.
// The number of points of the graph is the number of rows of the first 
// table. 
// It is not necessary to derfine columns for the X - and Y - errors.
// Null values in a column are set to 0 in the graph, for the X - and Y 
// values as well as for the errors. 
// 
// If the first table has less than 2 rows or if a column does not exist
// in the table an error message is written to TFError.

   if (graph == NULL)
      {
      // make a new graph
      if (GetNumRows() < 2)
         {
         TFError::SetError("TFTable::MakeGraph", 
                           "Number of rows (%u) of table %s is too small to"
                           " create a TGraphError.", GetNumRows(), GetName());

         return NULL;
         }
      
      graph = new TGraphErrors(GetNumRows());
      graph->SetTitle(GetName());
      }
   
   TFErrorType prevErrType = TFError::GetErrorType();
   TFError::SetErrorType(kAllErr);

   // number of copied rows is the minimum of number of rows of this table
   // and number of values in graph
   UInt_t numRows = GetNumRows() < graph->GetN() ? GetNumRows() : graph->GetN();

   if (xCol != NULL && xCol[0] != 0)
      {
      try
         {
         // fill the X - axis
         const TFBaseCol & col = GetColumn(xCol);
         double * axis = graph->GetX();
         for (UInt_t row = 0; row < numRows; row++)
            axis[row] = col[row];

         // set NULL values to 0
         TFNullIter i_null = col.MakeNullIterator();
         while (i_null.Next())
            axis[*i_null] = 0;
         }
      catch (TFException) { }
      }

   if (yCol != NULL && yCol[0] != 0)
      {
      try
         {
         // fill the Y - axis
         const TFBaseCol & col = GetColumn(yCol);
         double * axis = graph->GetY();
         for (UInt_t row = 0; row < numRows; row++)
            axis[row] = col[row];

         // set NULL values to 0
         TFNullIter i_null = col.MakeNullIterator();
         while (i_null.Next())
            axis[*i_null] = 0;
         }
      catch (TFException) { }
      }

   if (xErrCol != NULL && xErrCol[0] != 0)
      {
      try
         {
         // fill the X - errors
         const TFBaseCol & col = GetColumn(xErrCol);
         double * axis = graph->GetEX();
         for (UInt_t row = 0; row < numRows; row++)
            axis[row] = col[row];

         // set NULL values to 0
         TFNullIter i_null = col.MakeNullIterator();
         while (i_null.Next())
            axis[*i_null] = 0;
         }
      catch (TFException) { }
      }

   if (yErrCol != NULL && yErrCol[0] != 0)
      {
      try
         {
         // fill the Y - errors 
         const TFBaseCol & col = GetColumn(yErrCol);
         double * axis = graph->GetEY();
         for (UInt_t row = 0; row < numRows; row++)
            axis[row] = col[row];

         // set NULL values to 0
         TFNullIter i_null = col.MakeNullIterator();
         while (i_null.Next())
            axis[*i_null] = 0;
         }
      catch (TFException) { }
      }

   TFError::SetErrorType(prevErrType);

   // try to set the axis labels
   if (xCol != NULL && xCol[0] != 0 &&
       yCol != NULL && yCol[0] != 0    )
      {
      TAxis * axis = graph->GetXaxis();
      if (axis)  axis->SetTitle(xCol);
      axis = graph->GetYaxis();
      if (axis)  axis->SetTitle(yCol);
      }


   return graph;
}
//_____________________________________________________________________________
void TFTable::Print(const Option_t* option) const
{
   TFIOElement::Print(option);

   printf("\n  number of rows: %u  number of columns: %u\n",  
      GetNumRows(), GetNumColumns() );

   if (option[0] != 0 &&
       strchr(option, 'c') == NULL &&
       strchr(option, 'C') == NULL    )
      // do not print columns 
      return;

   std::map<TString, TNamed> columns;

   for (I_ColList i_c = fColumns.begin(); i_c != fColumns.end(); i_c++)
      columns[i_c->GetCol().GetName()] = TNamed(i_c->GetCol().GetColTypeName(),
                                                i_c->GetCol().GetTypeName() );

   if (fio)
      fio->GetColNames(columns);

   std::map<TString, TString> typedefNames;
   typedefNames[TFBoolArrCol::Class()->GetName()] = "TFBoolArrCol";
   typedefNames[TFCharArrCol::Class()->GetName()] = "TFCharArrCol";
   typedefNames[TFUCharArrCol::Class()->GetName()] = "TFUCharArrCol";
   typedefNames[TFShortArrCol::Class()->GetName()] = "TFShortArrCol";
   typedefNames[TFUShortArrCol::Class()->GetName()] = "TFUShortArrCol";
   typedefNames[TFIntArrCol::Class()->GetName()] = "TFIntArrCol";
   typedefNames[TFUIntArrCol::Class()->GetName()] = "TFUIntArrCol";
   typedefNames[TFFloatArrCol::Class()->GetName()] = "TFFloatArrCol";
   typedefNames[TFDoubleArrCol::Class()->GetName()] = "TFDoubleArrCol";
   typedefNames[TFBoolCol::Class()->GetName()] = "TFBoolCol";
   typedefNames[TFCharCol::Class()->GetName()] = "TFCharCol";
   typedefNames[TFUCharCol::Class()->GetName()] = "TFUCharCol";
   typedefNames[TFShortCol::Class()->GetName()] = "TFShortCol";
   typedefNames[TFUShortCol::Class()->GetName()] = "TFUShortCol";
   typedefNames[TFIntCol::Class()->GetName()] = "TFIntCol";
   typedefNames[TFUIntCol::Class()->GetName()] = "TFUIntCol";
   typedefNames[TFFloatCol::Class()->GetName()] = "TFFloatCol";
   typedefNames[TFDoubleCol::Class()->GetName()] = "TFDoubleCol";


   std::map<TString, TNamed>::const_iterator i_col;
   for (i_col = columns.begin(); i_col != columns.end(); i_col++)
      if (typedefNames.find(i_col->second.GetName()) == typedefNames.end())
         printf("%-20s %-40s %16s %s\n", 
           i_col->first.Data(), i_col->second.GetName(), "", i_col->second.GetTitle());
      else
         printf("%-20s %-40s %-16s %s\n", 
           i_col->first.Data(), 
           i_col->second.GetName(), 
           typedefNames[i_col->second.GetName()].Data(), 
           i_col->second.GetTitle());

}

