// ////////////////////////////////////////////////////////////////////////////
//
//  File:      TFAsroFile.cpp
//
//  Version:   1.0
//
//  Author:    Reiner Rohlfs (GADC)
//
//  History:   1.0   14.07.03  first released version
//             1.2   31.01.08  change TBuffer toTBufferFile
//
// ////////////////////////////////////////////////////////////////////////////
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "TClass.h"

#if ROOT_VERSION_CODE >= ROOT_VERSION(5, 15, 0)
#include <TBufferFile.h>
#define MyBuffer TBufferFile
#else
#include <TBuffer.h>
#define MyBuffer TBuffer
#endif

#ifdef R__BYTESWAP
#if (defined(__linux) || defined(__APPLE__)) && defined(__i386__) && defined(__GNUC__)
#define USE_BSWAPCPY
#include <Bswapcpy.h>
#else
#include <Bytes.h>
#endif
#endif

#include "TFAsroFile.h"

#define MAX_UNIQUE_NAMES 0xffffffffu

extern "C" void R__zipLZMA(Int_t cxlevel, Int_t* nin, char* bufin, Int_t* lout, char* bufout, Int_t* nout);
extern "C" void R__unzipLZMA(Int_t* nin, UChar_t* bufin, Int_t* lout, char* bufout, Int_t* nout);
const Int_t MAX_CUT_LENGTH = 0xffffff;

static Bool_t Compress(int compLevel, char* in, int inLength, char* out, UInt_t* outLength);
static Bool_t Uncompress(UChar_t* in, char* out, UInt_t outLength);

//_____________________________________________________________________________
// TFAsroKey, TFAsroValue and TFAsroFile are internal classes.
// Theys should not be used directly by an applications or in an
// interactive session!

// There is one TFAsroFile object created per open ASRO file, even if more
// than one container of the file is opened.
// There is one map-pair <TFAsroKez, TFAsroValue> per element in the file.
// An element can be for example one image, one table (without columns) or
// one column.
// The component names and the class names are stored in the fNames and
// fClassNames, respectively. The index in these vectors are stored int
// TFAsroKey and TFAsroValue, respectively.
// Therefore it is important: Never delete a name in fClassNames and fNames!
// even if the component is deleted in the file.

TFAsroKey::TFAsroKey(const TFAsroKey& key) {
  fElName = key.fElName;
  fSubName = key.fSubName;
  fCycle = key.fCycle;
}
TFAsroKey& TFAsroKey::operator=(const TFAsroKey& key) {
  if (this != &key) {
    fElName = key.fElName;
    fSubName = key.fSubName;
    fCycle = key.fCycle;
  }
  return *this;
}
//_____________________________________________________________________________
bool TFAsroKey::operator<(const TFAsroKey& key) const {
  // This is used to sort the TFAsroKeys in the fEntries - map.
  // The first key is the element name, than the cycle number,
  // and than the subName (== column name). This priority of keys
  // must not be changed!

  if (fElName != key.fElName)
    return fElName < key.fElName;

  if (fCycle != key.fCycle)
    return fCycle < key.fCycle;

  return fSubName < key.fSubName;
}
TFAsroValue::TFAsroValue() {
  fPos = 0;
  fFileLength = 0;
  fDataLength = 0;
  fClassName = 0;
}
//_____________________________________________________________________________
TFAsroValue::TFAsroValue(const TFAsroValue& value) {
  fPos = value.fPos;
  fFileLength = value.fFileLength;
  fDataLength = value.fDataLength;
  fClassName = value.fClassName;
}
//_____________________________________________________________________________
TFAsroValue& TFAsroValue::operator=(const TFAsroValue& value) {
  if (this != &value) {
    fPos = value.fPos;
    fFileLength = value.fFileLength;
    fDataLength = value.fDataLength;
    fClassName = value.fClassName;
  }
  return *this;
}
//_____________________________________________________________________________
//_____________________________________________________________________________
Bool_t TFAsroColIter::Next() {
  if (mi_entry == mi_end)
    return kFALSE;

  m_colName = mi_entry->first.GetSubName();
  m_classNameIndex = mi_entry->second.GetClassName();
  mi_entry++;
  return kTRUE;
}
//_____________________________________________________________________________
Bool_t TFAsroElementIter::Next() {
  fKey.IncreseCycle();

  std::map<TFAsroKey, TFAsroValue>::iterator i_entry;
  i_entry = fEntries->lower_bound(fKey);
  if (i_entry == fEntries->end())
    return kFALSE;

  fKey = i_entry->first;
  return kTRUE;
}
//_____________________________________________________________________________
//_____________________________________________________________________________
TFAsroFile::TFAsroFile() {
  fDes[0] = fDes[1] = fDes[2] = 0;
  fFreeReserve = 0;
  fFree = NULL;
  fFile = -1;
}
//_____________________________________________________________________________
TFAsroFile::TFAsroFile(const char* fileName, Bool_t* readOnly) {
  //  Opens or creates an ASRO file.
  //  First it tries to open read and write. If this is not possible
  //  and readOnly is set to kTRUE it tries to open also as readOnly.
  //
  //  In any case readOnly is set to kFALSE if it was possible to
  //  open as readOnly, independent of the input value of readOnly.
  //  readOnly is unchanged (stay as kTRUE) if the file is opened as
  //  read only. If the file could not opened at all (either system
  //  error or it is read only, but it should be opened as read write)
  //  readOnly is not modified.
  //
  //  If anything went wrong the file descriptor fFile is set to a value
  //  less than 0. If fFile >= 0 the calling function can assume that
  //  the file is successfully open and can be used.

  bool ok = true;  // will be set to false if anything goes wrong

  fFree = NULL;

  fFile = open(fileName, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

  if (fFile < 0) {
    if (*readOnly) {
      // try to open as read only
      fFile = open(fileName, O_RDONLY);
      if (fFile < 0)
        return;
    } else
      return;
  } else
    *readOnly = kFALSE;

  struct stat buf;
  fstat(fFile, &buf);
  if (buf.st_size > 0) {
    // the file exist already
    char id[8] = "";
    ok &= read(fFile, id, 8) == 8;
    if (!ok || strncmp(id, "ASRO0001", 8) != 0) {
      // it is not an ASRO - file
      close(fFile);
      fFile = -2;
      return;
    }

#ifdef R__BYTESWAP
    UInt_t des[4];
    ok &= read(fFile, des, 4 * sizeof(UInt_t)) == 4 * sizeof(UInt_t);

#ifdef USE_BSWAPCPY
    bswapcpy32(fDes, des, 4);
#else
    for (int i = 0; i < 4; i++)
      frombuf(reinterpret_cast<char*&>(des), &fDes[i]);
#endif
#else
    ok &= read(fFile, fDes, 4 * sizeof(UInt_t)) == 4 * sizeof(UInt_t);
#endif

    // read and create the descriptor
    lseek(fFile, fDes[0], SEEK_SET);
    if (fDes[1] > 0) {
      MyBuffer buffer(TBuffer::kRead, fDes[1]);
      ok &= read(fFile, buffer.Buffer(), fDes[1]) == fDes[1];
      Streamer(buffer);
    }

    // read the free mem info
    fFreeReserve = fDes[2];
    fFree = new UInt_t[fFreeReserve / sizeof(UInt_t)];
#ifdef R__BYTESWAP
    UInt_t* freeSwap = new UInt_t[fFreeReserve / sizeof(UInt_t)];
    ok &= read(fFile, freeSwap, fDes[2]) == fDes[2];
#ifdef USE_BSWAPCPY
    bswapcpy32(fFree, freeSwap, fFreeReserve / sizeof(UInt_t));
#else
    for (int i = 0; i < fFreeReserve / sizeof(UInt_t); i++)
      frombuf(reinterpret_cast<char*&>(freeSwap), &fFree[i]);
#endif
    delete[] freeSwap;
#else
    ok &= read(fFile, fFree, fDes[2]) == fDes[2];
#endif
  } else {
    // we create a new ASRO - file
    ok &= write(fFile, "ASRO0001", 8) == 8;

    fDes[0] = 8 + 4 * sizeof(UInt_t);
    fDes[1] = 0;
    fDes[2] = 2 * sizeof(UInt_t);
    fDes[3] = 0;

    fFreeReserve = fDes[2];
    fFree = new UInt_t[fFreeReserve / sizeof(UInt_t)];
    fFree[0] = 8 + 6 * sizeof(UInt_t);
    fFree[1] = 0xFFFFFFFFU - (8 + 6 * sizeof(UInt_t));

#ifdef R__BYTESWAP
    UInt_t desSwap[4];
    UInt_t freeSwap[2];
#ifdef USE_BSWAPCPY
    bswapcpy32(desSwap, fDes, 4);
    bswapcpy32(freeSwap, fFree, 2);
#else
    for (int i = 0; i < 4; i++)
      frombuf(reinterpret_cast<char*&>(fDes), &desSwap[i]);
    for (int i = 0; i < 2; i++)
      frombuf(reinterpret_cast<char*&>(fFree), &freeSwap[i]);
#endif
    ok &= write(fFile, desSwap, 4 * sizeof(UInt_t)) == 4 * sizeof(UInt_t);
    ok &= write(fFile, freeSwap, fDes[2]) == fDes[2];
#else
    ok &= write(fFile, fDes, 4 * sizeof(UInt_t)) == 4 * sizeof(UInt_t);
    ok &= write(fFile, fFree, fDes[2]) == fDes[2];
#endif
  }

  if (ok)
    fFileName = fileName;
  else {
    close(fFile);
    fFile = -3;
  }
}
//_____________________________________________________________________________
TFAsroFile::~TFAsroFile() {
  if (fFile >= 0)
    close(fFile);

  delete[] fFree;
}
//_____________________________________________________________________________
TObject* TFAsroFile::Read(const char* name, const char* subName, Int_t cycle) {
  // Returns the requested object, read from the file.
  // If the retunr value is not NULL the calling function can assume that
  // everything is OK.

  if (fFile < 0)
    return NULL;

  // find the nameIndex in names
  UInt_t nameIndex;
  int numNames = fNames.size();
  if (numNames == 0)
    return NULL;

  for (nameIndex = 0; nameIndex < numNames; nameIndex++)
    if (fNames[nameIndex] == name)
      break;
  if (nameIndex == numNames)
    return NULL;

  return Read(TFAsroKey(nameIndex, subName, cycle));
}
//_____________________________________________________________________________
TObject* TFAsroFile::Read(const TFAsroKey& key) {
  // Returns the requested object, read from the file.
  // If the retunr value is not NULL the calling function can assume that
  // everything is OK.

  if (fFile < 0)
    return NULL;

  // look for the key in this file
  std::map<TFAsroKey, TFAsroValue>::iterator i_entry;
  i_entry = fEntries.find(key);
  if (i_entry == fEntries.end())
    // this key does not exist in the file
    return NULL;

  // read the buffer
  bool ok;
  lseek(fFile, i_entry->second.GetPos(), SEEK_SET);
  MyBuffer buffer(TBuffer::kRead, i_entry->second.GetDataLength());
  if (i_entry->second.GetDataLength() == i_entry->second.GetFileLength())
    ok = read(fFile, buffer.Buffer(), i_entry->second.GetDataLength()) == i_entry->second.GetDataLength();
  else {
    UChar_t* fileBuffer = new UChar_t[i_entry->second.GetFileLength()];
    ok = read(fFile, fileBuffer, i_entry->second.GetFileLength()) == i_entry->second.GetFileLength();
    Uncompress(fileBuffer, buffer.Buffer(), i_entry->second.GetDataLength());
    delete[] fileBuffer;
  }

  if (!ok)
    return NULL;

  // create a new object and stream it
  TClass cl(fClassNames[i_entry->second.GetClassName()].Data());
  TObject* obj = (TObject*)cl.New();

  if (obj)
    obj->Streamer(buffer);

  return obj;
}
//_____________________________________________________________________________
bool TFAsroFile::InitWrite() {
  if (fFile < 0)
    return false;

  // free space for old description
  MakeFree(fDes[0], fDes[1] + fDes[2] + fDes[3]);

  // write 0 to first byte
  lseek(fFile, 8, SEEK_SET);
  UInt_t zero = 0;
  return write(fFile, &zero, sizeof(UInt_t)) == sizeof(UInt_t);
}
//_____________________________________________________________________________
bool TFAsroFile::Write(TObject* obj, int compLevel, const char* name, const char* subName, Int_t cycle) {
  if (fFile < 0)
    return false;

  // find the nameIndex in names or add it to names
  UInt_t nameIndex;
  int numNames = fNames.size();
  for (nameIndex = 0; nameIndex < numNames; nameIndex++)
    if (fNames[nameIndex] == name)
      break;
  if (nameIndex == numNames)
    fNames.push_back(TString(name));

  TFAsroKey key(nameIndex, subName, cycle);

  // find obj in descriptor
  TFAsroValue& asroValue = fEntries[key];

  // free space for object to be deleted
  if (asroValue.GetPos() > 0)
    MakeFree(asroValue.GetPos(), asroValue.GetFileLength());

  // write obj into a buffer
  MyBuffer buffer(TBuffer::kWrite);
  obj->Streamer(buffer);
  asroValue.SetDataLength(buffer.Length());
  char* dataBuffer;
  if (compLevel <= 0 || buffer.Length() < 256) {
    dataBuffer = buffer.Buffer();
    asroValue.SetFileLength(buffer.Length());
  } else {
    UInt_t fileLength;
    dataBuffer = new char[buffer.Length()];
    if (Compress(compLevel, buffer.Buffer(), buffer.Length(), dataBuffer, &fileLength))
      asroValue.SetFileLength(fileLength);
    else
    // there is a compression error: do not compress
    {
      delete[] dataBuffer;
      dataBuffer = buffer.Buffer();
      asroValue.SetFileLength(buffer.Length());
    }
  }

  fDes[3] = 2 * sizeof(UInt_t);

  // find the classNameIndex in ClassNames or add it to names
  UInt_t classNameIndex;
  int numClassNames = fClassNames.size();
  const char* className = obj->IsA()->GetName();

  for (classNameIndex = 0; classNameIndex < numClassNames; classNameIndex++)
    if (fClassNames[classNameIndex] == className)
      break;
  if (classNameIndex == numClassNames)
    fClassNames.push_back(TString(className));

  // find position in file for obj and store the className
  asroValue.SetPos(GetFree(asroValue.GetFileLength()));
  asroValue.SetClassName(classNameIndex);

  // save new obj to file
  lseek(fFile, asroValue.GetPos(), SEEK_SET);
  bool ok = write(fFile, dataBuffer, asroValue.GetFileLength()) == asroValue.GetFileLength();
  if (dataBuffer != buffer.Buffer())
    delete[] dataBuffer;

  return ok;
}
//_____________________________________________________________________________
bool TFAsroFile::FinishWrite() {
  if (fFile < 0)
    return false;

  bool ok = true;

  // create and fill buffer for the descriptor
  MyBuffer desBuffer(TBuffer::kWrite);
  Streamer(desBuffer);
  fDes[1] = desBuffer.Length();

  fDes[3] = 2 * sizeof(UInt_t);
  // get new position for descriptor
  fDes[0] = GetFree(fDes[1] + fDes[2] + fDes[3]);

  // save descriptor to file
  lseek(fFile, fDes[0], SEEK_SET);
  ok &= write(fFile, desBuffer.Buffer(), fDes[1]) == fDes[1];

#ifdef R__BYTESWAP
  // save free space to file
  UInt_t* freeSwap = new UInt_t[fDes[2] / sizeof(UInt_t)];
#ifdef USE_BSWAPCPY
  bswapcpy32(freeSwap, fFree, fDes[2] / sizeof(UInt_t));
#else
  for (int i = 0; i < fDes[2] / sizeof(UInt_t); i++)
    frombuf(reinterpret_cast<char*&>(fFree), &freeSwap[i]);
#endif
  ok &= write(fFile, freeSwap, fDes[2]) == fDes[2];
  delete[] freeSwap;

  // save first bytes to file
  UInt_t desSwap[4];
#ifdef USE_BSWAPCPY
  bswapcpy32(desSwap, fDes, 4);
#else
  for (int i = 0; i < 4; i++)
    frombuf(reinterpret_cast<char*&>(fDes), &desSwap[i]);
#endif
  lseek(fFile, 8, SEEK_SET);
  ok &= write(fFile, desSwap, 4 * sizeof(UInt_t)) == 4 * sizeof(UInt_t);
#else
  // save free space to file
  ok &= write(fFile, fFree, fDes[2]) == fDes[2];

  // save first bytes to file
  lseek(fFile, 8, SEEK_SET);
  ok &= write(fFile, fDes, 4 * sizeof(UInt_t)) == 4 * sizeof(UInt_t);
#endif

  return ok;
}
//_____________________________________________________________________________
bool TFAsroFile::Delete(const char* name, const char* subName, Int_t cycle) {
  if (fFile < 0)
    return false;

  // find the nameIndex in names
  UInt_t nameIndex;
  int numNames = fNames.size();
  if (numNames == 0)
    return false;

  for (nameIndex = 0; nameIndex < numNames; nameIndex++)
    if (fNames[nameIndex] == name)
      break;
  if (nameIndex == numNames)
    return false;

  TFAsroKey key(nameIndex, subName, cycle);

  // find entry in descriptor
  std::map<TFAsroKey, TFAsroValue>::iterator i_entry;
  i_entry = fEntries.find(key);
  if (i_entry == fEntries.end())
    return false;

  // free space for old description
  MakeFree(fDes[0], fDes[1] + fDes[2] + fDes[3]);

  // free space for object to be deleted
  MakeFree(i_entry->second.GetPos(), i_entry->second.GetFileLength());

  // delete entry in descriptor
  fEntries.erase(i_entry);

  if (subName[0] == 0) {
    // delete also all columns of this table
    std::map<TFAsroKey, TFAsroValue>::iterator i_begin = fEntries.upper_bound(TFAsroKey(nameIndex, "", cycle));
    std::map<TFAsroKey, TFAsroValue>::iterator i_end = fEntries.lower_bound(TFAsroKey(nameIndex, "", cycle + 1));
    std::map<TFAsroKey, TFAsroValue>::iterator i_col = i_begin;

    // free space for all columns
    while (i_col != i_end) {
      MakeFree(i_col->second.GetPos(), i_col->second.GetFileLength());
      i_col++;
    }

    // delete the columns in the descriptor
    fEntries.erase(i_begin, i_end);
  }

  bool ok = true;

  // create and fill buffer
  MyBuffer buffer(TBuffer::kWrite);
  Streamer(buffer);
  fDes[1] = buffer.Length();

  // get new position for descriptor
  fDes[3] = 2 * 2 * sizeof(UInt_t);
  fDes[0] = GetFree(fDes[1] + fDes[2] + fDes[3]);

  // save descriptor to file
  lseek(fFile, fDes[0], SEEK_SET);
  ok &= write(fFile, buffer.Buffer(), fDes[1]) == fDes[1];

#ifdef R__BYTESWAP
  // save free space to file
  UInt_t* freeSwap = new UInt_t[fDes[2] / sizeof(UInt_t)];
#ifdef USE_BSWAPCPY
  bswapcpy32(freeSwap, fFree, fDes[2] / sizeof(UInt_t));
#else
  for (int i = 0; i < fDes[2] / sizeof(UInt_t); i++)
    frombuf(reinterpret_cast<char*&>(fFree), &freeSwap[i]);
#endif
  ok &= write(fFile, freeSwap, fDes[2]) == fDes[2];
  delete[] freeSwap;

  // save first bytes to file
  UInt_t desSwap[4];
#ifdef USE_BSWAPCPY
  bswapcpy32(desSwap, fDes, 4);
#else
  for (int i = 0; i < 4; i++)
    frombuf(reinterpret_cast<char*&>(fDes), &desSwap[i]);
#endif
  lseek(fFile, 8, SEEK_SET);
  ok &= write(fFile, desSwap, 4 * sizeof(UInt_t)) == 4 * sizeof(UInt_t);
#else
  // save free space to file
  ok &= write(fFile, fFree, fDes[2]) == fDes[2];

  // save first bytes to file
  lseek(fFile, 8, SEEK_SET);
  ok &= write(fFile, fDes, 4 * sizeof(UInt_t)) == 4 * sizeof(UInt_t);
#endif

  return ok;
}
//_____________________________________________________________________________
UInt_t TFAsroFile::GetFree(UInt_t size) {
  UInt_t bestFit = 0;
  UInt_t bestSize = 0xFFFFFFFFU;

  // look for the smallest hole, but at least the size of "size"
  for (UInt_t index = 0; index < fDes[2] / (2 * sizeof(UInt_t)); index++)
    if (size <= fFree[index * 2 + 1] && bestSize > fFree[index * 2 + 1]) {
      bestFit = index;
      bestSize = fFree[index * 2 + 1];
    }

  UInt_t pos = fFree[bestFit * 2];
  if (size == fFree[bestFit * 2 + 1]) {
    // the new fits perfect in this free space
    // remove the hole
    memmove(fFree + 2 * bestFit, fFree + 2 * (bestFit + 1), fDes[2] - (bestFit + 1) * 2 * sizeof(UInt_t));
    fDes[2] -= 2 * sizeof(UInt_t);
    fDes[3] += 2 * sizeof(UInt_t);
  } else {
    // resize the hole
    fFree[bestFit * 2] += size;      // shift the start pos
    fFree[bestFit * 2 + 1] -= size;  // decrease the hole
  }
  return pos;
}
//_____________________________________________________________________________
void TFAsroFile::MakeFree(UInt_t pos, UInt_t size) {
  // find first free behind pos
  UInt_t index = 0;

  while (index < fDes[2] / (2 * sizeof(UInt_t)) && pos > fFree[index * 2])
    index++;

  if (index == 0) {
    // new free space before the first
    if (pos + size == fFree[0]) {
      // we can increase the first free space
      fFree[0] = pos;
      fFree[1] += size;
    } else {
      // insert a new free item at index 0
      if (fDes[2] == fFreeReserve) {
        // increase allocated memory for 100 entries
        fFreeReserve += 100 * sizeof(UInt_t);
        UInt_t* tmp = new UInt_t[fFreeReserve / sizeof(UInt_t)];
        memcpy(tmp + 2, fFree, fDes[2]);

        delete[] fFree;
        fFree = tmp;
      } else {
        // make space for the new entry
        memmove(fFree + 2, fFree, fDes[2]);
      }
      fDes[2] += 2 * sizeof(UInt_t);
      fDes[3] -= 2 * sizeof(UInt_t);
      fFree[0] = pos;
      fFree[1] = size;
    }
    return;
  }

  bool before = fFree[(index - 1) * 2] + fFree[(index - 1) * 2 + 1] == pos;
  bool after = pos + size == fFree[index * 2];

  if (before && !after)
    // there is a free space just before but allocated space after
    // increase the space just before
    fFree[(index - 1) * 2 + 1] += size;

  else if (!before && after)
  // there is a free space just behind but allocated space before
  // increase the space and reset the pos just behind
  {
    fFree[index * 2] -= size;
    fFree[index * 2 + 1] += size;
  }

  else if (before && after)
  // there is a free space just before and just behind
  // increase the space of the one before and remove the one behind
  {
    fFree[(index - 1) * 2 + 1] += size + fFree[index * 2 + 1];
    memmove(fFree + 2 * index, fFree + 2 * (index + 1), fDes[2] - (index + 1) * 2 * sizeof(UInt_t));
    fDes[2] -= 2 * sizeof(UInt_t);
    fDes[3] += 2 * sizeof(UInt_t);
  }

  else if (!before && !after)
  // there is no free space just before an no just behind
  // create a new free space
  {
    if (fDes[2] == fFreeReserve) {
      // increase allocated memory for 100 entries
      fFreeReserve += 100 * sizeof(UInt_t);
      UInt_t* tmp = new UInt_t[fFreeReserve / sizeof(UInt_t)];
      memcpy(tmp, fFree, index * 2 * sizeof(UInt_t));
      memcpy(tmp + (index + 1) * 2, fFree + index * 2, fDes[2] - index * 2 * sizeof(UInt_t));

      delete[] fFree;
      fFree = tmp;
    } else {
      // make space for the new entry
      memmove(fFree + 2 * (index + 1), fFree + 2 * index, fDes[2] - index * 2 * sizeof(UInt_t));
    }
    fDes[2] += 2 * sizeof(UInt_t);
    fDes[3] -= 2 * sizeof(UInt_t);
    fFree[index * 2] = pos;
    fFree[index * 2 + 1] = size;
  }
}
//_____________________________________________________________________________
UInt_t TFAsroFile::GetFreeCycle(const char* name) {
  // find the nameIndex in names or add it to names
  UInt_t nameIndex;
  int numNames = fNames.size();
  for (nameIndex = 0; nameIndex < numNames; nameIndex++)
    if (fNames[nameIndex] == name)
      break;
  if (nameIndex == numNames)
    fNames.push_back(TString(name));

  std::map<TFAsroKey, TFAsroValue>::iterator i_entry;
  i_entry = fEntries.upper_bound(TFAsroKey(nameIndex, "", 0));

  UInt_t prevCycle = 0;
  while (i_entry != fEntries.end() && nameIndex == i_entry->first.GetElName()) {
    if (i_entry->first.GetCycle() - prevCycle > 1)
      return prevCycle + 1;
    prevCycle = i_entry->first.GetCycle();
    i_entry++;
  }

  return prevCycle < MAX_UNIQUE_NAMES ? prevCycle + 1 : 0;
}
//_____________________________________________________________________________
UInt_t TFAsroFile::GetNumSubs(const char* name, Int_t cycle) {
  // find the nameIndex in names
  UInt_t nameIndex;
  int numNames = fNames.size();
  if (numNames == 0)
    return 0;

  for (nameIndex = 0; nameIndex < numNames; nameIndex++)
    if (fNames[nameIndex] == name)
      break;
  if (nameIndex == numNames)
    return 0;

  std::map<TFAsroKey, TFAsroValue>::iterator i_entry;
  i_entry = fEntries.upper_bound(TFAsroKey(nameIndex, "", cycle));

  UInt_t numSub = 0;
  while (i_entry != fEntries.end() && i_entry->first.GetCycle() == cycle && nameIndex == i_entry->first.GetElName()) {
    numSub++;
    i_entry++;
  }

  return numSub;
}
//_____________________________________________________________________________
UInt_t TFAsroFile::GetNextCycle(const char* name, Int_t cycle) {
  // find the nameIndex in names
  UInt_t nameIndex;
  int numNames = fNames.size();
  if (numNames == 0)
    return 1;

  for (nameIndex = 0; nameIndex < numNames; nameIndex++)
    if (fNames[nameIndex] == name)
      break;
  if (nameIndex == numNames)
    return 1;

  std::map<TFAsroKey, TFAsroValue>::iterator i_entry;
  i_entry = fEntries.lower_bound(TFAsroKey(nameIndex, "", cycle + 1));
  if (i_entry == fEntries.end() || nameIndex != i_entry->first.GetElName())
    return 0;

  return i_entry->first.GetCycle();
}
//_____________________________________________________________________________
TFAsroColIter* TFAsroFile::MakeColIter(const char* name, Int_t cycle) {
  // find the nameIndex in names
  UInt_t nameIndex;
  int numNames = fNames.size();
  for (nameIndex = 0; nameIndex < numNames; nameIndex++)
    if (fNames[nameIndex] == name)
      break;

  std::map<TFAsroKey, TFAsroValue>::iterator i_entry, i_end;
  i_entry = fEntries.upper_bound(TFAsroKey(nameIndex, "", cycle));
  i_end = fEntries.lower_bound(TFAsroKey(nameIndex, "", cycle + 1));
  return new TFAsroColIter(i_entry, i_end, &fClassNames);
}
//_____________________________________________________________________________
static Bool_t Compress(int compLevel, char* in, int inLength, char* out, UInt_t* outLength) {
  *outLength = 0;
  while (inLength > 0) {
    int length;
    if (inLength <= MAX_CUT_LENGTH)
      length = inLength;
    else if (inLength < MAX_CUT_LENGTH * 2)
      length = inLength / 2;
    else
      length = MAX_CUT_LENGTH;
    inLength -= length;

    int nout;
    R__zipLZMA(compLevel, &length, in, &length, out, &nout);
    if (nout == 0)
      return kFALSE;
    in += length;
    out += nout;
    *outLength += nout;
  }
  return kTRUE;
}
//_____________________________________________________________________________
static Bool_t Uncompress(UChar_t* in, char* out, UInt_t outLength) {
  Int_t readTotal = 0;
  while (1) {
    Int_t nin = 9 + ((Int_t)in[3] | ((Int_t)in[4] << 8) | ((Int_t)in[5] << 16));
    Int_t nout = (Int_t)in[6] | ((Int_t)in[7] << 8) | ((Int_t)in[8] << 16);
    Int_t read;
    R__unzipLZMA(&nin, in, &nout, out, &read);
    if (read == 0)
      return kFALSE;
    readTotal += read;
    if (readTotal >= outLength)
      return kTRUE;
    in += nin;
    out += read;
  }
}
