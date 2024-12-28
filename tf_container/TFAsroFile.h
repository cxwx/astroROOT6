//_____________________________________________________________________________
//
//  File:      TFAsroFile.h
//
//  Version:   1.0
//
//  Author:    Reiner Rohlfs (GADC)
//
//  History:   1.0   14.07.03  first released version
//
//_____________________________________________________________________________
#ifndef ROOT_TFAsroFile
#define ROOT_TFAsroFile

#ifndef ROOT_TObject
#include "TObject.h"
#endif

#ifndef ROOT_TString
#include "TString.h"
#endif

#include <map>
#include <vector>
 

//_____________________________________________________________________________
class TFAsroKey : public TObject
{
protected:
   UInt_t      fElName;       // name of the component
   TString     fSubName;      // column name
   Int_t       fCycle;        // cycle number in file
   
public:
   TFAsroKey()    {fCycle = 0; fElName = 0;}
   TFAsroKey(const TFAsroKey & key);
   TFAsroKey(UInt_t elName, const char * subName, Int_t cycle)
      : fSubName(subName) {fElName = elName; fCycle = cycle;}

   TFAsroKey & operator = (const TFAsroKey & key);

   bool     operator < (const TFAsroKey & key) const;

   void         IncreseCycle()     {fCycle++;}

   UInt_t       GetElName() const  {return fElName;}
   const char * GetSubName() const {return fSubName.Data();}
   Int_t        GetCycle() const   {return fCycle;}

   ClassDef(TFAsroKey, 1)     // internal class to store data in an ASRO file
};

//_____________________________________________________________________________

class TFAsroValue : public TObject
{
protected:
   UInt_t      fPos;          // position in asro - file
   UInt_t      fFileLength;   // length in bytes in file (compressed);
   UInt_t      fDataLength;   // length in bytes of data (uncompressed);
   UInt_t      fClassName;    // class name of this element

public:
   TFAsroValue();
   TFAsroValue(const TFAsroValue & value);

   TFAsroValue & operator = (const TFAsroValue & value);

   bool     operator < (const TFAsroValue & value) const
               {return fPos < value.fPos;}

   UInt_t      GetPos()        const {return fPos;}
   UInt_t      GetFileLength() const {return fFileLength;}
   UInt_t      GetDataLength() const {return fDataLength;}
   UInt_t      GetClassName()  const {return fClassName;}

   void        SetPos(UInt_t pos)             {fPos = pos;}
   void        SetFileLength(UInt_t length)   {fFileLength = length;}
   void        SetDataLength(UInt_t length)   {fDataLength = length;}
   void        SetClassName(UInt_t className) {fClassName = className;}


   ClassDef(TFAsroValue, 1)  // internal class to store data in an ASRO file

};

//_____________________________________________________________________________

class TFAsroColIter
{
public:
   TFAsroColIter(std::map<TFAsroKey, TFAsroValue>::iterator & start,
                 std::map<TFAsroKey, TFAsroValue>::iterator & end,
                 std::vector<TString> * classNames)
      {mi_entry = start; mi_end = end; m_classNames = classNames; m_colName = NULL;}

   Bool_t        Next();
   const char *  GetColName()    {return m_colName;}
   const char *  GetClassName()  {return (*m_classNames)[m_classNameIndex].Data();}

private:
   std::map<TFAsroKey, TFAsroValue>::iterator   mi_entry;
   std::map<TFAsroKey, TFAsroValue>::iterator   mi_end;
   const char *                                 m_colName;
   std::vector<TString>                         * m_classNames;
   UInt_t                                       m_classNameIndex;
};

//_____________________________________________________________________________

class TFAsroElementIter
{
public:
   TFAsroElementIter(std::map<TFAsroKey, TFAsroValue> * entries)
            { fEntries = entries;}

   Bool_t      Next();
   TFAsroKey & GetKey() {return fKey;}
   void        Reset()  {fKey = TFAsroKey();}

private:
   std::map<TFAsroKey, TFAsroValue>   * fEntries;

   TFAsroKey                     fKey;
};

//_____________________________________________________________________________

class TFAsroFile
{
protected:
   std::map<TFAsroKey, TFAsroValue>   fEntries;
   std::vector<TString>               fClassNames;
   std::vector<TString>               fNames;

   UInt_t      fDes[4];       //! position, length of fEntries,
                              //! length of fFree and not used mem
   UInt_t      fFreeReserve;  //! allocated memory for fFree;
   UInt_t      * fFree;       //! array of (pos, length) of free mem in file

   int         fFile;         //! file handler;
   TString     fFileName;     //! file name of this file
public:
   TFAsroFile();
   TFAsroFile(const char * fileName, Bool_t * readOnly);
   ~TFAsroFile();

   TObject *    Read(const TFAsroKey & key);
   TObject *    Read(const char * name, const char * subName, Int_t cycle);
   bool         Delete(const char * name, const char * subName, Int_t cycle);
   bool         InitWrite();
   bool         Write(TObject * obj, int compLevel, 
                      const char * name, const char * subName, Int_t cycle);
   bool         FinishWrite();
   void         Map();

   Bool_t       IsOpen()      {return fFile >= 0;}
   const char * GetFileName() {return fFileName.Data();}

   UInt_t       GetNumItems() {return fEntries.size();}
   UInt_t       GetFreeCycle(const char * name);
   UInt_t       GetNumSubs(const char * name, Int_t cycle);
   UInt_t       GetNextCycle(const char * name, Int_t cycle);

   TFAsroColIter *      MakeColIter(const char * name, Int_t cycle);
   TFAsroElementIter *  MakeElementIter() 
                             {return new TFAsroElementIter(&fEntries);}

protected:
   UInt_t       GetFree(UInt_t size);
   void         MakeFree(UInt_t pos, UInt_t size);

   ClassDef(TFAsroFile, 1)      // internal class to store data in an ASRO file

};

#endif


