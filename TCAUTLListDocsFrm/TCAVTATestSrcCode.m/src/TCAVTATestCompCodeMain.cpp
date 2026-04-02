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
// TCAVTAReadCompDir
//-----------------------------------------------------------------------------
void TCAVTAReadCompDir(const CATUnicodeString& i_DirName, CATListOfCATUnicodeString& o_rList, CATBoolean i_All = FALSE)
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
          TCAVTAReadCompDir(fullPathName, o_rList, TRUE);
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
        TCAVTAReadCompDir(fullPathName, o_rList);
      }
    }
    CATCloseDirectory(&Dir);
  }
  return;
} // TCAVTAReadCompDir

//-----------------------------------------------------------------------------
// TCAVTACompHasSubString
//-----------------------------------------------------------------------------
CATBoolean TCAVTACompHasSubString(const CATUnicodeString& i_rLine,
                                  const CATUnicodeString& i_rSubStr)
{
  CATBoolean hasSubStr = FALSE;
  if (i_rLine != "" && i_rSubStr != "")
  {
    hasSubStr = i_rLine.SearchSubString(i_rSubStr) > -1;
  }
  return hasSubStr;
} // TCAVTACompHasSubString

//-----------------------------------------------------------------------------
// TCAVTAParceCompFile
//-----------------------------------------------------------------------------
void TCAVTAParceCompFile(const CATUnicodeString& i_rFileName,
                         CATListOfCATUnicodeString& io_rComponents)
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
    if (TCAVTACompHasSubString(fLine, "TCA_PWP_COMPONENT_H"))
    {
      fLine = fLine.Strip(CATUnicodeString::CATStripModeAll);
      fLine.ReplaceAll("(", "");
      fLine.ReplaceAll(")", "");
      fLine.ReplaceAll(";", "");
      int idx = fLine.SearchSubString("TCA_PWP_COMPONENT_H");
      if (idx == 0)
      {
        fLine.ReplaceAll("TCA_PWP_COMPONENT_H", "");
        io_rComponents.Append(fLine);
      }
    }
  }
  io_rComponents.RemoveDuplicates();
  return;
} // TCAVTAParceCompFile

//-----------------------------------------------------------------------------
// TCAVTAParceCompFiles
//-----------------------------------------------------------------------------
void TCAVTAParceCompFiles(const CATUnicodeString& i_rDir,
                          CATListOfCATUnicodeString& io_rComponents)
{ 
  if (i_rDir != "" )
  {
    CATListOfCATUnicodeString hdrFiles;
    CATListOfCATUnicodeString dirEntries;
    TCAVTAReadCompDir(i_rDir, dirEntries, TRUE);

    cerr << "dirEntries: " << dirEntries.Size() << endl;
    for (int d = 1; d <= dirEntries.Size(); d++)
    {
      CATUnicodeString fileComp = dirEntries[d];
      int length = fileComp.GetLengthInChar();
      int idx = fileComp.SearchSubString(".h");
      int diff = length - idx;
      if (diff == 2)
      {
        hdrFiles.Append(fileComp);
      }
    }

    int hdrCount = hdrFiles.Size();
    cerr << "hdrCount: " << hdrCount << endl;
    for (int p = 1; p <= hdrCount; p++)
    {
      CATListOfCATUnicodeString compNames;
      TCAVTAParceCompFile(hdrFiles[p], compNames);
      io_rComponents.Append(compNames);
    }
  }
  return;
} // TCAVTAParceCompFiles

//-----------------------------------------------------------------------------
// TCAVTAGetCompCppFiles
//-----------------------------------------------------------------------------
void TCAVTAGetCompCppFiles(const CATUnicodeString& i_rDir, CATListOfCATUnicodeString& o_rFiles)
{
  if (i_rDir != "")
  {
    CATListOfCATUnicodeString dirEntries;
    TCAVTAReadCompDir(i_rDir, dirEntries, TRUE);
    for (int d = 1; d <= dirEntries.Size(); d++)
    {
      CATUnicodeString fileComp = dirEntries[d];
      int length = fileComp.GetLengthInChar();
      int idx = fileComp.SearchSubString(".cpp");
      int diff = length - idx;
      if (diff == 4)
      {
        o_rFiles.Append(fileComp);
      }
    }
  }
  return;
} // TCAVTAGetCompCppFiles

//----------------------------------------------------------------------------------
// TCAVTAIsComponentUsed
//----------------------------------------------------------------------------------
CATBoolean TCAVTAIsComponentUsed(const CATUnicodeString& i_rComponent,
                                 const CATListOfCATUnicodeString& i_rFiles)
{
  if (i_rComponent.SearchSubString("Tst") > -1 ||
      i_rComponent.SearchSubString("VTAN") == 0 ||
      i_rComponent.SearchSubString("VTAGoSurfClip") == 0 ||
      i_rComponent.SearchSubString("VTAGoGroup") == 0)
  {
    return TRUE;
  }
  CATBoolean isUsed = FALSE;
  for (int f = 1; f <= i_rFiles.Size(); f++)
  {
    if (i_rFiles[f].SearchSubString("TCAEVTAGo.cpp") > -1)
    {
      continue;
    }
    CATListOfCATUnicodeString fileLines;
    TCADRTReadFile(i_rFiles[f], fileLines, FALSE, FALSE);
    for (int i = 1; i <= fileLines.Size(); i++)
    {
      if (TCAVTACompHasSubString(fileLines[i], i_rComponent) &&
          !TCAVTACompHasSubString(fileLines[i], "COMPONENT") &&
          !TCAVTACompHasSubString(fileLines[i], "CATAddClassExtension"))
      {
        CATUnicodeString lineStr = fileLines[i].Strip(CATUnicodeString::CATStripModeAll);
        CATUnicodeString comp1 = "(" + i_rComponent + ")";
        CATUnicodeString comp2 = "\"" + i_rComponent + "\"";
        if (TCAVTACompHasSubString(lineStr, comp1) || TCAVTACompHasSubString(lineStr, comp2))
        {
          int idx1 = lineStr.SearchSubString("//");
          int idx2 = lineStr.SearchSubString(i_rComponent);
          if (idx1 < 0 || idx1 > idx2)
          {
            // cerr << "USED: " << i_rComponent << " in " << fileLines[i] << endl;
            isUsed = TRUE;
            break;
          }
        }
      }
    }
    if (isUsed)
    {
      break;
    }
  }
  return isUsed;
} // TCAVTAIsComponentUsed

//----------------------------------------------------------------------------------
// main
//----------------------------------------------------------------------------------
int main3(int iArgc, char** iArgv)
{
  cerr << "*** VTA Test Comp Code START ***" << endl << endl; 
  if ( iArgc > 1)
  {
    CATListOfCATUnicodeString compNames;
    TCAVTAParceCompFiles(iArgv[1], compNames);
    CATListOfCATUnicodeString cppFiles;
    TCAVTAGetCompCppFiles(iArgv[1], cppFiles);

    for (int m = 1; m <= compNames.Size(); m++)
    {
      if (!TCAVTAIsComponentUsed(compNames[m], cppFiles))
      {
        cerr << "NOT USED: " << compNames[m] << endl;
      }
    }
  }
  else
  {
    cerr << endl << "MISSING input directory" << endl;
  }
  cerr << endl << "*** VTA Test Comp Code END   ***" << endl;
  return 0;
} // main