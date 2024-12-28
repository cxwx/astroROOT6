// ///////////////////////////////////////////////////////////////////
//
//  File:      TFVirtualIO.cxx
//
//  Version:   1.0
//
//  Author:    Reiner Rohlfs (GADC)
//
//  History:   1.0   15.07.03  first released version
//
// ///////////////////////////////////////////////////////////////////
#include "TFVirtualIO.h"
#include "TFIOElement.h"

ClassImp(TFVirtualIO)
ClassImp(TFVirtualFileIter)


TFVirtualFileIter::~TFVirtualFileIter()
{
   delete fElement;
}

