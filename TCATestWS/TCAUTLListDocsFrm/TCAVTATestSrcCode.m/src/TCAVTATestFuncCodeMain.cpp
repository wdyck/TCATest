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
// TCAVTAReadFuncDir
//-----------------------------------------------------------------------------
void TCAVTAReadFuncDir(const CATUnicodeString& i_DirName, CATListOfCATUnicodeString& o_rList, CATBoolean i_All = FALSE)
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
      char fullPathName[2560];
      CATUnicodeString fileName(Entry.name);
      if (fileName == "." || fileName == ".." || fileName == "" || fileName == "misc")
      {
        continue;
      }
      if (i_All)
      {
        CATMakePath(i_DirName.ConvertToChar(), fileName.ConvertToChar(), fullPathName);
        CATDirectory newDir;
        CATLibStatus status = ::CATOpenDirectory(fullPathName, &newDir);
        if (CATLibError != status)
        {
          TCAVTAReadFuncDir(fullPathName, o_rList, TRUE);
        }
        else
        {
          o_rList.Append(fullPathName);
        }
      }
      else if ((fileName.SearchSubString("VTA") == 0 || fileName.SearchSubString("TCA") == 0 || fileName == "src")
               && fileName.SearchSubString("Interfaces") == -1
               && fileName.SearchSubString("TCAVTATestInfoMain") == -1)
      {
        CATMakePath(i_DirName.ConvertToChar(), fileName.ConvertToChar(), fullPathName);
        o_rList.Append(fullPathName);
        TCAVTAReadFuncDir(fullPathName, o_rList);
      }
    }
    CATCloseDirectory(&Dir);
  }
  return;
} // TCAVTAReadFuncDir

//-----------------------------------------------------------------------------
// TCAVTAFuncHasSubString
//-----------------------------------------------------------------------------
CATBoolean TCAVTAFuncHasSubString(const CATUnicodeString& i_rLine,
                                  const CATUnicodeString& i_rSubStr)
{
  CATBoolean hasSubStr = FALSE;
  if (i_rLine != "" && i_rSubStr != "")
  {
    hasSubStr = i_rLine.SearchSubString(i_rSubStr) > -1;
  }
  return hasSubStr;
} // TCAVTAFuncHasSubString

//-----------------------------------------------------------------------------
// TCAVTAParceFuncFile
//-----------------------------------------------------------------------------
void TCAVTAParceFuncFile(const CATUnicodeString& i_rFileName,
                         CATListOfCATUnicodeString& io_rFunctions)
{
  char* valueDir = new char[1024];
  char* valueFile = new char[256];
  ::CATSplitPath(i_rFileName.ConvertToChar(), valueDir, valueFile);

  CATUnicodeString className = valueFile;
  className.ReplaceAll(".cpp", "");
  className.Append("::");

  delete[] valueDir;
  delete[] valueFile;

  CATListOfInt indexes;
  CATListOfCATUnicodeString includes;
  CATListOfCATUnicodeString fileLines;
  TCADRTReadFile(i_rFileName, fileLines, FALSE, FALSE);
  CATUnicodeString compNameLine;
  CATListOfCATUnicodeString descLines;
  for (int l = 1; l <= fileLines.Size(); l++)
  {
    CATUnicodeString fLine = fileLines[l];
    if (TCAVTAFuncHasSubString(fLine, className))
    {
      int idx = fLine.SearchSubString(className);
      fLine = fLine.SubString(idx, fLine.GetLengthInChar() - idx);
      fLine.ReplaceAll(className, "");
      idx = fLine.SearchSubString("(");
      if (idx > -1)
      {
        fLine = fLine.SubString(0, idx);
        if (className.SearchSubString(fLine) == -1 &&
            fLine.SearchSubString("~") == -1 &&
            fLine.SearchSubString("doIt") == -1 &&
            fLine.SearchSubString("runTest") == -1)
        {
          fLine.Append("(");
          io_rFunctions.Append(fLine);
        }
      }
    }
  }
  io_rFunctions.RemoveDuplicates();
  return;
} // TCAVTAParceFuncFile

//-----------------------------------------------------------------------------
// TCAVTAParceFuncFiles
//-----------------------------------------------------------------------------
void TCAVTAParceFuncFiles(const CATUnicodeString& i_rDir,
                          CATListOfCATUnicodeString& io_rFunctions)
{
  CATListOfCATUnicodeString hdrFiles;
  CATListOfCATUnicodeString dirEntries;
  TCAVTAReadFuncDir(i_rDir, dirEntries, TRUE);

  cerr << "dirEntries: " << dirEntries.Size() << endl;
  for (int d = 1; d <= dirEntries.Size(); d++)
  {
    CATUnicodeString fileFunc = dirEntries[d];
    int length = fileFunc.GetLengthInChar();
    int idx = fileFunc.SearchSubString(".cpp");
    int diff = length - idx;
    if (diff == 4 && fileFunc.SearchSubString("Settings") == -1)
    {
      hdrFiles.Append(fileFunc);
    }
  }

  int cppCount = hdrFiles.Size();
  cerr << "cppCount: " << cppCount << endl;
  for (int p = 1; p <= cppCount; p++)
  {
    CATListOfCATUnicodeString funcNames;
    TCAVTAParceFuncFile(hdrFiles[p], funcNames);
    io_rFunctions.Append(funcNames);
  }
  return;
} // TCAVTAParceFuncFiles

//-----------------------------------------------------------------------------
// TCAVTAGetFuncCppFiles
//-----------------------------------------------------------------------------
void TCAVTAGetFuncCppFiles(const CATUnicodeString& i_rDir, CATListOfCATUnicodeString& o_rFiles)
{
  if (i_rDir != "")
  {
    CATListOfCATUnicodeString dirEntries;
    TCAVTAReadFuncDir(i_rDir, dirEntries, TRUE);
    for (int d = 1; d <= dirEntries.Size(); d++)
    {
      CATUnicodeString fileFunc = dirEntries[d];
      int length = fileFunc.GetLengthInChar();
      int idx = fileFunc.SearchSubString(".cpp");
      int diff = length - idx;
      if (diff == 4)
      {
        o_rFiles.Append(fileFunc);
      }
    }
  }
  return;
} // TCAVTAGetFuncCppFiles

//----------------------------------------------------------------------------------
// TCAVTAIsFunctionUsed
//----------------------------------------------------------------------------------
CATBoolean TCAVTAIsFunctionUsed(const CATUnicodeString& i_rFunction,
                                const CATListOfCATUnicodeString& i_rFiles)
{
  if (i_rFunction.SearchSubString("Tst") > -1 ||
      i_rFunction.SearchSubString("VTAN") == 0 ||
      i_rFunction.SearchSubString("VTAGoSurfClip") == 0 ||
      i_rFunction.SearchSubString("VTAGoGroup") == 0)
  {
    // return TRUE;
  }

  CATBoolean isUsed = FALSE;
  for (int f = 1; f <= i_rFiles.Size(); f++)
  {
    if (i_rFiles[f].SearchSubString("TCAEVTAGo.cpp") > -1)
    {
      // continue;
    }

    char* valueDir = new char[1024];
    char* valueFile = new char[256];
    ::CATSplitPath(i_rFiles[f].ConvertToChar(), valueDir, valueFile);
    CATUnicodeString funcDef = valueFile;
    funcDef.ReplaceAll(".cpp", "");
    funcDef.Append("::");
    funcDef.Append(i_rFunction);

    CATUnicodeString funcPtr("&");
    funcPtr.Append(funcDef);
    funcPtr.ReplaceAll("(", "");

    CATListOfCATUnicodeString fileLines;
    TCADRTReadFile(i_rFiles[f], fileLines, FALSE, FALSE);
    for (int i = 1; i <= fileLines.Size(); i++)
    {
      if (TCAVTAFuncHasSubString(fileLines[i], i_rFunction) || TCAVTAFuncHasSubString(fileLines[i], funcPtr))
      {
        CATUnicodeString lineStr = fileLines[i].Strip(CATUnicodeString::CATStripModeAll);

        /*  if (funcPtr == "&TCAPWPAutoUpdateDlg::onShowReport")
          {
            cerr << "funcPtr " << funcPtr << endl;
            cerr << "lineStr " << lineStr << endl;
            cerr << "i_rFunction " << i_rFunction << endl;
          }*/

        if (!TCAVTAFuncHasSubString(lineStr, funcDef) || TCAVTAFuncHasSubString(lineStr, funcPtr))
        {
          // cerr << "USED: " << i_rFunction << " in " << fileLines[i] << endl;
          isUsed = TRUE;
          break;
        }
      }
    }
    if (isUsed)
    {
      break;
    }
  }

  return isUsed;
} // TCAVTAIsFunctionUsed

//----------------------------------------------------------------------------------
// main
//----------------------------------------------------------------------------------
int main1(int iArgc, char** iArgv)
{
  cerr << "*** VTA Test Func Code START ***" << endl << endl;
  if (iArgc > 1)
  {
    int cntNotUsed = 0;
    CATListOfCATUnicodeString funcNames;
    TCAVTAParceFuncFiles(iArgv[1], funcNames);
    CATListOfCATUnicodeString cppFiles;
    TCAVTAGetFuncCppFiles(iArgv[1], cppFiles);
    for (int m = 1; m <= funcNames.Size(); m++)
    {
      // cerr << funcNames[m] << endl;
      if (!TCAVTAIsFunctionUsed(funcNames[m], cppFiles))
      {
        cerr << "NOT USED: " << funcNames[m] << endl;
        cntNotUsed++;
      }
    }
    cerr << "Count total: " << funcNames.Size() << endl;
    cerr << "Not used   : " << cntNotUsed << endl;
  }
  else
  {
    cerr << endl << "MISSING input directory" << endl;
  }
  cerr << endl << "*** VTA Test Func Code END   ***" << endl;
  return 0;
} // main