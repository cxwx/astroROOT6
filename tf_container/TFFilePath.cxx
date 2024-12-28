// ///////////////////////////////////////////////////////////////////
//
//  File:      TFFilePath.cpp
//
//  Version:   1.0
//
//  Author:    Reiner Rohlfs (GADC)
//
//  History:   1.0   08.08.03  first released version
//
// ///////////////////////////////////////////////////////////////////
#include "TSystem.h"
#include "TFGroup.h"

ClassImp(TFFilePath)

//_____________________________________________________________________________
// The string of TFFilePath is a file name including the path
// The path can be absolute or relative. 

#ifdef WIN32
#define        DOT_DOT        "\\..\\"
#define        DOT            "\\.\\"
#define        DOT_DOT_SLASH  "..\\"
#define        DS             '\\'       // Directory Seperator
#define        DS2            "\\"       // Directory Seperator
#else 
#define        DOT_DOT        "/../"
#define        DOT            "/./"
#define        DOT_DOT_SLASH  "../"
#define        DS             '/'        // Directory Seperator
#define        DS2            "/"        // Directory Seperator
#endif


//_____________________________________________________________________________
static void RemoveDots(char * str)
{
   // first remove /./
   char * pos2 = str;
   while (pos2 = strstr(pos2, DOT))
      strcpy(pos2 + 1, pos2 + 3);

   pos2 = str;
   while (char * pos = strstr(pos2, DOT_DOT))
      {
      pos2 = pos -1;
      while (*pos2 != DS && pos2 != str)
         pos2--;
      strcpy(pos2, pos + 3);
      }

}

//_____________________________________________________________________________
Bool_t TFFilePath::IsRelativePath() const
{
//  test if this is an relative path

#ifdef WIN32
   if (Data()[1] == ':' || Data()[0] == DS)
      return kFALSE;
#else 
   if (Data()[0] == DS)
      return kFALSE;
#endif
   return kTRUE;
}

//_____________________________________________________________________________

void TFFilePath::MakeAbsolutePath(const char * isRelativeTo)
{
// Converts an relative path to an absolute path. If this path is already
// absolute it remove /../ if there are any in this path.
// It is assumed that this path is relative to the path isRelativeTo.
// isRelativeTo has to be either an absolute path or must be relative to
// the working directory

   const char * currDir = gSystem->WorkingDirectory();
   size_t   maxLength;
   char  *  str;

   maxLength = strlen(currDir) + strlen(isRelativeTo) + Length() + 3;
   str = new char[maxLength];

#ifdef WIN32
   if (Data()[1] == ':')
      // path is already absolute, but remove the dots
      str[0] = 0;

   else if (Data()[0] == DS)
      // path is already absolute but without derive
      {
      str[0] = currDir[0];
      str[1] = currDir[1];
      str[2] = 0;
      } 
#else 
   if (Data()[0] == DS)
      // path is already absolute but remove the dots
      {
      str[0] = 0;
      } 
#endif

   else
      // path is relative to "isRelativeTo"
      {

#ifdef WIN32
      // make "relative" an absolute path if it is not already
      if (isRelativeTo[1] == ':')
         {
         // relative is an absolute path including drive
         strcpy(str, isRelativeTo);
         }
      else if (isRelativeTo[0] == DS)
         {
         // relative is an absolute path without drive
         str[0] = currDir[0];
         str[1] = currDir[1];
         str[2] = 0;
         strcat(str, isRelativeTo);
         }
#else 
      if (isRelativeTo[0] == DS)
         {
         // relative is an absolute path
         strcpy(str, isRelativeTo);
         }
#endif
      else
         {
         // relative is an relative path to the current directory
         strcpy(str, currDir);
         strcat(str, DS2);
         strcat(str, isRelativeTo);
         }

      // remove the filename of relative
      char * pos = strrchr(str, DS);
      if (pos)    *(pos+1) = 0;
      }
      
   strcat(str, Data());

   RemoveDots(str);

   *this = str;

   delete [] str;
}
//_____________________________________________________________________________
void TFFilePath::MakeRelativePath(const char * relativeTo)
{
// Converts this path to an relative path which will be relative to 
// "relativeTo".
// This path has to be an absolute path. If is is not already an
// absolute path first call MakeAbsolutePath() before calling this
// function.
// relativeTo has to be either an absolute path or must be relative to
// the working directory

   const char * currDir = gSystem->WorkingDirectory();

   size_t   maxLength;
   char * base;


   // make relativeTo to an absolute path
   maxLength = strlen(currDir) + strlen(relativeTo) + 3;
   base = new char[maxLength];

#ifdef WIN32
   if (relativeTo[1] == ':')
      {
      // relativeTo is an absolute path including drive
      strcpy(base, relativeTo);
      }
   else if (relativeTo[0] == DS)
      {
      // relative is an absolute path without drive
      base[0] = currDir[0];
      base[1] = currDir[1];
      base[2] = 0;
      strcat(base, relativeTo);
      }
#else 
   if (relativeTo[0] == DS)
      {
      // relative is an absolute path
      strcpy(base, relativeTo);
      }
#endif
   else
      {
      // relative is an relative path to the current directory
      strcpy(base, currDir);
      strcat(base, DS2);
      strcat(base, relativeTo);
      }

   // remove the filename of isRelative
   char * pos = strrchr(base, DS);
   if (pos)    *(pos+1) = 0;

   RemoveDots(base);


   // remove the path that is common in base and in this str
   char * str;
   char * basePos1, *basePos2;
   char * strPos1,  *strPos2;
   str = new char[Length() + 1];

   strcpy(str, Data());

   strPos1 = str;
   basePos1 = base;

   do
      {   
      basePos2 = strchr(basePos1, DS);
      if (basePos2 == NULL)
         break;

      strPos2 = strchr(strPos1, DS);
      if (strPos2 == NULL)
         break;

      if (basePos2 - basePos1 != strPos2 - strPos1)
         break;

      if (strncmp(strPos1, basePos1, strPos2 - strPos1))
         break;            
   
      strPos1  = strPos2 + 1;
      basePos1 = basePos2 + 1;

      } while(1);

   char * result;

   result = new char[2 * strlen(basePos1) + strlen(strPos1) +2];
   result[0] = 0;

   while (basePos1 = strchr(basePos1 + 1, DS))
      {
      strcat(result, DOT_DOT_SLASH);
      } 

   strcat(result, strPos1);

   * this = result;

   delete [] result;
   delete [] str;
   delete [] base;
}
