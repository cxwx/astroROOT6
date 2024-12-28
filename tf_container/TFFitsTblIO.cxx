// ///////////////////////////////////////////////////////////////////
//
//  File:      TFFitsTblIO.cxx
//
//  Version:   1.1.2
//
//  Author:    Reiner Rohlfs (GADC)
//
//  History:   1.0   14.08.03  first released version
//             1.1.1 01.06.04  update checksum after saving the 
//                             columns 
//             1.1.2 11.08.04  do not write to columns with 0 rows
//                   11.08.04  initialize anyNull to 0 (false) to
//                             work also corecctly if the column has
//                             0 rows
//                   11.08.04  Write the FITS TNULL value to the
//                             header of the ROOT column, even if no
//                             NULL value is available in the column.
//                   11.08.04  Write the keyword TNULL to the FITS 
//                             header if the null keyword of the 
//                             column is defined, even if no NULL 
//                             value is available in the column
//                   17.08.04  Write the FITS column number into
//                             an attribute of the ROOT column and
//                             use this attribute to create columns
//                             in expectede order in a FITS table
//
// ///////////////////////////////////////////////////////////////////
#ifdef _SOLARIS
#include <ieeefp.h>
#endif

#include <math.h>
#include <map>
#include <string>

#include "TFFitsIO.h"
#include "TFError.h"
#include "TFTable.h"
#include "TFColumn.h"
#include "TFGroup.h"

#include "fitsio.h"


#ifdef WIN32
#define        DS             '\\'       // Directory Seperator
#else 
#define        DS             '/'        // Directory Seperator
#endif


struct FitsColDef
{
   FitsColDef() {}
   FitsColDef(const char * _tform, int _dataType, long long _nullOffset, long long _defaultNull)
      {
      strcpy(tform, _tform);
      dataType    = _dataType;
      nullOffset  = _nullOffset;
      defaultNull = _defaultNull;
      }

   char        tform[2];
   int         dataType;
   long long   nullOffset;
   long long   defaultNull;
};

std::map<TClass*, FitsColDef> _fitsColDef;


static const char * errMsg[] = {
"Error during reading columns; cfitsio error: %d",
"Cannot delete / insert rows in table %s of file %s FITS error: %d",
"Cannot create new column %s in table %s of file %s FITS error: %d",
"Cannot update column %s in table %s of file %s FITS error: %d"
};

static int CreateFitsColumn(fitsfile * fptr, const TFBaseCol & col);
template<class B, class C> int WriteFitsColumn(fitsfile * fptr, C & col);
template<class B, class C> int WriteFitsArrColumn(fitsfile * fptr, C & col);
static int WriteStringFitsColumn(fitsfile * fptr, TFStringCol & col);
static int WriteFitsGroupColumn(fitsfile * fptr, TFGroupCol & col);


static TFBaseCol * StringColFits2Root(fitsfile * fptr, int col, 
                        const char * colName, long numRows, int width, int * status);
static TFBaseCol * BoolColFits2Root(fitsfile * fptr, int col, 
                        const char * colName, long numRows, int * status);
static TFBaseCol * IntColFits2Root(fitsfile * fptr, int col, 
                        const char * colName, long numRows, int * status);
static TFBaseCol * ShortColFits2Root(fitsfile * fptr, int col, 
                        const char * colName, long numRows, int * status);
static TFBaseCol * ByteColFits2Root(fitsfile * fptr, int col, 
                        const char * colName, long numRows, int * status);
static TFBaseCol * FloatColFits2Root(fitsfile * fptr, int col, 
                        const char * colName, long numRows, int * status);
static TFBaseCol * DoubleColFits2Root(fitsfile * fptr, int col, 
                        const char * colName, long numRows, int * status);

static TFBaseCol * StringArrColFits2Root(fitsfile * fptr, int col, 
                        const char * colName, long numRows, long repeat, int width, int * status);
static TFBaseCol * IntArrColFits2Root(fitsfile * fptr, int col, 
                        const char * colName, long numRows, long repeat, int * status);
static TFBaseCol * ShortArrColFits2Root(fitsfile * fptr, int col, 
                        const char * colName, long numRows, long repeat, int * status);
static TFBaseCol * ByteArrColFits2Root(fitsfile * fptr, int col, 
                        const char * colName, long numRows, long repeat, int * status);
static TFBaseCol * FloatArrColFits2Root(fitsfile * fptr, int col, 
                        const char * colName, long numRows, long repeat, int * status);
static TFBaseCol * DoubleArrColFits2Root(fitsfile * fptr, int col, 
                        const char * colName, long numRows, long repeat, int * status);


//_____________________________________________________________________________
static void InitFitsColDef()
{
// initialize the _fitsColDef map, which is used in several functions of
// this file.
// The FitscolDef struct descrives for each column - class of AstroROOT
// some FITS parameters.
 
   if (_fitsColDef.size() > 0)
      return;

   _fitsColDef[TFBoolCol::Class()] =      FitsColDef("L", TLOGICAL,   0,             0);

   _fitsColDef[TFCharCol::Class()] =      FitsColDef("B", TSHORT,     0,             32767);

   _fitsColDef[TFUCharCol::Class()] =     FitsColDef("B", TBYTE,      0,             255);

   _fitsColDef[TFShortCol::Class()] =     FitsColDef("I", TSHORT,     0,             32767);

   _fitsColDef[TFUShortCol::Class()] =    FitsColDef("U", TUSHORT,    32768,         32767);

   _fitsColDef[TFIntCol::Class()] =       FitsColDef("J", TINT,       0,             2147483647);

   _fitsColDef[TFUIntCol::Class()] =      FitsColDef("V", TUINT,      2147483648LL,  2147483647);

   _fitsColDef[TFFloatCol::Class()] =     FitsColDef("E", TFLOAT,     0,             0);

   _fitsColDef[TFDoubleCol::Class()] =    FitsColDef("D", TDOUBLE,    0,             0);

   _fitsColDef[TFStringCol::Class()] =    FitsColDef("A", TSTRING,    0,             0);


   _fitsColDef[TFBoolArrCol::Class()] =   FitsColDef("L" , TLOGICAL,  0,             0);
                                                                    
   _fitsColDef[TFCharArrCol::Class()] =   FitsColDef("B" , TSHORT,    0,             32767);
                                                                    
   _fitsColDef[TFUCharArrCol::Class()] =  FitsColDef("B" , TBYTE,     0,             255);
                                                                    
   _fitsColDef[TFShortArrCol::Class()] =  FitsColDef("I" , TSHORT,    0,             32767);
                                                                    
   _fitsColDef[TFUShortArrCol::Class()] = FitsColDef("U" , TUSHORT,   32768,         32767);
                                                                    
   _fitsColDef[TFIntArrCol::Class()] =    FitsColDef("J" , TINT,      0,             2147483647);
                                                                    
   _fitsColDef[TFUIntArrCol::Class()] =   FitsColDef("V" , TUINT,     2147483648LL,  2147483647);
                                                                    
   _fitsColDef[TFFloatArrCol::Class()] =  FitsColDef("E" , TFLOAT,    0,             0);
                                                                    
   _fitsColDef[TFDoubleArrCol::Class()] = FitsColDef("D" , TDOUBLE,   0,             0);
}                                                                   

//_____________________________________________________________________________
TFIOElement * MakeTable(fitsfile * fptr, int * status)
{
// creates either a table or a group
   if (*status != 0) return NULL;

   long numRows = 0;
   fits_get_num_rows(fptr, &numRows, status);

   // test if this is a group table;
   // keyword EXTNAME is GROUPING
   char tableName[40];
   char keyName[20];
   strcpy(keyName, "EXTNAME");
   fits_read_key(fptr, TSTRING, keyName, tableName, NULL, status);
   if (*status != 0 || strcmp(tableName, "GROUPING") != 0)
      {
      *status = 0;
      return new TFTable("no name", numRows);
      }

   TFTable * group = new TFGroup("no name", numRows);
   TFGroupCol & grCol = (TFGroupCol&)group->AddColumn(GROUP_COL_NAME, TFGroupCol::Class());

   // read the position (cycle) and version
   int * cycle = new int [numRows];
   int * version = new int [numRows];
   int cycleNulVal;
   int anyNull;
   fits_read_col(fptr, TINT32BIT, 3, 1, 1, numRows, &cycleNulVal, version, &anyNull, status);
   fits_read_col(fptr, TINT32BIT, 4, 1, 1, numRows, &cycleNulVal, cycle, &anyNull, status);
   
   // read the name
   int typecode, minSize;
   long repeat, width;
   fits_get_coltype(fptr, 2, &typecode, &repeat, &width, status);
   minSize = width + 1 > 7 ? width + 1 : 7;
   char ** name;
   name = new char*[numRows];
   for (int row = 0; row < numRows; row++)
      name[row] = new char[minSize];      

   char strNulVal[7];
   strcpy(strNulVal, "\n\r\'\b\"\t");
   fits_read_col(fptr, TSTRING, 2, 1, 1, numRows, strNulVal, name, &anyNull, status);


   // read the data type
   fits_get_coltype(fptr, 1, &typecode, &repeat, &width, status);
   minSize = width + 1 > 7 ? width + 1 : 7;
   char ** dataType;
   dataType = new char*[numRows];
   for (int row = 0; row < numRows; row++)
      dataType[row] = new char[minSize];      

   fits_read_col(fptr, TSTRING, 1, 1, 1, numRows, strNulVal, dataType, &anyNull, status);

   // read the fileName
   fits_get_coltype(fptr, 5, &typecode, &repeat, &width, status);
   minSize = width + 1 > 7 ? width + 1 : 7;
   char ** fileName;
   fileName = new char*[numRows];
   for (int row = 0; row < numRows; row++)
      fileName[row] = new char[minSize];      

   fits_read_col(fptr, TSTRING, 5, 1, 1, numRows, strNulVal, fileName, &anyNull, status);

   char thisFileName[512];
   fits_file_name(fptr, thisFileName, status);
   // remove the path
   char * pos = strrchr(thisFileName, DS);
   if (!pos) 
      pos = thisFileName;
   else
      pos++;

   for (int row = 0; row < numRows; row++)
      {
      // set the data type of this row
      TFDataType dType;
      if (strcmp (name[row], "GROUPING") == 0)
         dType = kGroupType;
      else if (strcmp(dataType[row], "IMAGE") == 0)
         dType = kImageType;
      else if (strcmp(dataType[row], "BINTABLE") == 0)
         dType = kTableType;
      else 
         dType = kBaseType;
      
      // set the cycle number of thiw row.
      // if dataType and name and version is defined use version
      // else use the position
      if (strcmp(name[row],     strNulVal) != 0 &&
          strcmp(dataType[row], strNulVal) != 0 &&
          version[row] > 0                           ) 
         cycle[row] = -version[row];

      grCol[row] = TFElementPtr(strcmp(fileName[row], strNulVal) == 0 ?
                                pos : fileName[row], 
                                name[row], cycle[row], dType);
      }

   delete [] cycle;
   delete [] version;
   for (int row = 0; row < numRows; row++)
      {
      delete [] dataType[row];
      delete [] name[row];
      delete [] fileName[row];
      }
   delete [] dataType;
   delete [] name;
   delete [] fileName;

   if (*status != 0)
      {
      delete group;
      *status = 0;
      group = new TFTable("no name", numRows);
      }

   return group;
}
//_____________________________________________________________________________
int CreateFitsTable(fitsfile* fptr, TFTable * table)
{
   int status = 0;
   if (table->IsA()->InheritsFrom(TFGroup::Class()))
      {
      // delete the previously created table in the TFTable constructor
      fits_delete_hdu(fptr, NULL, &status);
      fits_create_group(fptr, (char*)table->GetName(), GT_ID_ALL_URI, &status);
      }
   else
      {
      char * dummy[1];
      fits_create_tbl(fptr, BINARY_TBL, 0, 0, dummy, dummy, dummy, 
                      (char*)table->GetName(), &status);
      }


   if (status != 0)
      TFError::SetError("CreateFitsTable", 
                        "Cannot create table in file %s. FITS error: %d",
                        fptr->Fptr->filename, status);
  
   return status; 
}
//_____________________________________________________________________________
int SaveTable(fitsfile* fptr, TFTable* table)
{
// We do nothing here. All is done in SaveColumns

   return 0;
}
//_____________________________________________________________________________
UInt_t TFFitsIO::GetNumColumns()
{
   int numCols = 0;
   int status = 0;
   fits_get_num_cols((fitsfile*)fFptr, &numCols, &status);

   // test if this is a group table;
   // keyword EXTNAME is GROUPING
   char tableName[40];
   char keyName[40];
   strcpy(keyName, "EXTNAME");
   fits_read_key((fitsfile*)fFptr, TSTRING, keyName, tableName, NULL, &status);
   if (status == 0 && strcmp(tableName, "GROUPING") == 0)
      numCols -= 6;

   return numCols;
}
//_____________________________________________________________________________
TFBaseCol * TFFitsIO::ReadCol(const char * name)
{
   fitsfile * fptr = (fitsfile*)fFptr;

   int  status = 0;
   int  col;
   long numRows;
   int typecode;
   long repeat;
   long width;

   // get all necessary information of the requested column
   char * tmpName = new char[strlen(name) + 1];
   strcpy(tmpName, name);

   fits_get_colnum(fptr, CASEINSEN, tmpName, &col, &status);
   fits_get_num_rows(fptr, &numRows, &status);
   fits_get_coltype(fptr, col, &typecode, &repeat, &width, &status);
   delete [] tmpName;


   // check if everything is OK
   if (status != 0) 
      return NULL;

   // copy the FITS column into a ROOT column
   TFBaseCol * rootCol = NULL;
   if (repeat == 1)
      {
      if (typecode == TSTRING)
         rootCol = StringColFits2Root(fptr, col, name, numRows, width, &status);
      else if (typecode == TLOGICAL)
         rootCol = BoolColFits2Root(fptr, col, name, numRows, &status);
      else if (typecode == TINT32BIT)
         rootCol = IntColFits2Root(fptr, col, name, numRows, &status);
      else if (typecode == TSHORT)
         rootCol = ShortColFits2Root(fptr, col, name, numRows, &status);
      else if (typecode == TBYTE)
         rootCol = ByteColFits2Root(fptr, col, name, numRows, &status);
      else if (typecode == TFLOAT)
         rootCol = FloatColFits2Root(fptr, col, name, numRows, &status);
      else if (typecode == TDOUBLE)
         rootCol = DoubleColFits2Root(fptr, col, name, numRows, &status);
      }

   else if (repeat > 1)
      {
      if (typecode == TSTRING)
         rootCol = StringColFits2Root(fptr, col, name, numRows, width, &status);
      else if (typecode == TBYTE)
         rootCol = ByteArrColFits2Root(fptr, col, name, numRows, repeat, &status);
      else if (typecode == TSHORT)
         rootCol = ShortArrColFits2Root(fptr, col, name, numRows, repeat, &status);
      else if (typecode == TINT32BIT)
         rootCol = IntArrColFits2Root(fptr, col, name, numRows, repeat, &status);
      else if (typecode == TFLOAT)
         rootCol = FloatArrColFits2Root(fptr, col, name, numRows, repeat,  &status);
      else if (typecode == TDOUBLE)
         rootCol = DoubleArrColFits2Root(fptr, col, name, numRows, repeat,  &status);
      }

   if (rootCol)
      {
      // get column unit
      char keyword[10];
      char colUnit[30];
      int  st = 0;
      sprintf(keyword, "TUNIT%d", col);
      fits_read_keyword(fptr, keyword, colUnit, NULL, &st);
      if (st == 0)
         {
         int len = strlen(colUnit) - 2;
         while (colUnit[len] == ' ') len--;
         colUnit[len+1] = 0;
         rootCol->SetUnit(colUnit + 1);
         }

      // column number 
      rootCol->AddAttribute(TFIntAttr("colum number", col, "", "column number in FITS table"));
      }


   return rootCol;
}
//_____________________________________________________________________________
void TFFitsIO::ReadAllCol(ColList & columns)
{

   fitsfile * fptr = (fitsfile*)fFptr;

   int  status = 0;
   long numRows;
   int numCols = 0;
   fits_get_num_cols(fptr, &numCols, &status);
   fits_get_num_rows(fptr, &numRows, &status);
   for (int col = 1; status == 0 && col <= numCols; col++)
      {
      int typecode;
      long repeat;
      long width;
      fits_get_coltype(fptr, col, &typecode, &repeat, &width, &status);
      
      // get column name
      char keyword[10];
      char colName[100];
      sprintf(keyword, "TTYPE%d", col);
      fits_read_keyword(fptr, keyword, colName, NULL, &status);
      int len = strlen(colName) - 2;
      while (colName[len] == ' ') len--;
      colName[len+1] = 0;

      if (strcmp(colName + 1, "MEMBER_LOCATION") == 0 ||
          strcmp(colName + 1, "MEMBER_NAME")     == 0 ||
          strcmp(colName + 1, "MEMBER_POSITION") == 0 ||
          strcmp(colName + 1, "MEMBER_URI_TYPE") == 0 ||
          strcmp(colName + 1, "MEMBER_VERSION")  == 0 ||
          strcmp(colName + 1, "MEMBER_XTENSION") == 0    )
         continue;


      TNamed tname(colName + 1,"");
      if (columns.find(TFColWrapper(tname)) != columns.end())
         // this column was already read
         continue;

      TFBaseCol * rootCol = NULL;
      if (repeat == 1)
         {
         if (typecode == TSTRING)
            rootCol = StringColFits2Root(fptr, col, colName + 1, numRows, width, &status);
         else if (typecode == TLOGICAL)
            rootCol = BoolColFits2Root(fptr, col, colName + 1, numRows, &status);
         else if (typecode == TINT32BIT)
            rootCol = IntColFits2Root(fptr, col, colName + 1, numRows, &status);
         else if (typecode == TSHORT)
            rootCol = ShortColFits2Root(fptr, col, colName + 1, numRows, &status);
         else if (typecode == TBYTE)
            rootCol = ByteColFits2Root(fptr, col, colName + 1, numRows, &status);
         else if (typecode == TFLOAT)
            rootCol = FloatColFits2Root(fptr, col, colName + 1, numRows, &status);
         else if (typecode == TDOUBLE)
            rootCol = DoubleColFits2Root(fptr, col, colName + 1, numRows, &status);
         else
            {
            printf("code: %d  TBYTE : %d\n", typecode, TBYTE);
            continue;
            }
         }

      else if (repeat > 1)
         {
         if (typecode == TSTRING)
            rootCol = StringColFits2Root(fptr, col, colName + 1, numRows, width, &status);
         else if (typecode == TBYTE)
            rootCol = ByteArrColFits2Root(fptr, col, colName + 1, numRows, repeat, &status);
         else if (typecode == TSHORT)
            rootCol = ShortArrColFits2Root(fptr, col, colName + 1, numRows, repeat, &status);
         else if (typecode == TINT32BIT)
            rootCol = IntArrColFits2Root(fptr, col, colName + 1, numRows, repeat, &status);
         else if (typecode == TFLOAT)
            rootCol = FloatArrColFits2Root(fptr, col, colName + 1, numRows, repeat,  &status);
         else if (typecode == TDOUBLE)
            rootCol = DoubleArrColFits2Root(fptr, col, colName + 1, numRows, repeat,  &status);

         else
            {
            printf("code: %d  repeat : %d width: %d\n", typecode, repeat, width);
            continue;
            }
         }


      if (rootCol != NULL)
         {
         // get column unit
         char keyword[10];
         char colUnit[30];
         int  st = 0;
         sprintf(keyword, "TUNIT%d", col);
         fits_read_keyword(fptr, keyword, colUnit, NULL, &st);
         if (st == 0)
            {
            int len = strlen(colUnit) - 2;
            while (colUnit[len] == ' ') len--;
            colUnit[len+1] = 0;
            rootCol->SetUnit(colUnit + 1);
            }

         // column number 
         rootCol->AddAttribute(TFIntAttr("colum number", col, "",
                                         "column number in FITS table"));

         columns.insert(TFColWrapper(*rootCol));
         }
      }

   if (status != 0)
      TFError::SetError("TFFitsIO::ReadAllCol", errMsg[0], status); 

}
//_____________________________________________________________________________
Int_t TFFitsIO::SaveColumns(ColList & columns, Int_t compLevel)
{

   if (fFptr == NULL)  return 0;
   InitFitsColDef();

   fitsfile * fptr = (fitsfile*)fFptr;
   int status = 0;

   TFTable * table = dynamic_cast<TFTable*>(fElement);
   if (table == NULL)   return -1;

   // first add new columns if necessary:
   // first find all columns which has to be created. Get their FITS column number
   // if defiend and in a second step (second loop) create the columns in the
   // defined order.
   multimap<int, const TFBaseCol *> sortColumn;
   I_ColList i_c = columns.begin();

   TFErrorType prevErrType = TFError::GetErrorType();
   TFError::SetErrorType(kExceptionErr);
   while (i_c != columns.end())
      {
      int colNum;
      fits_get_colnum(fptr, CASESEN, (char*)i_c->GetCol().GetName(), &colNum, &status);
      if (status == COL_NOT_FOUND)
         {
         status = 0;
         int colNum = 1000;
         try {
             TFIntAttr & colAttr = dynamic_cast<TFIntAttr&>(i_c->GetCol().GetAttribute("colum number"));
             colNum = colAttr.GetValue();
             }
         catch ( TFException ) {}
         sortColumn.insert(multimap<int, const TFBaseCol *>::value_type(colNum, &(i_c->GetCol())) );
         }
      i_c++;
      }
   TFError::SetErrorType(prevErrType);
      
   multimap<int, const TFBaseCol *>::iterator i_sortCol = sortColumn.begin();
   while (i_sortCol != sortColumn.end())
      {
      status = CreateFitsColumn(fptr, *(i_sortCol->second));

      if (status != 0)
         {
         TFError::SetError("TFFitsIO::SaveColumns", errMsg[2],
                           i_sortCol->second->GetName(), table->GetName(), 
                           fptr->Fptr->filename, status);
         return -1;
         }
      i_sortCol++;
      }

   // resize the number of rows
   long numFitsRows;
   fits_get_num_rows(fptr, &numFitsRows, &status);

   if (numFitsRows < table->GetNumRows())
      fits_insert_rows(fptr, numFitsRows, table->GetNumRows() - numFitsRows,
                       &status);
   else if (numFitsRows > table->GetNumRows())
      fits_delete_rows(fptr, table->GetNumRows() + 1, 
                       numFitsRows - table->GetNumRows(), &status);
   
   if (status != 0)
      {
      TFError::SetError("TFFitsIO::SaveColumns", errMsg[1],
                        table->GetName(), fptr->Fptr->filename, status);
      return -1;
      }

   // write the values to the FITS columns
   i_c = columns.begin();
   while (i_c != columns.end())
      {
      if (i_c->GetCol().IsA() == TFBoolCol::Class())
         status = WriteFitsColumn<char>
                     (fptr, dynamic_cast<TFBoolCol&>(i_c->GetCol()) ); 
      else if (i_c->GetCol().IsA() == TFCharCol::Class())
         status = WriteFitsColumn<short>
                     (fptr, dynamic_cast<TFCharCol&>(i_c->GetCol()) ); 
      else if (i_c->GetCol().IsA() == TFUCharCol::Class())
         status = WriteFitsColumn<TFUCharCol::value_type>
                     (fptr, dynamic_cast<TFUCharCol&>(i_c->GetCol()) ); 
      else if (i_c->GetCol().IsA() == TFShortCol::Class())
         status = WriteFitsColumn<TFShortCol::value_type>
                     (fptr, dynamic_cast<TFShortCol&>(i_c->GetCol()) ); 
      else if (i_c->GetCol().IsA() == TFUShortCol::Class())
         status = WriteFitsColumn<TFUShortCol::value_type>
                     (fptr, dynamic_cast<TFUShortCol&>(i_c->GetCol()) ); 
      else if (i_c->GetCol().IsA() == TFIntCol::Class())
         status = WriteFitsColumn<TFIntCol::value_type>
                     (fptr, dynamic_cast<TFIntCol&>(i_c->GetCol()) ); 
      else if (i_c->GetCol().IsA() == TFUIntCol::Class())
         status = WriteFitsColumn<TFUIntCol::value_type>
                     (fptr, dynamic_cast<TFUIntCol&>(i_c->GetCol()) ); 
      else if (i_c->GetCol().IsA() == TFFloatCol::Class())
         status = WriteFitsColumn<TFFloatCol::value_type>
                     (fptr, dynamic_cast<TFFloatCol&>(i_c->GetCol()) ); 
      else if (i_c->GetCol().IsA() == TFDoubleCol::Class())
         status = WriteFitsColumn<TFDoubleCol::value_type>
                     (fptr, dynamic_cast<TFDoubleCol&>(i_c->GetCol()) ); 
      else if (i_c->GetCol().IsA() == TFStringCol::Class())
         status = WriteStringFitsColumn(fptr, dynamic_cast<TFStringCol&>(i_c->GetCol()) );

      else if (i_c->GetCol().IsA() == TFBoolArrCol::Class())
         status = WriteFitsArrColumn<char>
                     (fptr, dynamic_cast<TFBoolArrCol&>(i_c->GetCol()) ); 
      else if (i_c->GetCol().IsA() == TFCharArrCol::Class())
         status = WriteFitsArrColumn<short>
                     (fptr, dynamic_cast<TFCharArrCol&>(i_c->GetCol()) ); 
      else if (i_c->GetCol().IsA() == TFUCharArrCol::Class())
         status = WriteFitsArrColumn<TFUCharArrCol::value_type>
                     (fptr, dynamic_cast<TFUCharArrCol&>(i_c->GetCol()) ); 
      else if (i_c->GetCol().IsA() == TFShortArrCol::Class())
         status = WriteFitsArrColumn<TFShortArrCol::value_type>
                     (fptr, dynamic_cast<TFShortArrCol&>(i_c->GetCol()) ); 
      else if (i_c->GetCol().IsA() == TFUShortArrCol::Class())
         status = WriteFitsArrColumn<TFUShortArrCol::value_type>
                     (fptr, dynamic_cast<TFUShortArrCol&>(i_c->GetCol()) ); 
      else if (i_c->GetCol().IsA() == TFIntArrCol::Class())
         status = WriteFitsArrColumn<TFIntArrCol::value_type>
                     (fptr, dynamic_cast<TFIntArrCol&>(i_c->GetCol()) ); 
      else if (i_c->GetCol().IsA() == TFUIntArrCol::Class())
         status = WriteFitsArrColumn<TFUIntArrCol::value_type>
                     (fptr, dynamic_cast<TFUIntArrCol&>(i_c->GetCol()) ); 
      else if (i_c->GetCol().IsA() == TFFloatArrCol::Class())
         status = WriteFitsArrColumn<TFFloatArrCol::value_type>
                     (fptr, dynamic_cast<TFFloatArrCol&>(i_c->GetCol()) ); 
      else if (i_c->GetCol().IsA() == TFDoubleArrCol::Class())
         status = WriteFitsArrColumn<TFDoubleArrCol::value_type>
                     (fptr, dynamic_cast<TFDoubleArrCol&>(i_c->GetCol()) ); 
      else if (i_c->GetCol().IsA() == TFGroupCol::Class())
         status = WriteFitsGroupColumn(fptr, dynamic_cast<TFGroupCol&>(i_c->GetCol()) );

      if (status != 0)
         {
         TFError::SetError("TFFitsIO::SaveColumns", errMsg[3],
                           i_c->GetCol().GetName(), table->GetName(), 
                           fptr->Fptr->filename, status);
         return -1;
         }
      i_c++;
      }

   fits_write_chksum(fptr, &status);
   return 0;
}
//_____________________________________________________________________________
static int CreateFitsColumn(fitsfile * fptr, const TFBaseCol & col)
{
// create one new column int FITS table and sets offset for unsigend int
// and signed char correctly.

   // we do not create the group columns
   if (col.IsA()->InheritsFrom(TFGroupCol::Class()) )
      return 0;

   std::map<TClass*, FitsColDef>::iterator i_fcd;
   i_fcd = _fitsColDef.find(col.IsA());
   if (i_fcd == _fitsColDef.end())   return -1;
   FitsColDef & fcd = i_fcd->second;

   int status = 0;
   char tform[10];

   // string columns are somehting special
   if (col.IsA() == TFStringCol::Class())
      {
      TFErrorType prevErrType = TFError::GetErrorType();
      TFError::SetErrorType(kExceptionErr);
      UInt_t length;
      try {
         TFUIntAttr & attr = dynamic_cast<TFUIntAttr&>(col.GetAttribute("max size"));
         length = attr;
         }
      catch (TFException) {
         // we have to find the maximum size of the strings
         const TFStringCol & strCol = dynamic_cast<const TFStringCol&>(col);
         length = 1;
         for (UInt_t row = 0; row < col.GetNumRows(); row++)
            if (length < strCol[row].Length())
               length = strCol[row].Length();
         }
      TFError::SetErrorType(prevErrType);
      sprintf(tform, "%u%s", length, fcd.tform);
      }
   else
      sprintf(tform, "%d%s", col.GetNumBins(), fcd.tform);

   // here we create the column
   int colNum;
   fits_get_num_cols(fptr, &colNum, &status);
   colNum++;
   fits_insert_col(fptr, colNum, (char*)col.GetName(), tform, &status);

   if (col.IsA() == TFCharCol::Class() ||
       col.IsA() == TFCharArrCol::Class() )
      {
      // make the column to be a signed char
      int colNum;
      fits_get_colnum(fptr, CASESEN, (char*)col.GetName(), &colNum, &status);
      char key[20];
      int val;

      sprintf(key, "TZERO%d", colNum);
      val = -128;
      fits_write_key(fptr, TINT, key, &val, 
                     (char*)"offset for signed integers", &status);

      sprintf(key, "TSCAL%d", colNum);
      val = 1;
      fits_write_key(fptr, TINT, key, &val, (char*)"data are not scaled", 
                     &status);
      }

   return status;
}
//_____________________________________________________________________________
template<class B, class C>
int SetNullValue(fitsfile * fptr, C & col, FitsColDef & fcd, B & nullVal, 
                 int colNum, bool writeNull, int status)
{
// set the TNULL keyword if it does not already exist.
// set parameter nullVal:
//    either  - as defined in TNULL keyword
//    or      - as defined in null - attribute of col
//    or      - a default value.

   if (status != 0)  return status;

   // there is no FITS keyword for real values and logical
   if (fcd.dataType == TFLOAT)
      {
      long nan = 0xffffffff;
      nullVal = (B)(*((float*)(&nan)));
      return status;
      }
   else if (fcd.dataType == TDOUBLE)
      {
      unsigned long long nan = 0xffffffffffffffffLL;
      nullVal = (B)(*((double*)(&nan)));
      return status;
      }
   else if (fcd.dataType == TLOGICAL)
      {
      nullVal = 127;
      return status;
      }

   // exist the TNULL - keyword already?
   char keyword[10];
   char strNullVal[20];
   sprintf(keyword, "TNULL%d", colNum);
   int tmpStatus = 0;
   fits_read_keyword(fptr, keyword, strNullVal, NULL, &tmpStatus);
   if (tmpStatus == 0)
      {
      nullVal = (B)(atoll(strNullVal) + fcd.nullOffset);
      return status;
      }
   // TNULL - keyword does not exist.

   // Is the null value in an attribute of col defined?

   TFErrorType prevErrType = TFError::GetErrorType();
   TFError::SetErrorType(kExceptionErr);
   try {
      TFBaseAttr & nullAttr = col.GetAttribute("null");

      if (nullAttr.IsA() == TFUIntAttr::Class())
         nullVal = (B)((TFUIntAttr&)nullAttr).GetValue();
      else if (nullAttr.IsA() == TFIntAttr::Class())
         nullVal = (B)((TFIntAttr&)nullAttr).GetValue();
      else
         nullVal = 0;

      writeNull = true;
      }
   catch (TFException) {
      nullVal = (B)(fcd.defaultNull + fcd.nullOffset);
      }
   TFError::SetErrorType(prevErrType);

   if (writeNull)
      {
      // set the TNULL keyword in the FITS table
      int keyNullValue = (int)(nullVal - fcd.nullOffset);
      fits_write_key(fptr, TINT, keyword, &keyNullValue, 
                    (char*)"Undefined value for this field", &status);
      // we also have to update the cfitsio internal buffer
      fits_set_btblnull(fptr, colNum, keyNullValue, &status);
      }

   return status;   
}
//_____________________________________________________________________________
template<class B, class C> int WriteFitsColumn(fitsfile * fptr, C & col)
{

   std::map<TClass*, FitsColDef>::iterator i_fcd;
   i_fcd = _fitsColDef.find(col.IsA());
   if (i_fcd == _fitsColDef.end())   return -1;
   FitsColDef & fcd = i_fcd->second;

   int status = 0;   

   int colNum;
   fits_get_colnum(fptr, CASESEN, (char*)col.GetName(), &colNum, &status);
   if (status != 0)
      return status;

   long numData = col.GetNumRows();
   B * buffer = new B [numData];

   for (int row = 0; row < numData; row++)
      buffer[row ] = col[row];

   B nullVal;
   status = SetNullValue(fptr, col, fcd, nullVal, colNum, 
                         col.HasNull(), status);

   if (numData > 0)
      {
      if (col.HasNull())
         {
         TFNullIter i_null = col.MakeNullIterator();
         while (i_null.Next())
            buffer[*i_null] = nullVal;

         fits_write_colnull(fptr, fcd.dataType, colNum, 1, 1, numData, buffer, 
                            &nullVal, &status);
         }
      else
         fits_write_col(fptr, fcd.dataType, colNum, 1, 1, numData, buffer, &status);
      }

   delete [] buffer;
   return status;
}
//_____________________________________________________________________________
template<class B, class C> int WriteFitsArrColumn(fitsfile * fptr, C & col)
{

   std::map<TClass*, FitsColDef>::iterator i_fcd;
   i_fcd = _fitsColDef.find(col.IsA());
   if (i_fcd == _fitsColDef.end())   return -1;
   FitsColDef & fcd = i_fcd->second;

   int status = 0;   

   int colNum;
   fits_get_colnum(fptr, CASESEN, (char*)col.GetName(), &colNum, &status);
   if (status != 0)
      return status;

   long numData = col.GetNumRows() * col.GetNumBins();
   B * buffer = new B [numData];

   int index = 0;
   for (int row = 0; row < col.GetNumRows(); row++)
         for (int bin = 0; bin < col.GetNumBins(); bin++)
            {
            buffer[index] = col[row][bin];
            index++;
            }

   B nullVal;
   status = SetNullValue(fptr, col, fcd, nullVal, colNum, 
                         col.HasNull(), status);

   if (numData > 0)
      {
      if (col.HasNull())
         {
         TFNullIter i_null = col.MakeNullIterator();
         while (i_null.Next())
            buffer[i_null->Row() * col.GetNumBins() + i_null->Bin()] = nullVal;

         fits_write_colnull(fptr, fcd.dataType, colNum, 1, 1, numData, buffer, 
                            &nullVal, &status);
         }
      else
         fits_write_col(fptr, fcd.dataType, colNum, 1, 1, numData, buffer, &status);
      }

   delete [] buffer;
   return status;
}
//_____________________________________________________________________________
static int WriteStringFitsColumn(fitsfile * fptr, TFStringCol & col)
{
   int status = 0;   

   int colNum;
   fits_get_colnum(fptr, CASESEN, (char*)col.GetName(), &colNum, &status);
   if (status != 0)
      return status;

   long repeat;
   long width;
   fits_get_coltype(fptr, colNum, NULL, NULL, &width, &status);

   long numData = col.GetNumRows();
   
   // prepare the data buffer
   int minSize = width + 1 > 7 ? width + 1 : 7;
   char ** buffer;
   buffer = new char*[numData];
   for (int row = 0; row < numData; row++)
      {
      buffer[row] = new char[minSize];      
      strncpy(buffer[row], col[row].Data(), minSize);
      buffer[row][minSize-1] = 0;
      }

   if (col.HasNull())
      {
      char nulVal[7];
      strcpy(nulVal, "\n\r\'\b\"\t");
      TFNullIter i_null = col.MakeNullIterator();
      while (i_null.Next())
         strcpy(buffer[*i_null], nulVal);
   
      fits_write_colnull(fptr, TSTRING, colNum, 1, 1, numData, buffer, 
                         nulVal, &status);
      }
   else
      fits_write_col(fptr, TSTRING, colNum, 1, 1, numData, buffer, &status);

   for (int row = 0; row < numData; row++)
      delete buffer[row];
   delete [] buffer;

   return status;
}
//_____________________________________________________________________________
static int WriteFitsGroupColumn(fitsfile * fptr, TFGroupCol & col)
{
   int status = 0;   

   long numData = col.GetNumRows();
   
   // prepare the data buffers
   long nameSize, dataTypeSize, fileNameSize, uriTypeSize;
   int * cycle       = new int [numData];
   int * version     = new int [numData];
   char ** dataType  = new char* [numData];
   char ** name      = new char* [numData];
   char ** fileName  = new char* [numData];
   char ** uriType   = new char* [numData];

   fits_get_coltype(fptr, 1, NULL, NULL, &dataTypeSize, &status);
   fits_get_coltype(fptr, 2, NULL, NULL, &nameSize,     &status);
   fits_get_coltype(fptr, 5, NULL, NULL, &fileNameSize, &status);
   fits_get_coltype(fptr, 6, NULL, NULL, &uriTypeSize,  &status);

   for (int row = 0; row < numData; row++)
      {
      dataType[row] = new char[dataTypeSize+1];      
      name[row]     = new char[nameSize+1];      
      fileName[row] = new char[fileNameSize+1];      
      uriType[row]  = new char[uriTypeSize+1];      
      }

   
   // fill the data buffers
   for (int row = 0; row < numData; row++)
      {
      const TFElementPtr & el = col[row];

      strncpy(name[row], el.GetElementName(), nameSize);
      name[row][nameSize] = 0;

      strncpy(fileName[row], el.GetFileName(), fileNameSize);
      fileName[row][fileNameSize] = 0;

      strncpy(uriType[row], "URL", uriTypeSize);
      uriType[row][uriTypeSize] = 0;

      switch (el.GetDataType())
         {
         case kImageType:
            strncpy(dataType[row], "IMAGE", dataTypeSize);
            break;
         case kTableType:
            strncpy(dataType[row], "BINTABLE", dataTypeSize);
            break;
         case kGroupType:
            strncpy(dataType[row], "GROUPING", dataTypeSize);
            break;
         default:
            dataType[row][0] = 0;
         }
      dataType[row][dataTypeSize] = 0;

      if (el.GetCycle() < 0)
         {
         version[row] = -el.GetCycle();
         cycle[row]   = 0;
         }
      else
         {
         version[row] = 0;
         cycle[row]   = el.GetCycle();
         }

      }

   // write the data into the FITS file
   fits_write_col(fptr, TSTRING,   1, 1, 1, numData, dataType, &status);
   fits_write_col(fptr, TSTRING,   2, 1, 1, numData, name,     &status);
   fits_write_col(fptr, TINT32BIT, 3, 1, 1, numData, version,  &status);
   fits_write_col(fptr, TINT32BIT, 4, 1, 1, numData, cycle,    &status);
   fits_write_col(fptr, TSTRING,   5, 1, 1, numData, fileName, &status);
   fits_write_col(fptr, TSTRING,   6, 1, 1, numData, uriType,  &status);


   // free the data buffers
   delete [] cycle;
   delete [] version;
   for (int row = 0; row < numData; row++)
      {
      delete [] dataType[row];
      delete [] name[row];
      delete [] fileName[row];
      delete [] uriType[row];
      }
   delete [] dataType;
   delete [] name;
   delete [] fileName;
   delete [] uriType;

   return status;
}
//_____________________________________________________________________________
Int_t TFFitsIO::DeleteColumn(const char * name)
{
// delete one column in file

   fitsfile * fptr = (fitsfile*)fFptr;

   int status = 0;   
   int colNum;
   fits_get_colnum(fptr, CASESEN, (char*)name, &colNum, &status);
   if (status != 0)
      return status;

   fits_delete_col(fptr, colNum, &status);

   return status;
}
//_____________________________________________________________________________
void TFFitsIO::GetColNames(std::map<TString, TNamed> & columns)
{
   fitsfile * fptr = (fitsfile*)fFptr;

   int  status = 0;
   int numCols = 0;
   fits_get_num_cols(fptr, &numCols, &status);

   for (int col = 1; status == 0 && col <= numCols; col++)
      {
      int typecode;
      long repeat;
      long width;
      fits_get_coltype(fptr, col, &typecode, &repeat, &width, &status);
      
      // get column name
      char keyword[10];
      char colName[100];
      sprintf(keyword, "TTYPE%d", col);
      fits_read_keyword(fptr, keyword, colName, NULL, &status);
      int len = strlen(colName) - 2;
      while (colName[len] == ' ') len--;
      colName[len+1] = 0;

      if (strcmp(colName + 1, "MEMBER_LOCATION") == 0 ||
          strcmp(colName + 1, "MEMBER_NAME")     == 0 ||
          strcmp(colName + 1, "MEMBER_POSITION") == 0 ||
          strcmp(colName + 1, "MEMBER_URI_TYPE") == 0 ||
          strcmp(colName + 1, "MEMBER_VERSION")  == 0 ||
          strcmp(colName + 1, "MEMBER_XTENSION") == 0    )
         continue;

      TNamed name;

      if (repeat == 1)
         {
         if (typecode == TSTRING)
            {
            name.SetName(TFStringCol::Class()->GetName());
            name.SetTitle(StringFormat::GetTypeName());
            }
         else if (typecode == TLOGICAL)
            {
            name.SetName(TFBoolCol::Class()->GetName());
            name.SetTitle(BoolFormat::GetTypeName());
            }
         else if (typecode == TINT32BIT)
            {
            // test if it is a unsigned column
            char keyword[10];
            char offset[30];
            sprintf(keyword, "TZERO%d", col);
            fits_read_keyword(fptr, keyword, offset, NULL, &status);
            if (status != 0 || strcmp(offset, "2147483648") != 0)
               {
               status = 0;
               name.SetName(TFIntCol::Class()->GetName());
               name.SetTitle(IntFormat::GetTypeName());
               }
            else
               {
               name.SetName(TFUIntCol::Class()->GetName());
               name.SetTitle(UIntFormat::GetTypeName());
               }
               
            }
         else if (typecode == TSHORT)
            {
            // test if it is a unsigned column
            char keyword[10];
            char offset[30];
            sprintf(keyword, "TZERO%d", col);
            fits_read_keyword(fptr, keyword, offset, NULL, &status);

            if (status != 0 || strcmp(offset, "32768") != 0)
               {
               status = 0;
               name.SetName(TFShortCol::Class()->GetName());
               name.SetTitle(ShortFormat::GetTypeName());
               }
            else
               {
               name.SetName(TFUShortCol::Class()->GetName());
               name.SetTitle(UShortFormat::GetTypeName());
               }
            }
         else if (typecode == TBYTE)
            {
            name.SetName(TFUCharCol::Class()->GetName());
            name.SetTitle(UCharFormat::GetTypeName());
            }
         else if (typecode == TFLOAT)
            {
            name.SetName(TFFloatCol::Class()->GetName());
            name.SetTitle(FloatFormat::GetTypeName());
            }
         else if (typecode == TDOUBLE)
            {
            name.SetName(TFDoubleCol::Class()->GetName());
            name.SetTitle(DoubleFormat::GetTypeName());
            }
         else
            {
            continue;
            }
         }

      else if (repeat > 1)
         {
         if (typecode == TSTRING)
            {
            name.SetName(TFStringCol::Class()->GetName());
            name.SetTitle(StringFormat::GetTypeName());
            }
         else if (typecode == TBYTE)
            {
            name.SetName(TFUCharArrCol::Class()->GetName());
            name.SetTitle(UCharFormat::GetTypeName());
            }
         else if (typecode == TSHORT)
            {
            // test if it is a unsigned column
            char keyword[10];
            char offset[30];
            sprintf(keyword, "TZERO%d", col);
            fits_read_keyword(fptr, keyword, offset, NULL, &status);

            if (status != 0 || strcmp(offset, "32768") != 0)
               {
               status = 0;
               name.SetName(TFShortArrCol::Class()->GetName());
               name.SetTitle(ShortFormat::GetTypeName());
               }
            else
               {
               name.SetName(TFUShortArrCol::Class()->GetName());
               name.SetTitle(UShortFormat::GetTypeName());
               }

            }
         else if (typecode == TFLOAT)
            {
            name.SetName(TFFloatArrCol::Class()->GetName());
            name.SetTitle(StringFormat::GetTypeName());
            }
         else if (typecode == TDOUBLE)
            {
            name.SetName(TFDoubleArrCol::Class()->GetName());
            name.SetTitle(StringFormat::GetTypeName());
            }
         else
            {
            continue;
            }
         }

      columns[colName + 1] = name;
      }
}

//_____________________________________________________________________________
static TFBaseCol * StringColFits2Root(fitsfile * fptr, int col, 
                       const char * colName, long numRows, int width, int * status)
{
   // create a new string column
   TFStringCol * rootCol = new TFStringCol(colName, numRows);

   // write the lenght into the header of the column
   rootCol->AddAttribute(TFUIntAttr("max size", width, "byte", 
                         "maximum size of string in FITS file without terminating 0") );


   // prepare the data buffer
   int minSize = width + 1 > 7 ? width + 1 : 7;
   char ** buffer;
   buffer = new char*[numRows];
   for (int row = 0; row < numRows; row++)
      buffer[row] = new char[minSize];      

   char nulVal[7];
   strcpy(nulVal, "\n\r\'\b\"\t");

   // read the column
   int anyNull = 0;
   fits_read_col(fptr, TSTRING, col, 1, 1, numRows, nulVal, buffer, &anyNull, status);
   
   // copy the strings into the root column
   for (int row = 0; row < numRows; row++)
      {
      if (anyNull && strcmp(nulVal, buffer[row]) == 0)
          rootCol->SetNull(row);
      else
          (*rootCol)[row] = buffer[row];
      delete [] buffer[row];
      }

   // clean memory
   delete [] buffer;

   return rootCol;
}
//_____________________________________________________________________________
static TFBaseCol * BoolColFits2Root(fitsfile * fptr, int col, 
                    const char * colName, long numRows, int * status)
{
   if (*status != 0)  return NULL;

   // create a new bool column
   TFBoolCol * rootCol = new TFBoolCol(colName, numRows);

   // prepare the data buffer
   char * buffer = new char [numRows];
   char nulVal = 127;

   // read the column
   int anyNull = 0;
   fits_read_col(fptr, TLOGICAL, col, 1, 1, numRows, &nulVal, buffer, &anyNull, status);

   
   // copy the buffer into the root column
   for (int row = 0; row < numRows; row++)
      {
      if (anyNull && buffer[row] == nulVal)
         rootCol->SetNull(row);
      else
         (*rootCol)[row] = buffer[row];
      }

   // clean memory
   delete [] buffer;
   return rootCol;
}
//_____________________________________________________________________________
static TFBaseCol * IntColFits2Root(fitsfile * fptr, int col, 
                    const char * colName, long numRows, int * status)
{
   if (*status != 0)  return NULL;

   // test if it is a unsigned column
   char keyword[10];
   char offset[30];
   sprintf(keyword, "TZERO%d", col);
   fits_read_keyword(fptr, keyword, offset, NULL, status);

   if (*status != 0 || strcmp(offset, "2147483648") != 0)
      {
      *status = 0;
      // this is signed integer
      // create a new string column
      TFIntCol * rootCol = new TFIntCol(colName, numRows);

      // prepare the data buffer
      int * buffer = new int [numRows];
      char strNullVal[20];
      int nulVal = 0;
      sprintf(keyword, "TNULL%d", col);
      fits_read_keyword(fptr, keyword, strNullVal, NULL, status);
      if (*status == 0)
         {
         nulVal = atoi(strNullVal);
         // write the FITS NULL value into the header of the column
         rootCol->AddAttribute(TFIntAttr("null", nulVal, "", 
                              "FITS NULL value of this column") );
         }
      else
         *status = 0;


      // read the column
      int anyNull = 0;
      fits_read_col(fptr, TINT32BIT, col, 1, 1, numRows, &nulVal, buffer, &anyNull, status);
      
      // copy the strings into the root column
      for (int row = 0; row < numRows; row++)
         {
         if (anyNull && buffer[row] == nulVal)
             rootCol->SetNull(row);
         (*rootCol)[row] = buffer[row];
         }

      // clean memory
      delete [] buffer;
      return rootCol;
      }
   else
      {
      // this is unsigned integer
      // create a new string column
      TFUIntCol * rootCol = new TFUIntCol(colName, numRows);

      // prepare the data buffer
      unsigned int * buffer = new unsigned int [numRows];
      unsigned int nulVal= 0;
      int anyNull = 0;

      char strNullVal[20];
      sprintf(keyword, "TNULL%d", col);
      fits_read_keyword(fptr, keyword, strNullVal, NULL, status);
      if (*status == 0)
         {
         nulVal = (unsigned int)( atoll(strNullVal) + 2147483648LL);
         // write the FITS NULL value into the header of the column
         rootCol->AddAttribute(TFUIntAttr("null", nulVal, "", 
                              "FITS NULL value of this column") );
         }
      else
         *status = 0;

      // read the column
      fits_read_col(fptr, TUINT, col, 1, 1, numRows, &nulVal, buffer, &anyNull, status);
      
      // copy the strings into the root column
      for (int row = 0; row < numRows; row++)
         {
         if (anyNull && buffer[row] == nulVal)
            rootCol->SetNull(row);
         (*rootCol)[row] = buffer[row];
         }

      // clean memory
      delete [] buffer;
      return rootCol;
      }

}
//_____________________________________________________________________________
static TFBaseCol * ShortColFits2Root(fitsfile * fptr, int col, 
                   const char * colName, long numRows, int * status)
{
   if (*status != 0)  return NULL;

   // test if it is a unsigned column
   char keyword[10];
   char offset[30];
   sprintf(keyword, "TZERO%d", col);
   fits_read_keyword(fptr, keyword, offset, NULL, status);

   if (*status != 0 || strcmp(offset, "32768") != 0)
      {
      // this is a signed short
      *status = 0;
      // create a new short column
      TFShortCol * rootCol = new TFShortCol(colName, numRows);

      // prepare the data buffer
      short * buffer = new short [numRows];
      short nulVal = 0;
      char strNullVal[20];
      sprintf(keyword, "TNULL%d", col);
      fits_read_keyword(fptr, keyword, strNullVal, NULL, status);
      if (*status == 0)
         {
         nulVal = atoi(strNullVal);
         // write the FITS NULL value into the header of the column
         rootCol->AddAttribute(TFIntAttr("null", nulVal, "", 
                              "FITS NULL value of this column") );
         }
      else
         *status = 0;

      // read the column
      int anyNull = 0;
      fits_read_col(fptr, TSHORT, col, 1, 1, numRows, &nulVal, buffer, &anyNull, status);
      
      // copy the data into the root column
      for (int row = 0; row < numRows; row++)
         {
         if (anyNull && buffer[row] == nulVal)
             rootCol->SetNull(row);
         (*rootCol)[row] = buffer[row];
         }

      // clean memory
      delete [] buffer;
      return rootCol;
      }
   else
      {
      // this is a unsigned short
      // create a new short column
      TFUShortCol * rootCol = new TFUShortCol(colName, numRows);

      // prepare the data buffer
      unsigned short * buffer = new unsigned short [numRows];
      unsigned short nulVal = 0;

      char strNullVal[20];
      sprintf(keyword, "TNULL%d", col);
      fits_read_keyword(fptr, keyword, strNullVal, NULL, status);
      if (*status == 0)
         {
         nulVal = (unsigned short)( atoi(strNullVal) + 32768);
         // write the FITS NULL value into the header of the column
         rootCol->AddAttribute(TFUIntAttr("null", nulVal, "", 
                              "FITS NULL value of this column") );
         }
      else
         *status = 0;

      // read the column
      int anyNull = 0;
      fits_read_col(fptr, TUSHORT, col, 1, 1, numRows, &nulVal, buffer, &anyNull, status);
      
      // copy the data into the root column
      for (int row = 0; row < numRows; row++)
         {
         if (anyNull && buffer[row] == nulVal)
             rootCol->SetNull(row);
         (*rootCol)[row] = buffer[row];
         }

      // clean memory
      delete [] buffer;
      return rootCol;
      }
}
//_____________________________________________________________________________
static TFBaseCol * ByteColFits2Root(fitsfile * fptr, int col, 
                   const char * colName, long numRows, int * status)
{
   // create a new short column
   TFUCharCol * rootCol = new TFUCharCol(colName, numRows);

   // prepare the data buffer
   unsigned char * buffer = new unsigned char [numRows];
   unsigned char nulVal = 0;
   char keyword[10];
   char strNullVal[20];
   sprintf(keyword, "TNULL%d", col);
   fits_read_keyword(fptr, keyword, strNullVal, NULL, status);
   if (*status == 0)
      {
      nulVal = (unsigned char)atoi(strNullVal);
      // write the FITS NULL value into the header of the column
      rootCol->AddAttribute(TFUIntAttr("null", nulVal, "", 
                           "FITS NULL value of this column") );
      }
   else
      *status = 0;

   // read the column
   int anyNull = 0;
   fits_read_col(fptr, TBYTE, col, 1, 1, numRows, &nulVal, buffer, &anyNull, status);
   
   // copy the data into the root column
   for (int row = 0; row < numRows; row++)
      {
      if (anyNull && buffer[row] == nulVal)
          rootCol->SetNull(row);
      (*rootCol)[row] = buffer[row];
      }

   // clean memory
   delete [] buffer;

   return rootCol;
}
//_____________________________________________________________________________
static TFBaseCol * FloatColFits2Root(fitsfile * fptr, int col, 
                   const char * colName, long numRows, int * status)
{
   // create a new short column
   TFFloatCol * rootCol = new TFFloatCol(colName, numRows);

   // prepare the data buffer
   float * buffer = new float [numRows];
   long nan = 0xffffffff;
   float nulVal = *((float*)(&nan));

   // read the column
   int anyNull = 0;
   fits_read_col(fptr, TFLOAT, col, 1, 1, numRows, &nulVal, buffer, &anyNull, status);
   
   // copy the data into the root column
   for (int row = 0; row < numRows; row++)
      {
      if (isnan(buffer[row]))
          rootCol->SetNull(row);
      (*rootCol)[row] = buffer[row];
      }

   // clean memory
   delete [] buffer;
   return rootCol;
}
//_____________________________________________________________________________
static TFBaseCol * DoubleColFits2Root(fitsfile * fptr, int col, 
                   const char * colName, long numRows, int * status)
{
   // create a new short column
   TFDoubleCol * rootCol = new TFDoubleCol(colName, numRows);

   // prepare the data buffer
   double * buffer = new double [numRows];
   unsigned long long nan = 0xffffffffffffffffLL;
   double nulVal = *((double*)(&nan));

   // read the column
   int anyNull = 0;
   fits_read_col(fptr, TDOUBLE, col, 1, 1, numRows, &nulVal, buffer, &anyNull, status);
   
   // copy the data into the root column
   for (int row = 0; row < numRows; row++)
      {
      if (isnan(buffer[row]))
          rootCol->SetNull(row);
      (*rootCol)[row] = buffer[row];
      }

   // clean memory
   delete [] buffer;
   return rootCol;
}
//_____________________________________________________________________________
static TFBaseCol * ByteArrColFits2Root(fitsfile * fptr, int col, 
                   const char * colName, long numRows, long repeat, int * status)
{
   // create a new short column
   TFUCharArrCol * rootCol = new TFUCharArrCol(colName, numRows);
   rootCol->SetNumBins(repeat);

   // prepare the data buffer
   unsigned char * buffer = new unsigned char [numRows * repeat];
   unsigned char nulVal = 0;
   char keyword[10];
   char strNullVal[20];
   sprintf(keyword, "TNULL%d", col);
   fits_read_keyword(fptr, keyword, strNullVal, NULL, status);
   if (*status == 0)
      nulVal = (unsigned char)atoi(strNullVal);
   else
      *status = 0;


   // read the column
   int anyNull = 0;
   fits_read_col(fptr, TBYTE, col, 1, 1, numRows * repeat, &nulVal, 
                 buffer, &anyNull, status);
   
   // copy the data into the root column
   int index = 0;
   for (int row = 0; row < numRows; row++)
      for (int bin = 0; bin < repeat; bin++)
         {
         if (anyNull && buffer[index] == nulVal)
             rootCol->SetNull(row, bin);
         (*rootCol)[row][bin] = buffer[index];
         index++;
         }

   // clean memory
   delete [] buffer;

   return rootCol;
}
//_____________________________________________________________________________
static TFBaseCol * ShortArrColFits2Root(fitsfile * fptr, int col, 
                        const char * colName, long numRows, long repeat, int * status)
{
   if (*status != 0)  return NULL;

   // test if it is a unsigned column
   char keyword[10];
   char offset[30];
   sprintf(keyword, "TZERO%d", col);
   fits_read_keyword(fptr, keyword, offset, NULL, status);

   if (*status != 0 || strcmp(offset, "32768") != 0)
      {
      // this is a signed short
      *status = 0;
      // create a new short column
      TFShortArrCol * rootCol = new TFShortArrCol(colName, numRows);
      rootCol->SetNumBins(repeat);

      // prepare the data buffer
      short * buffer = new short [numRows * repeat];
      short nulVal = 0;
      char strNullVal[20];
      sprintf(keyword, "TNULL%d", col);
      fits_read_keyword(fptr, keyword, strNullVal, NULL, status);
      if (*status == 0)
         {
         nulVal = atoi(strNullVal);
         // write the FITS NULL value into the header of the column
         rootCol->AddAttribute(TFIntAttr("null", nulVal, "", 
                              "FITS NULL value of this column") );
         }
      else
         *status = 0;

      // read the column
      int anyNull = 0;
      fits_read_col(fptr, TSHORT, col, 1, 1, numRows * repeat, &nulVal, 
                    buffer, &anyNull, status);

      // copy the data into the root column
      int index = 0;
      for (int row = 0; row < numRows; row++)
         for (int bin = 0; bin < repeat; bin++)
            {
            if (anyNull && buffer[index] == nulVal)
                rootCol->SetNull(row, bin);
            (*rootCol)[row][bin] = buffer[index];
            index++;
            }

      // clean memory
      delete [] buffer;
      return rootCol;
      }
   else
      {
      // this is a unsigned short
      // create a new short column
      TFUShortArrCol * rootCol = new TFUShortArrCol(colName, numRows);
      rootCol->SetNumBins(repeat);

      // prepare the data buffer
      unsigned short * buffer = new unsigned short [numRows * repeat];
      unsigned short nulVal = 0;
      char strNullVal[20];
      sprintf(keyword, "TNULL%d", col);
      fits_read_keyword(fptr, keyword, strNullVal, NULL, status);
      if (*status == 0)
         {
         nulVal = (unsigned short)( atoi(strNullVal) + 32768);
         // write the FITS NULL value into the header of the column
         rootCol->AddAttribute(TFUIntAttr("null", nulVal, "", 
                              "FITS NULL value of this column") );
         }
      else
         *status = 0;

      // read the column
      int anyNull = 0;
      fits_read_col(fptr, TUSHORT, col, 1, 1, numRows * repeat, &nulVal, 
                    buffer, &anyNull, status);
      
      // copy the data into the root column
      int index = 0;
      for (int row = 0; row < numRows; row++)
         for (int bin = 0; bin < repeat; bin++)
            {
            if (anyNull && buffer[index] == nulVal)
                rootCol->SetNull(row, bin);
            (*rootCol)[row][bin] = buffer[index];
            index++;
            }

      // clean memory
      delete [] buffer;
      return rootCol;
      }
}
//_____________________________________________________________________________
static TFBaseCol * IntArrColFits2Root(fitsfile * fptr, int col, 
                        const char * colName, long numRows, long repeat, int * status)
{
   if (*status != 0)  return NULL;

   // test if it is a unsigned column
   char keyword[10];
   char offset[30];
   sprintf(keyword, "TZERO%d", col);
   fits_read_keyword(fptr, keyword, offset, NULL, status);

   if (*status != 0 || strcmp(offset, "2147483648") != 0)
      {
      // this is a signed int
      *status = 0;
      // create a new short column
      TFIntArrCol * rootCol = new TFIntArrCol(colName, numRows);
      rootCol->SetNumBins(repeat);

      // prepare the data buffer
      int * buffer = new int [numRows * repeat];
      int nulVal = 0;
      char strNullVal[20];
      sprintf(keyword, "TNULL%d", col);
      fits_read_keyword(fptr, keyword, strNullVal, NULL, status);
      if (*status == 0)
         {
         nulVal = atoi(strNullVal);
         // write the FITS NULL value into the header of the column
         rootCol->AddAttribute(TFIntAttr("null", nulVal, "", 
                              "FITS NULL value of this column") );
         }
      else
         *status = 0;

      // read the column
      int anyNull = 0;
      fits_read_col(fptr, TINT32BIT, col, 1, 1, numRows * repeat, &nulVal, 
                    buffer, &anyNull, status);

      // copy the data into the root column
      int index = 0;
      for (int row = 0; row < numRows; row++)
         for (int bin = 0; bin < repeat; bin++)
            {
            if (anyNull && buffer[index] == nulVal)
                rootCol->SetNull(row, bin);
            (*rootCol)[row][bin] = buffer[index];
            index++;
            }

      // clean memory
      delete [] buffer;
      return rootCol;
      }
   else
      {
      // this is a unsigned short
      // create a new short column
      TFUIntArrCol * rootCol = new TFUIntArrCol(colName, numRows);
      rootCol->SetNumBins(repeat);

      // prepare the data buffer
      unsigned int * buffer = new unsigned int [numRows * repeat];
      unsigned int nulVal = 0;
      char strNullVal[20];
      sprintf(keyword, "TNULL%d", col);
      fits_read_keyword(fptr, keyword, strNullVal, NULL, status);
      if (*status == 0)
         {
         nulVal = (unsigned int)( atoi(strNullVal) + 2147483648LL);
         // write the FITS NULL value into the header of the column
         rootCol->AddAttribute(TFUIntAttr("null", nulVal, "", 
                              "FITS NULL value of this column") );
         }
      else
         *status = 0;

      // read the column
      int anyNull = 0;
      fits_read_col(fptr, TUINT, col, 1, 1, numRows * repeat, &nulVal, 
                    buffer, &anyNull, status);
      
      // copy the data into the root column
      int index = 0;
      for (int row = 0; row < numRows; row++)
         for (int bin = 0; bin < repeat; bin++)
            {
            if (anyNull && buffer[index] == nulVal)
                rootCol->SetNull(row, bin);
            (*rootCol)[row][bin] = buffer[index];
            index++;
            }

      // clean memory
      delete [] buffer;
      return rootCol;
      }
}
//_____________________________________________________________________________
static TFBaseCol * FloatArrColFits2Root(fitsfile * fptr, int col, 
                        const char * colName, long numRows, long repeat, int * status)
{
   // create a new short column
   TFFloatArrCol * rootCol = new TFFloatArrCol(colName, numRows);
   rootCol->SetNumBins(repeat);

   // prepare the data buffer
   float * buffer = new float [numRows * repeat];
   long nan = 0xffffffff;
   float nulVal = *((float*)(&nan));

   // read the column
   int anyNull = 0;
   fits_read_col(fptr, TFLOAT, col, 1, 1, numRows * repeat, &nulVal, 
                 buffer, &anyNull, status);
   
   // copy the data into the root column
   int index = 0;
   for (int row = 0; row < numRows; row++)
      for (int bin = 0; bin < repeat; bin++)
         {
         if (isnan(buffer[index]))
             rootCol->SetNull(row, bin);
         (*rootCol)[row][bin] = buffer[index];
         index++;
         }

   // clean memory
   delete [] buffer;
   return rootCol;
}
//_____________________________________________________________________________
static TFBaseCol * DoubleArrColFits2Root(fitsfile * fptr, int col, 
                        const char * colName, long numRows, long repeat, int * status)
{
   // create a new short column
   TFDoubleArrCol * rootCol = new TFDoubleArrCol(colName, numRows);
   rootCol->SetNumBins(repeat);

   // prepare the data buffer
   double * buffer = new double [numRows * repeat];
   unsigned long long nan = 0xffffffffffffffffLL;
   double nulVal = *((double*)(&nan));

   // read the column
   int anyNull = 0;
   fits_read_col(fptr, TDOUBLE, col, 1, 1, numRows * repeat, &nulVal, 
                 buffer, &anyNull, status);
   
   // copy the data into the root column
   int index = 0;
   for (int row = 0; row < numRows; row++)
      for (int bin = 0; bin < repeat; bin++)
         {
         if (isnan(buffer[index]))
             rootCol->SetNull(row, bin);
         (*rootCol)[row][bin] = buffer[index];
         index++;
         }

   // clean memory
   delete [] buffer;
   return rootCol;
}
