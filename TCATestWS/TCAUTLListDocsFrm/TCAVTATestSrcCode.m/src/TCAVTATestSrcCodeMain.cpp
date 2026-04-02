#ifdef _WIN32
#include <direct.h>
// MSDN recommends against using getcwd & chdir names
#define cwd _getcwd
#define cd _chdir
#else
#include "unistd.h"
#define cwd getcwd
#define cd chdir
#endif
 
#include "CATListOfCATUnicodeString.h" 
#include "CATUnicodeString.h"
#include "iostream.h" 
#include "TCAXmlUtils.h" 

//-----------------------------------------------------------------------------
// TCAVTAReadSrcDir
//-----------------------------------------------------------------------------
void TCAVTAReadSrcDir(const CATUnicodeString& i_DirName, CATListOfCATUnicodeString& o_rList, CATBoolean i_All = FALSE)
{
  CATDirectory Dir;
  CATLibStatus status = ::CATOpenDirectory(i_DirName, &Dir);
  if (CATLibError != status)
  {
    int EndOfDir = 0;
    CATDirectoryEntry Entry;
    while ((EndOfDir != 1) && (CATLibSuccess == status))
    {
      status = ::CATReadDirectory(&Dir, &Entry, &EndOfDir);
      if ((CATLibError == status) && (EndOfDir != 1))
      {
        cerr << "\nERROR READING NEXT ENTRY IN:\n" << i_DirName.ConvertToChar() << endl;
      }
      char fullPathName[256];
      CATUnicodeString fileName(Entry.name);
      if (i_All)
      {
        CATMakePath(i_DirName.ConvertToChar(), fileName.ConvertToChar(), fullPathName);
        o_rList.Append(fullPathName);
      }
      else if ((fileName.SearchSubString("VTA") == 0 || fileName.SearchSubString("TCA") == 0 || fileName == "src")
               && fileName.SearchSubString("Interfaces") == -1
               && fileName.SearchSubString("TCAVTATestInfoMain") == -1)
      {
        CATMakePath(i_DirName.ConvertToChar(), fileName.ConvertToChar(), fullPathName);
        o_rList.Append(fullPathName);
        TCAVTAReadSrcDir(fullPathName, o_rList);
      }
    }
    CATCloseDirectory(&Dir);
  }
  return;
} // TCAVTAReadSrcDir

//-----------------------------------------------------------------------------
// TCAVTAHasSubString
//-----------------------------------------------------------------------------
CATBoolean TCAVTAHasSubString(const CATUnicodeString& i_rLine,
                              const CATUnicodeString& i_rSubStr)
{
  CATBoolean hasSubStr = FALSE;
  if (i_rLine != "" && i_rSubStr != "")
  {
    hasSubStr = i_rLine.SearchSubString(i_rSubStr) > -1;
  }
  return hasSubStr;
} // TCAVTAHasSubString

//-----------------------------------------------------------------------------
// TCAVTAHasSubString
//-----------------------------------------------------------------------------
CATUnicodeString TCAVTAGetInclude(const CATUnicodeString& i_rLine)
{
  CATUnicodeString includeString = i_rLine;
  includeString.ReplaceAll("#include", "");
  includeString.ReplaceAll("\"", "");
  includeString.ReplaceAll(" ", "");
  includeString.ReplaceAll(".h", "");
  return includeString;
} // TCAVTAHasSubString

//-----------------------------------------------------------------------------
// TCAVTAWriteSrcFile
//-----------------------------------------------------------------------------
void TCAVTAWriteSrcFile(const CATUnicodeString& i_FileName,
                        const CATListOfCATUnicodeString i_rFileLines,
                        const CATListOfInt& i_rToIgnore)
{

  cerr << endl << "*** i_FileName *** " << endl;
  ofstream outputFileStream;
  outputFileStream.open(i_FileName.ConvertToChar(), ios::out);
  if (outputFileStream)
  {
    for (int l = 1; l <= i_rFileLines.Size(); l++)
    {
      if (i_rToIgnore.Locate(l) == 0)
      {
        outputFileStream << i_rFileLines[l].ConvertToChar() << endl;
      }
      else
      {
        cerr << i_rFileLines[l] << endl;
      }
    }    
    outputFileStream.close();
  }
  cerr << "*** END *** " << endl << endl;
  return;
} // TCAVTAWriteSrcFile

//-----------------------------------------------------------------------------
// TCAVTAParceSrcFile
//-----------------------------------------------------------------------------
void TCAVTAParceSrcFile(const CATUnicodeString& i_rFileName)
{
  CATListOfInt indexes;
  CATListOfCATUnicodeString includes;
  CATListOfCATUnicodeString fileLines;
  TCADRTReadFile(i_rFileName, fileLines, FALSE, FALSE);
  CATUnicodeString compNameLine;
  CATListOfCATUnicodeString descLines;
  for (int l = 1; l <= fileLines.Size(); l++)
  {
    CATUnicodeString fLine = fileLines[l];
    if (TCAVTAHasSubString(fLine, "#include"))
    {
      if (TCAVTAHasSubString(fLine, "\"TCAI") || TCAVTAHasSubString(fLine, "\"CATI"))
      {
        includes.Append(TCAVTAGetInclude(fLine));
        indexes.Append(l);
      }
    }
    else
    {
      for (int i = includes.Size(); i >= 1; i--)
      {
        if (TCAVTAHasSubString(fLine, includes[i]))
        {
          includes.RemovePosition(i);
          indexes.RemovePosition(i);
        }
      }
    }
  }
  if (indexes.Size() > 0)
  {
    TCAVTAWriteSrcFile(i_rFileName, fileLines, indexes);
  }
  return;
} // TCAVTAParceSrcFile

//-----------------------------------------------------------------------------
// TCAVTAParceCppFiles
//-----------------------------------------------------------------------------
void TCAVTAParceCppFiles(const CATUnicodeString& i_rDir)
{
  if (i_rDir != "")
  {
    CATListOfCATUnicodeString cppFiles;
    CATListOfCATUnicodeString dirEntries;
    TCAVTAReadSrcDir(i_rDir, dirEntries);

    for (int d = 1; d <= dirEntries.Size(); d++)
    {
      CATUnicodeString fileCpp = dirEntries[d];
      int length = fileCpp.GetLengthInChar();
      int idx = fileCpp.SearchSubString(".cpp");
      int diff = length - idx;
      if (diff == 4)
      {
        cppFiles.Append(fileCpp);
      }
    }
    for (int p = 1; p <= cppFiles.Size(); p++)
    {
      TCAVTAParceSrcFile(cppFiles[p]);
    }
  }
  return;
} // TCAVTAParceCppFiles

//----------------------------------------------------------------------------------
// main
//----------------------------------------------------------------------------------
int main6(int iArgc, char** iArgv)
{
  cerr << "*** VTA Test Src Code START ***" << endl << endl; 
  if ( iArgc > 1)
  {
    TCAVTAParceCppFiles(iArgv[1]);
  }
  else
  {
    cerr << endl << "MISSING input directory" << endl;
  }
  cerr << endl << "*** VTA Test Src Code END   ***" << endl;
  return 0;
} // main