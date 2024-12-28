// ////////////////////////////////////////////////////////////////////////////
//
//  File:      TFAsroIO.cxx
//
//  Version:   1.0
//
//  Author:    Reiner Rohlfs (GADC)
//
//  History:   1.0   16.07.03  first released version
//
// ////////////////////////////////////////////////////////////////////////////
#include "TFAsroFile.h"

//_____________________________________________________________________________
void MemTest(UInt_t prev, UInt_t pos)
{
   if (prev < pos)
      printf(" ===== lost memory from %u to %u  =====\n", 
             prev, pos - 1);

   if (prev > pos)
      printf(" +++++ memory used twice: from %u to %u +++++\n",
              pos, prev -1);
}
//_____________________________________________________________________________
void TFAsroFile::Map()
{
// prints beside some general information one line per object in the file
   if (fFile < 0)
      {
      printf("\n    file is not open\n\n");
      return;
      }

   std::map<TFAsroValue,TFAsroKey>  posEntry;

   std::map<TFAsroKey, TFAsroValue>::iterator i_entry;
   for (i_entry = fEntries.begin(); i_entry != fEntries.end(); i_entry++)
      posEntry[i_entry->second] = i_entry->first;

   // entry for descriptor
   TFAsroValue  desVal;
   if (fDes[1] > 0)
      {
      desVal.SetPos(fDes[0]);
      desVal.SetDataLength(fDes[1]);
      desVal.SetFileLength(fDes[1]);
      desVal.SetClassName(0xffffffff);
      posEntry[desVal] = TFAsroKey(0xffffffff, "", 0);
      }

   // entry for free mem
   desVal.SetPos(fDes[0] + fDes[1]);
   desVal.SetFileLength(fDes[2]);
   desVal.SetDataLength(fDes[2]);
   desVal.SetClassName(0xfffffffe);
   posEntry[desVal] = TFAsroKey(0xfffffffe, "", 0);

   // entry for not used mem
   if (fDes[3] > 0)
      {
      desVal.SetPos(fDes[0] + fDes[1] + fDes[2]);
      desVal.SetDataLength(fDes[3]);
      desVal.SetFileLength(fDes[3]);
      desVal.SetClassName(0xfffffffe);
      posEntry[desVal] = TFAsroKey(0xfffffffd, "", 0);
      }
   
   std::map<TFAsroValue,TFAsroKey>::iterator i_pos = posEntry.begin();

   UInt_t memIndex = 0;
   UInt_t prevEnd  = 24;
   UInt_t totalFree = 0;
   while (i_pos != posEntry.end())
      {
      while (memIndex < fDes[2] / (2 * sizeof(UInt_t)) &&
             fFree[memIndex * 2] < i_pos->first.GetPos())
         {
         MemTest(prevEnd, fFree[memIndex * 2]);
         printf("%10u %10u %20s\n", 
                fFree[memIndex * 2], fFree[memIndex* 2 + 1],
                "***  free  ***");
         prevEnd = fFree[memIndex * 2] + fFree[memIndex* 2 + 1];
         totalFree += fFree[memIndex* 2 + 1];
         memIndex++;
         }

      MemTest(prevEnd, i_pos->first.GetPos());
      const char * className;
      if (i_pos->first.GetClassName() == 0xffffffff)
         className = "TFAsroFile";
      else if (i_pos->first.GetClassName() == 0xfffffffe)
         className = "";
      else
         className = fClassNames[i_pos->first.GetClassName()].Data();
      
      const char * elName;
      if ( i_pos->second.GetElName() == 0xffffffff)
         elName = "data descriptor";
      else if ( i_pos->second.GetElName() == 0xfffffffe)
         elName = "free mem descriptor";
      else if ( i_pos->second.GetElName() == 0xfffffffd)
         elName = "not used memory";
      else
         elName = fNames[i_pos->second.GetElName()].Data();

      printf("%10u %10u %4.1f %20s %20s %3u %s\n",
             i_pos->first.GetPos(), i_pos->first.GetFileLength(),
             (double)i_pos->first.GetDataLength() / i_pos->first.GetFileLength(),
             elName, i_pos->second.GetSubName(),
             i_pos->second.GetCycle(), className );
      prevEnd = i_pos->first.GetPos() + i_pos->first.GetFileLength();
      i_pos++;
      }

   while (memIndex < fDes[2] / (2 * sizeof(UInt_t)))
      {
      MemTest(prevEnd, fFree[memIndex * 2]);
      printf("%10u %10u %20s\n", 
               fFree[memIndex * 2], fFree[memIndex* 2 + 1],
               "***  free  ***");
      prevEnd = fFree[memIndex * 2] + fFree[memIndex* 2 + 1];
      memIndex++;
      }

   printf("\n\n number of classNames:  %d   number of element names: %d\n",
          fClassNames.size(), fNames.size());
   printf("free memory in file: %u : %5.2f%%\n",
          totalFree,  double(totalFree) / fFree[(memIndex -1) * 2] * 100);



}
