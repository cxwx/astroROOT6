// ////////////////////////////////////////////////////////////////////////////
//
//  File:      TFGroupIterator.cpp
//
//  Version:   1.0
//
//  Author:    Reiner Rohlfs (GADC)
//
//  History:   1.0   12.08.03  first released version
//
// ////////////////////////////////////////////////////////////////////////////
#include <TList.h>

#include "TFGroup.h"
#include "TFError.h"

ClassImp(TFGroupIter)
ClassImp(TFSelector)

//_____________________________________________________________________________
// TFGroupIter:
//    A TFGroupIter should be used to get access to all children of any level
//    of a TFGroup. The Next() function returns kTRUE if there is a further 
//    TFIOElement in the group and kFALSE if there is no TFIOElement any more to
//    "point to". The operator * () and the operator ->() can be used to 
//    access the next TFIOElement of one of its derived classes like a TFTable
//    or a TFImage.
// 
//    The elements are opened "readOnly". To get elements for update call the
//    SetReadOnly() - function before calling the Next() function.
//
//    There are some possibilities to control the number of returned elements and
//    the order of the returned elements from the operator *() and operator ->():
//    A filter can be defined to filter rows of the first TFGroup depending on
//    other columns in the group ( see SetFilter() and Next() ). The rows of the 
//    first group can be sorted depending on the values of an other column of the 
//    group. ( see Sort() and Next() ). The returned elements can be selected by 
//    their names (see SetSelector() and Next() ).
//
// TFSelector:
//    TFSelector is an abstract base class to select pointers of a TFGroup. In the
//    current version there is one derived class to select the pointers depending
//    on the name of the element the group pointer points to. See TFNameSelector.



TFGroupIter::TFGroupIter(TFGroup * group, std::set<TFElementIdPtr> * done, 
                                 TList * select,Bool_t readOnly)
   : fCol((TFGroupCol&)group->GetColumn(GROUP_COL_NAME)), fRowIter(group)
{
// private constructor used by this class only

   fGroup      = group;
   fList       = NULL;
   fReadOnly   = readOnly;
   fLast       = NULL;
   fDone       = done;
   fStart      = kFALSE;
   fSelect     = select;
}
//_____________________________________________________________________________
TFGroupIter::TFGroupIter(TFGroup * group)
   : fCol((TFGroupCol&)group->GetColumn(GROUP_COL_NAME)), fRowIter(group)
{
// private constructor. Use TFGroup::MakeGroupIterator() to create a TFGroupIter

   fGroup      = group;
   fList       = NULL;
   fReadOnly   = kTRUE;
   fLast       = NULL;
   fDone       = new std::set<TFElementIdPtr>;
   fStart      = kTRUE;
   fSelect     = new TList;
}
//_____________________________________________________________________________
TFGroupIter::TFGroupIter(const TFGroupIter & groupIter) 
   : fCol(groupIter.fCol), fRowIter(groupIter.fGroup) 
{
   fGroup         = groupIter.fGroup;
   fReadOnly      = groupIter.fReadOnly;
   fList          = NULL;
   fLast          = NULL;
   fDone          = new std::set<TFElementIdPtr>;
   fStart         = kTRUE;
   fSelect        = new TList;
}
//_____________________________________________________________________________
TFGroupIter::~TFGroupIter()
{
   if (fList)
      {
      delete fList->fGroup;
      delete fList;
      }

   if (fStart)
      {
      delete fDone;

      fSelect->Delete();
      delete fSelect;
      }

   delete fLast;
}
//_____________________________________________________________________________
Bool_t  TFGroupIter::Next()
{
// "Moves" this iterator to the next TFIOElement or one of a dervied class of 
// TFIOElement in a group. After the first time the TFGroupIter "points" to
// the first TFIOElement. This function returns kTRUE as long as there is a
// further TFIOElement in the group. It returns kFALSE if it has already 
// "pointed" to all TFIOElements of the group. The operator *() and the 
// operator->() can be used to access the actual TFIOELement this iterator
// "points to". 
// All elements in all branches of the group are returned but never the same
// element twice. Assume that a child of the group is a group with some 
// children and again some groups as children and so on. All elements that are 
// child of any child - group of this group are searched and returned if they
// pass the selection criteria and the filter.
// Therefore this TFGroupIter never atually "points" to a TFGroup.
//
// Before calling the first time Next() a filter can be set (SetFilter). For more
// information on setting a filter see class TFRowIterator and its member function
// TFRowIterator::SetFilter() This function is directly called by the SetFilter()
// function of this TFGroupIter class. But in any case the filter is only 
// applied to the first TFGroup and not to its children groups.
//
// The order of the returned elements can be sorted with the Sort() function. For
// more information how to sort the rows of a group see class TFRowIterator and its
// member function TFRowIterator::Sort(). This function is directly called by the
// Sort() function of this TFGroupIter class. But in any case only the rows of
// the first TFGroup are sorted not the rows of its children groups.
//
// Further it is possible to select elements the iterator "points to"  by
// the name of the elements. Call one or several times the SetSelector() function to
// define the names of returned elements. For more information how to set the 
// selected element names see class description of TFNameSelector. 

   if (fList)
      {
      if (fList->Next())
         return kTRUE;
      delete fList->fGroup;
      delete fList;            
      fList = NULL;
      }
   
   // delete the element returned at the previous call of this function   
   delete fLast;
   fLast = NULL;

   TIter selectIter(fSelect);

   while ( fRowIter.Next() )
      {
      TFElementIdPtr item( fCol[*fRowIter], fGroup->GetFileName());

      if (item.IsGroup())
         {
         if (fDone->insert(item).second == false)
            // we processed this element already
            continue;
                  
         // we found a group. Go into it and iterate their children
         TFErrorType errT = TFError::GetErrorType();
         TFError::SetErrorType(kNoErr);
         TFGroup * group = TFReadGroup(item.GetFileName(), item.GetElementName(), 
                                       item.GetCycle() );
         TFError::SetErrorType(errT);
         if (!group)
            continue;
         // make a new iterator for the new group and get the next
         // element from this group
         errT = TFError::GetErrorType();
         TFError::SetErrorType(kExceptionErr);
         try {
            TFError::SetErrorType(errT);
            fList = new TFGroupIter(group, fDone, fSelect, fReadOnly);
            if (fList->Next())
               {
               return kTRUE;
               }
            }
         catch (TFException) 
            {
            TFError::SetErrorType(errT);
            }
         

         if (fList) delete fList->fGroup;
         delete fList;            
         fList = NULL;
         continue;
         }

      // check if this element passes the selection criteria
      Bool_t notSelect = kFALSE;
      while (TFSelector * selector = (TFSelector*)selectIter())
         {
         if (!(selector->Select(item)))
            {
            notSelect = kTRUE;
            break;
            }
         }
      selectIter.Reset();
      if (notSelect)
         continue;

      // don't return the same element twice
      if (fDone->insert(item).second == false)
         // we processed this element already
         continue;

      // the next element is not a group, but a normal table or image or ..
      // try to open it
      TFIOElement * element = TFRead(item.GetFileName(), item.GetElementName(), 
                                     item.GetCycle(), fReadOnly ? kFRead : kFReadWrite);
      if (element)
         {
         fLast = element;
         return kTRUE;
         }
      }

  // we are at the end
  return kFALSE;
}
//_____________________________________________________________________________
void TFGroupIter::Reset()
{
// resets the iterator, but not the filter, not the sorting and not the selection
// criteria. The Next() function can be used again to iterate through all 
// TFIOElements.

   delete fList;
   fList = NULL;

   fDone->clear();
   
   fRowIter.Reset();
}
//_____________________________________________________________________________
void TFGroupIter::SetReadOnly(Bool_t readOnly) 
{
// As default the elements returned from the operator * () and the 
// operator->() are opened in readOnly mode. To open them in update 
// mode set readOnly to kFALSE.
   fReadOnly = readOnly;
   if (fList)
      fList->SetReadOnly(readOnly);
}
//_____________________________________________________________________________
void TFGroupIter::ClearSelectors()
{
// clears all selectors set by the SetSelector() function
   if (fStart)
      fSelect->Delete();
}
//_____________________________________________________________________________
void TFGroupIter::SetSelector(TFSelector * select)
{
// Set one selector. The number of elements this iterator "points to" 
// depend on select. For more information see TFNameSelector and the Next() 
// function. 
// select is adapted by this class and will be deleted by this TFGroupIter class.
   fSelect->Add(select);
}
