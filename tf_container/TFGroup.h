//_____________________________________________________________________________
//
//  File:      TFGroup.h
//
//  Version:   1.0
//
//  Author:    Reiner Rohlfs (GADC)
//
//  History:   1.0   04.08.03  first released version
//
//_____________________________________________________________________________
#ifndef ROOT_TFGroup
#define ROOT_TFGroup

#ifndef ROOT_TFError
#include "TFError.h"
#endif

#ifndef ROOT_TFTable
#include "TFTable.h"
#endif

#ifndef ROOT_TFColumn
#include "TFColumn.h"
#endif



#include <set>

class TFGroupIter;
class TFElementPtr;

enum TFDataType {
   kUndefType = 0,
   kBaseType  = 1,
   kImageType = 2,
   kTableType = 3,
   kGroupType = 4
};
   

//_____________________________________________________________________________

class TFSelector : public TObject
{
public:
   virtual Bool_t Select(TFElementPtr & item) const = 0;

   ClassDef(TFSelector, 0)  // Abstract base class for selectors to be used in TFGroup
};

//_____________________________________________________________________________

class TFToken
{
   char *   fBuffer;    // name of this token
   char **  fToken;     // array of token in fBuffer
   UInt_t   fNumToken;  // number of token 
   Bool_t   fLead;      // kTRUE if first character of fBuffer is a *
   Bool_t   fTrail;     // kTRUE if last character of fBuffer is a *

public:
   TFToken();
   ~TFToken();

   void     SetName(const char * name);
   Bool_t   Select(const char * testName);

   ClassDef(TFToken, 0)  // A helper class for TFNameSelector
};

//_____________________________________________________________________________

class TFNameSelector : public TFSelector
{
   TFToken  * fNames;   // one token class for each name
   UInt_t   fNumNames;  // number of names in this selector

public:
   TFNameSelector(const char * name);
   TFNameSelector(const char ** names, UInt_t numNames);
   ~TFNameSelector();

   virtual Bool_t Select(TFElementPtr & item) const;

   ClassDef(TFNameSelector, 0)  // Selects TFIOElemnts in a group depending on their name
};

//_____________________________________________________________________________

class TFFilePath : public TString
{
public:
   TFFilePath()   {}
   TFFilePath(const char * filePath) : TString(filePath) {}
   TFFilePath(TFFilePath & filePath) : TString(filePath) {}

   TFFilePath &   operator=(const char * fileName)
                     { TString::operator=(fileName);  return *this;}

   void     MakeAbsolutePath(const char * isRelativeTo);
   void     MakeRelativePath(const char * relativeTo);
   Bool_t   IsRelativePath() const;

   ClassDef(TFFilePath,1)  // Absolute or relative file path
};

//_____________________________________________________________________________

class TFElementPtr : public TObject
{
protected:
   TFFilePath  fFileName;     // the filename of element
   TString     fElementName;  // the element name 
   Int_t       fCycle;        // the cycle of this element in the file
                              // if < 0 : extversion in FITS file
   TFDataType  fType;         // Type of data

public:
   TFElementPtr() {fCycle = 0; fType = kUndefType;}
   TFElementPtr(const char * filePath, const char * name, 
                UInt_t cycle, TFDataType dataType) : fFileName(filePath), fElementName(name)
                   {fCycle = cycle; fType = dataType;}
   TFElementPtr(const TFIOElement * element);

   TFElementPtr(const TFElementPtr & groupItem);
      
   TFElementPtr & operator=(const TFElementPtr & elementPtr);
   Bool_t         operator==(const TFElementPtr & elementPtr) const;
   Bool_t         operator< (const TFElementPtr & elementPtr) const;

   Bool_t         IsGroup() const        {return fType == kGroupType;}
   TFDataType     GetDataType() const    {return fType;}
   const char *   GetFileName() const    {return fFileName.Data();}
   const char *   GetElementName() const {return fElementName.Data();}
   Int_t          GetCycle() const       {return fCycle;}

   void           MakeAbsolutePath(const char * IsRelativeTo)
                                    {fFileName.MakeAbsolutePath(IsRelativeTo);}
   void           MakeRelativePath(const char * relativeTo)
                                    {fFileName.MakeRelativePath(relativeTo);}
   Bool_t         IsRelativePath()  {return fFileName.IsRelativePath();}

   ClassDef(TFElementPtr,1) // A pointer of a TFGroup to other elements
};
//_________________________________________________________________________
class TFElementIdPtr : public TFElementPtr
{
protected:
   Long_t fileId;

public:
   TFElementIdPtr(const TFElementPtr & elementPtr, const char * isRelativeTo);

   Bool_t  operator< (const TFElementIdPtr & elementPtr) const;
};
//_________________________________________________________________________
class ElementPtrFormat
{
public:
    static char * Format(char * str, Int_t width, const char * format, const TFElementPtr & value);
    static void SetString(const char * str, TFElementPtr & value ) {}
    static const char * GetTypeName() {return "TFElementPtr";}
    static const char * GetBranchType() {return "";}
    static double       ToDouble(const TFElementPtr & value)
                      {TFError::SetError("ElementPtrFormat::ToDouble", 
                                         "Cannot convert a TFGroup pointer into a double"); return 0;}
    static void         SetDouble(Double_t dbl, TFElementPtr & value) {}
};

typedef    TFColumn<TFElementPtr, ElementPtrFormat>  TFGroupCol;
extern const char GROUP_COL_NAME[];


//_____________________________________________________________________________

class TFGroup : public TFTable
{

public:
   TFGroup() {}

   TFGroup(TFGroup & group)     : TFTable(group) {}
   TFGroup(const char * name, UInt_t numRows = 0)   : TFTable(name, numRows)  {}
   TFGroup(const char * name  , const char * fileName) : TFTable(name, fileName)  
         {if (fio) fio->CreateElement();}

   virtual ~TFGroup() {};

   virtual  TFGroup & operator = (TFGroup & group)  {TFTable::operator =(group); return *this;}

   virtual  UInt_t Attach(TFIOElement * element, Bool_t relativePath = kTRUE);
   virtual  void   Detach(TFIOElement * element);

            TFGroupIter MakeGroupIterator();

   ClassDef(TFGroup,1)  // A group with pointers to other TFIOElements
};

//_____________________________________________________________________________

class TFGroupIter
{
   TFGroup                  * fGroup;   // the group of this iterator
   TFGroupCol               & fCol;     // the group column of its group
   TFRowIter                fRowIter;   // the row iterator 
   Bool_t                   fReadOnly;  // open children as read only if kTRUE
   TFGroupIter              * fList;    // next chain in list;
   TFIOElement              * fLast;    // last element returned from Next()
   std::set<TFElementIdPtr> * fDone;    // list of already passed elements (TFElementPtr)
   Bool_t                   fStart;     // kTRUE if this iterator is the first in the list of itertors
   TList                    * fSelect;  // list of Selectors
   

   TFGroupIter(TFGroup * group);
   TFGroupIter(TFGroup * group, std::set<TFElementIdPtr> * done, 
               TList * select, Bool_t readOnly);
   TFGroupIter &  operator = (const TFGroupIter & i_group) {return *this;}

public:
   TFGroupIter(const TFGroupIter & groupIter);
   ~TFGroupIter();

   void           SetSelector(TFSelector * select);
   void           ClearSelectors();
   Bool_t         Filter(const char * filter)    {return fRowIter.Filter(filter);}
   void           Sort(const char * colName)     {fRowIter.Sort(colName);}
   void           ClearFilterSort()              {fRowIter.ClearFilterSort();}
   Bool_t         Next();
   TFIOElement &  operator * ()                  {return fList ? fList->operator*() : *fLast;}
   TFIOElement *  operator -> ()                 {return fList ? fList->operator->() : fLast;}
   void           Reset();
   void           SetReadOnly(Bool_t readOnly = kTRUE);
   Bool_t         IsReadOnly()         {return fReadOnly;}

friend  TFGroupIter   TFGroup::MakeGroupIterator();


   ClassDef(TFGroupIter,0)   // An iterator to get access to all children of a TFGroup
};


extern TFGroup *  TFReadGroup(const char * fileName, const char * name,  
                              UInt_t cycle = 0, FMode mode = kFRead);

#endif
