// ////////////////////////////////////////////////////////////////////////////
//
//  File:      TFColWrapper.h
//
//  Version:   1.0
//
//  Author:    Reiner Rohlfs (GADC)
//
//  History:   1.0   18.07.03  first released version
//
// ////////////////////////////////////////////////////////////////////////////
#ifndef ROOT_TFColWrapper
#define ROOT_TFColWrapper

#include "string.h"
#include <set>


class TFBaseCol;

class TFColWrapper
{
  TNamed & fCol;

public:
  TFColWrapper(TNamed & col) : fCol(col) { }
  TFBaseCol & GetCol() const {return (TFBaseCol&)fCol;}

  bool operator < (const TFColWrapper & colWr) const
    {return strcmp(fCol.GetName(), colWr.fCol.GetName()) < 0;} 
};

//_____________________________________________________________________________

typedef std::set<TFColWrapper>           ColList;
typedef std::set<TFColWrapper>::iterator I_ColList;


#endif

