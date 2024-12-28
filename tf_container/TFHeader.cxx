// ///////////////////////////////////////////////////////////////////
//
//  File:      TFHeader.cxx
//
//  Version:   1.0
//
//  Author:    Reiner Rohlfs (GADC)
//
//  History:   1.0   11.07.03  first released version
//
// ///////////////////////////////////////////////////////////////////
#include "TClass.h"

#include "TFHeader.h"
#include "TFError.h"

#ifndef TF_CLASS_IMP
#define TF_CLASS_IMP
ClassImp(TFBaseAttr)
ClassImp2T(TFAttr, T, F)
ClassImp(TFHeader)
ClassImp(TFAttrIter)
#endif

typedef std::list<TFBaseAttr*>::const_iterator I_Attr;

#ifndef ERRMSG_H   // Sun compiler includes this file several times
#define ERRMSG_H
static const char * errmsg_h[] = 
{ "Attribute with name %s does not exit in header (index = %u)"};
#endif

//_____________________________________________________________________________
// TFHeader:
//    TFHeader is a header for data organized in tables or image arrays
//    It is the metadata of the data it belongs to. A TFHeader consist of
//    a list of attributes. 
//
// TFBaseAttr, TFAttr:
//    The base class of attributes is TFBaseAttr. An attribute consist of 
//    a name, a value, a unit and a comment. The title of TNamed is used 
//    as the unit.
//
//    Data type specific attributes are implemented as template class (TFAttr). 
//    typedef for fife data types are defined: TFBoolAttr, TFIntAttr, 
//    TFUIntAttr, TFDoubleAttr and TFStringAttr.
//
//
// TFAttrIter:
//    The iterator TFAttrIter should be used to loop through all attributes
//    of a AstroROOT container like a table, a column or an image.


TFBaseAttr::TFBaseAttr(const TFBaseAttr & attribute)
   : TNamed(attribute)
{
   fComment = attribute.fComment;
}
//_____________________________________________________________________________
TFBaseAttr & TFBaseAttr::operator = (const TFBaseAttr & attribute)
{
   TNamed::operator=(attribute);

   fComment = attribute.fComment;
   return *this;
}
//_____________________________________________________________________________
bool TFBaseAttr::operator == (const TFBaseAttr & col) const
{
   return  fName    == col.fName    &&
           fTitle   == col.fTitle   &&
           fComment == col.fComment;
}
//_____________________________________________________________________________
//_____________________________________________________________________________
TFHeader::TFHeader(const TFHeader & header)
{

   I_Attr i_attr = header.fAttr.begin();
   while (i_attr != header.fAttr.end())
      { 
      fAttr.push_back((TFBaseAttr*)(*i_attr)->Clone()); 
      i_attr++;
      }
}
//_____________________________________________________________________________
TFHeader::~TFHeader()
{
// deletes also all attributes in the header

   for (I_Attr i_attr = fAttr.begin(); i_attr != fAttr.end(); i_attr++)
      delete *i_attr;

}
//_____________________________________________________________________________
TFHeader & TFHeader::operator = (const TFHeader & header) 
{
// deletes the attributes and copies all attributes of "header" into
// this header.

   if (this != &header)
      {
      for (I_Attr i_attr = fAttr.begin(); i_attr != fAttr.end(); i_attr++)
         delete *i_attr;
      fAttr.clear();

      I_Attr i_attr = header.fAttr.begin();
      while (i_attr != header.fAttr.end())
         { 
         fAttr.push_back((TFBaseAttr*)(*i_attr)->Clone()); 
         i_attr++;
         }
      }
   return *this;
}
//_____________________________________________________________________________
bool TFHeader::operator == (const TFHeader & header) const
{
// Return true if the number of attributes for this TFHeader and for 
// header is identical and if for each attribute of this TFHeader exist
// an identical attribute in header. The order of the attributes may
// be different.

   if (fAttr.size() != header.fAttr.size())
      return false;

   for (I_Attr i_attr1 = fAttr.begin(); i_attr1 != fAttr.end(); i_attr1++)
      {
      I_Attr i_attr2 = header.fAttr.begin();
      bool found = false;
      while (i_attr2 != header.fAttr.end())
         { 
         if ( (*i_attr1)->IsA() == (*i_attr2)->IsA()  &&
              **i_attr1 == **i_attr2)
            {
            found = true;
            break;
            }
         }
      if (!found)
         return false;
      }
   
   return true;   
}
//_____________________________________________________________________________
void TFHeader::AddAttribute(const TFBaseAttr & attr, Bool_t replace)
{
// Adds one attribute to this header. A copy of attr is added not
// attr itself. More than one attribute with the same name can be
// in the list of a header. If replace is set to kTRUE, the default 
// value, all existing attributes with the same name as attr are 
// removed. To have more than one attribute with the same name in this
// header replace has to be set to kFALSE.

   if (replace)
      DelAttribute(attr.GetName());

   fAttr.push_back((TFBaseAttr*)(attr.Clone())); 
}
//_____________________________________________________________________________
TFBaseAttr & TFHeader::GetAttribute(const char * key, UInt_t index) const
{
// Returns one attribute or throws a TFAttrException if the attribute 
// with the specified key and index is not in this header.
// If key is defined ( not NULL) only attributes with this name are
// returned. index defines the number of attributes with the name 
// "key" that are skipped before attribute is returned.
// The default value of index is 0.
// To return all attributes independent of the name a loop like this can
// be used:
//
//      try {
//         UInt_t index = 0;
//         while (1) {
//            TFBaseAttr & attr = header->GetAttribute(NULL, index);
//            // do something
//            index++;
//            }
//         }
//      catch ( TFAttrException ) {}
//
// To return all attribute of a given name a similar loop can be used, but
// GetAttribute(NULL, index) has to be replace by 
// GetAttribute("attr_name", index))
//
// Better and faster to get all attributes is the use of the  
// attribute iterator, see MakeAttrIterator()
//
// This functions either throws an exception or returns a dummy attribute 
// if the requested attribute does not exist. For more details see
// the user manual (Error Handling and Exceptions).

   UInt_t in_index = index;
   for (I_Attr i_attr = fAttr.begin(); i_attr != fAttr.end(); i_attr++)
      if (key == NULL || strcmp(key, (*i_attr)->GetName()) == 0 )
         {
         if (index == 0)
            // we found it
            return **i_attr;
         index--;
         }

   // here we have a problem: The attribute does not exist.
   // either throw an exception or return an "error" - attribute
   TFError::SetError("TFHeader::GetAttribute", errmsg_h[0], key, in_index);
   char msg[100];
   sprintf(msg, "Attribute %s does not exist", key);
   static TFBoolAttr errAttr("Error", false, "", msg);
   return errAttr;
}
//_____________________________________________________________________________
void TFHeader::DelAttribute(const char * key, Int_t index)
{
// Deletes one attribute or all attributes with the name "key".
// If index < 0, the default value, all attributes with the name
// "key" are deleted. If index >= 0 only one attribute is deleted.
// index defines the number of attributes with name key that are skipped
// before one attribute is deleted.
// If key is not defined ( set to NULL ) all attributes ( index < 0 ) or
// one attribute independent of its name is deleted.

   std::list<TFBaseAttr*>::iterator i_attr = fAttr.begin(); 
   while (i_attr != fAttr.end())
      {
      if (key == NULL || key[0] == 0 ||
         strcmp(key, (*i_attr)->GetName()) == 0    )
         {
         if (index <= 0)
            {
            delete *i_attr;
            i_attr = fAttr.erase(i_attr);
            if (index == 0)
               return;
            continue;
            }
         index--;
         }
      i_attr++;
      }
}
//_____________________________________________________________________________
UInt_t TFHeader::GetNumAttributes(const char * key) const
{
// Returns the number of attributes in this header with the name "key"
// If key is NULL (the default) the number of all attributes is returned.

   if (key == NULL)
      return fAttr.size();

   UInt_t   num = 0;
   for (I_Attr i_attr = fAttr.begin(); i_attr != fAttr.end(); i_attr++)
      if (strcmp(key, (*i_attr)->GetName()) == 0)
         num++;

   return num;
}
//_____________________________________________________________________________
TFAttrIter TFHeader::MakeAttrIterator() const
{
// Returns an iterator for all attributes. This iterater can be used if the 
// names of the attributes are not known to get all attributes.
// This method is faster than GetAttributes(). For more information about
// iterators see user manual (Iterators)
// 

   return TFAttrIter(&fAttr);
}
//_____________________________________________________________________________
void TFHeader::PrintH(const Option_t* option) const
{
// prints all attributes with name, value, unit and comment of this header
// if "h" or "H" is part of option


// this function cannot be called Print() g++ versin 2.95.3 would have an
// internal compiler error

   if (strchr(option, 'h') == NULL &&
       strchr(option, 'H') == NULL    )
      // do nothing 
      return;

   TFAttrIter i_header = MakeAttrIterator();
   char hstr[500];
   while (i_header.Next())
      {
      printf("%-16s : %20s %-5s | %s\n",
               i_header->GetName(),
               i_header->GetStringValue(hstr),
               i_header->GetUnit(),
               i_header->GetComment() );
      }
}
//_____________________________________________________________________________
Bool_t TFAttrIter::Next()          
{
// Moves to the next attribute in the header. 
// Returns kFALSE if there is no further attribute, else kTRUE.

   if (i_attr == fAttr->end()) return kFALSE;
   fAttribute = *i_attr; 
   i_attr++; 
   return kTRUE;
}

