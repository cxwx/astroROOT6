// ///////////////////////////////////////////////////////////////////
//
//  File:      TFGroup.cpp
//
//  Version:   1.0
//
//  Author:    Reiner Rohlfs (GADC)
//
//  History:   1.0   04.08.03  first released version
//
// ///////////////////////////////////////////////////////////////////
#include "TSystem.h"

#include "TFGroup.h"
#include "TFImage.h"
#include "TFError.h"


#ifndef TF_CLASS_IMP
#define TF_CLASS_IMP
ClassImp(TFElementPtr)
ClassImp(TFGroup)
#endif    // TF_CLASS_IMP

//_____________________________________________________________________________
//  TFGroup:
//    A TFGroup is a TFTable with a specific column, the TFGroupCol. But any 
//    other column can be added to this group as well. Each row of this 
//    TFGroupCol is a "pointer" to an other TFIOElement. It can point to an 
//    other TFGroup, to a TFTable or to any kind of TFImage. These TFIOElement 
//    to which the pointers point to can be in the same file as this group
//    or in any other file. See TFElementPtr to get more information about
//    these pointers to other TFIOElements. Typically data that logically 
//    belong together but are stored in different tables and image in several 
//    files are organized in this hierarchical groups. The organization
//    of a group can be very complicated, because several pointers from 
//    different groups can point to the same TFIOElement and a pointer can point
//    to a group which again may point to other groups and  elements. It is even 
//    possible, but not recommended, that a group points to an other group that
//    itself has a pointer to the first group.
//    
//    Once a group is opened, for example with TFReadGroup, it is very 
//    easy to get access to its so called children and children of children and
//    so on: With the MakeIterator() - function a group iterator (TFGroupIter)
//    is created which should be used to get C++ - pointers to the children of 
//    the group. It is possible to get the C++ - pointers sorted depending on 
//    one of the other columns of this TFGroup - table. And it is possible to 
//    get only a sub-group of the children. The selection of this sub-group 
//    depends on the name of the children and children of children. See 
//    TFSelector and TFNameSelector for more information how to select a 
//    sub-group.
//
//    Important Note: The name of the specific group column is "_GROUP_". This 
//                    name shall not be used for any other column in this 
//                    group table.
//
//
//  TFElementPtr:
//    This item is the data of one pointer to any kind of TFIOElement. It is 
//    the data of one row of the TFGroupCol. It consist of four information: 
//    The filename of the file in which the element is stored, either 
//    as an absolute path or relative to the group. The element name, the 
//    cycle number of this element in its file and a flag that is set 
//    if the children is a group. Normally it should not be necessary for 
//    an application to read or to write this TFElementPtr. It is done 
//    automatically if a element is attached to a group and if the 
//    TFGroupIter returns a C++ pointer to a child of the group.
//     

const char GROUP_COL_NAME[] =   {"_GROUP_"};


TFGroup *  TFReadGroup(const char * fileName, const char * name,  
                       UInt_t cycle, FMode mode)
{
   TFIOElement * element = TFRead(fileName, name, cycle, mode, TFGroup::Class());

   TFGroup * group = dynamic_cast<TFGroup *>(element);
   if (group == NULL)
      delete element;

   return group;
}



TFElementPtr::TFElementPtr(const TFIOElement * element)
   : fElementName(element->GetName())
{
// constructs a new group item for element.

   const char * fileName = element->GetFileName();
   if (fileName)
      {
      fFileName = fileName;
      fCycle    = element->GetCycle();
      }
   else
      fCycle = 0;

   if (element->IsA()->InheritsFrom(TFGroup::Class()))
      fType = kGroupType;
   else if (element->IsA()->InheritsFrom(TFTable::Class()))
      fType = kTableType;
   else if (element->IsA()->InheritsFrom(TFBaseImage::Class()))
      fType = kImageType;
   else
      fType = kBaseType;
}
//_____________________________________________________________________________
TFElementPtr::TFElementPtr(const TFElementPtr & elementPtr)
   : TObject(elementPtr)
{
//  cop;y constructor

   fFileName      = elementPtr.fFileName;
   fElementName   = elementPtr.fElementName;
   fCycle         = elementPtr.fCycle;
   fType          = elementPtr.fType;
}
//_____________________________________________________________________________
TFElementPtr & TFElementPtr::operator=(const TFElementPtr & elementPtr)
{
// assignment operator

   TObject::operator=(elementPtr);

   fFileName      = elementPtr.fFileName;
   fElementName   = elementPtr.fElementName;
   fCycle         = elementPtr.fCycle;
   fType          = elementPtr.fType;

   return *this;
}
//_____________________________________________________________________________
Bool_t TFElementPtr::operator==(const TFElementPtr & elementPtr) const
{
// returns kTRUE if this and elementPtr are identical. Note: the same file
// name can be expressed in different strings. For example as absolute path
// and as relative path. In this case this operator return kFALSE.

   return fElementName == elementPtr.fElementName &&
          fFileName    == elementPtr.fFileName    &&
          fCycle       == elementPtr.fCycle       &&
          fType        == elementPtr.fType;
}
//_____________________________________________________________________________
Bool_t TFElementPtr::operator< (const TFElementPtr & elementPtr) const
{
// operator to sort TFElementPtrs.

   if (fFileName < elementPtr.fFileName) return kTRUE;
   if (elementPtr.fFileName < fFileName) return kFALSE;

   if (fElementName < elementPtr.fElementName) return kTRUE;
   if (elementPtr.fElementName < fElementName) return kFALSE;

   if (fCycle < elementPtr.fCycle) return kTRUE;
   if (elementPtr.fCycle < fCycle) return kFALSE;

   return fType < elementPtr.fType;
}
//_____________________________________________________________________________
//_____________________________________________________________________________
TFElementIdPtr::TFElementIdPtr(const TFElementPtr & elementPtr, 
                               const char * isRelativeTo)
   : TFElementPtr(elementPtr)
{
// A TFElementIdPtr is a TFElementPtr with a file id which should be uniqe
// This id is used to sort them, which is faster than sorting TFElementPtr

   MakeAbsolutePath(isRelativeTo);
   if (gSystem->GetPathInfo(fFileName.Data(), &fileId, (Long_t*)NULL, NULL, NULL) == 1)
      {
      // try *.fits.gz
      char * fn = new char[fFileName.Length() + 4];
      strcpy(fn, fFileName.Data());
      strcat(fn, ".gz");
      if (gSystem->GetPathInfo(fn, &fileId, (Long_t*)NULL, NULL, NULL) == 1)
         // the file does not exist???
         fileId = -1;
      delete [] fn;
      }
}
//_____________________________________________________________________________
Bool_t TFElementIdPtr::operator< (const TFElementIdPtr & elementPtr) const
{
   if (fileId < elementPtr.fileId) return kTRUE;
   if (elementPtr.fileId < fileId) return kFALSE;

   if (fElementName < elementPtr.fElementName) return kTRUE;
   if (elementPtr.fElementName < fElementName) return kFALSE;

   if (fCycle < elementPtr.fCycle) return kTRUE;
   if (elementPtr.fCycle < fCycle) return kFALSE;

   return fType < elementPtr.fType;
}

//_____________________________________________________________________________
//_____________________________________________________________________________
static const char * dataTypeStr[] = {
"  undef",
"element",
"  image",
"  table",
"  group"
};
char * ElementPtrFormat::Format(char * str, Int_t width, const char * format, 
                                const TFElementPtr & value)
{
// dump function to print all data of one TFElementPtr

   if (width == 0)
      sprintf(str, (format == NULL) ? "%s %3d %26s %s" : format,
              dataTypeStr[value.GetDataType()],
              value.GetCycle(),
              value.GetElementName(),
              value.GetFileName() );
   else
      sprintf(str, (format == NULL) ? "%s %3d %*s %s" : format,
              dataTypeStr[value.GetDataType()],
              value.GetCycle(),
              width, value.GetElementName(),
              value.GetFileName() );
   return str;
}


//_____________________________________________________________________________
//_____________________________________________________________________________
UInt_t TFGroup::Attach(TFIOElement * element, Bool_t relativePath)
{
// Attaches one element to this group. A new row in this group table is crated
// and the pointer of the specific group column will point to the new element.
// The pointer to element will be either relative to the file of this
// group or an absolute path depending on the parameter relativePath. The row 
// number of this new group is returned. It can be used if fill the other columns 
// of this group table if there are any.
// If a pointer to element already exist in this group no second pointer is added
// but the row number of the already existing pointer is returned.
// 
// This group and the new element must be associated to a file. An error is 
// written to TFError if one of them or both are exist only in memory an TF_MAX_ROWS
// is returned.
//
// This attachment is not automatically updated in the file. Call SaveElement()
// to update the group table in the file.

   if (!element->IsFileConnected())
      {
      TFError::SetError("TFGroup::Attach", 
                        "IOElement %s is not connected to a ROOT file."
                        " Cannot attach it to group %s.",
                         element->GetName(), GetName());

      return TF_MAX_ROWS;
      }

   if (!IsFileConnected())
      {
      TFError::SetError("TFGroup::Attach", 
                        "Group %s is not connected to a ROOT file."
                        " Cannot attach IOElements.",
                        GetName());

      return TF_MAX_ROWS;
      }

   
   TFErrorType errT = TFError::GetErrorType();
   TFError::SetErrorType(kNoErr);
   // AddColumn will return the already existing group column or a new 
   // created column.
   TFGroupCol & col = dynamic_cast<TFGroupCol&>
                        (AddColumn(GROUP_COL_NAME, TFGroupCol::Class()));
   TFError::SetErrorType(errT);

   TFElementPtr absItem(element);
#ifdef WIN32
   absItem.MakeAbsolutePath(".\\");
#else 
   absItem.MakeAbsolutePath("./");
#endif

   TFElementPtr relItem(absItem);
   relItem.MakeRelativePath(GetFileName());

   for(UInt_t row = 0; row < fNumRows; row++)
      if ( col[row] == relItem ||
           col[row] == absItem     )
         // this element already exist
         return row;

   InsertRows(1);
   col[fNumRows-1] = relativePath ? relItem : absItem;
   
   return fNumRows-1;
}
//_____________________________________________________________________________
void TFGroup::Detach(TFIOElement * element)
{
// Detaches the element from this group. 
// No error is generated if the element is not a children of this group.
// This detachment is not automatically updated in the file. Call SaveElement()
// to update the group table in the file.

   TFErrorType errT = TFError::GetErrorType();
   TFError::SetErrorType(kExceptionErr);
   try 
      {
      TFGroupCol & col = dynamic_cast<TFGroupCol&>(GetColumn(GROUP_COL_NAME));

      TFElementPtr absItem(element);
      absItem.MakeAbsolutePath(element->GetFileName());

      TFElementPtr relItem(absItem);
      relItem.MakeRelativePath(GetFileName());

      for(ULong_t row = 0; row < fNumRows; row++)
         if ( col[row] == relItem ||
              col[row] == absItem     )
            {
            DeleteRows(1, row);
            break;
            }
      }
   catch (TFException) { }
   TFError::SetErrorType(errT);
}
//_____________________________________________________________________________
TFGroupIter TFGroup::MakeGroupIterator()
{
// Creates an iterator to iterate through all children and children of children 
// of this group. 
// The iterator can be used to get the children sorted and to get only a sub-group.
// For more information how to use this iterator see TFGroupIterator.
//
// The itererator must not be used any more after a new element is added to this
// group or any of the children groups (function Attach)  or an element is removed
// from this group or any of its children groups (function Detach).
   return TFGroupIter(this);
}


