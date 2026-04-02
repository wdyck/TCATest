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
// TCAVTAReadHdrDir
//-----------------------------------------------------------------------------
void TCAVTAReadHdrDir(const CATUnicodeString& i_DirName, CATListOfCATUnicodeString& o_rList, CATBoolean i_All = FALSE)
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
      if (fileName == "." || fileName == ".." || fileName == "")
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
          TCAVTAReadHdrDir(fullPathName, o_rList, TRUE);
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
        TCAVTAReadHdrDir(fullPathName, o_rList);
      }
    }
    CATCloseDirectory(&Dir);
  }
  return;
} // TCAVTAReadHdrDir

//-----------------------------------------------------------------------------
// TCAVTAHdrHasSubString
//-----------------------------------------------------------------------------
CATBoolean TCAVTAHdrHasSubString(const CATUnicodeString& i_rLine,
                                 const CATUnicodeString& i_rSubStr)
{
  CATBoolean hasSubStr = FALSE;
  if (i_rLine != "" && i_rSubStr != "")
  {
    hasSubStr = i_rLine.SearchSubString(i_rSubStr) > -1;
  }
  return hasSubStr;
} // TCAVTAHdrHasSubString

//-----------------------------------------------------------------------------
// TCAVTAHdrHasSubString
//-----------------------------------------------------------------------------
CATUnicodeString TCAVTAHdrGetInclude(const CATUnicodeString& i_rLine)
{
  CATUnicodeString includeString = i_rLine;
  includeString.ReplaceAll("#include", "");
  includeString.ReplaceAll("\"", "");
  includeString.ReplaceAll(" ", "");
  includeString.ReplaceAll(".h", "");
  return includeString;
} // TCAVTAHdrHasSubString

//-----------------------------------------------------------------------------
// TCAVTAWriteHdrFile
//-----------------------------------------------------------------------------
void TCAVTAWriteHdrFile(const CATUnicodeString& i_FileName,
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
} // TCAVTAWriteHdrFile

//-----------------------------------------------------------------------------
// TCAVTAParceInterfaceFile
//-----------------------------------------------------------------------------
void TCAVTAParceInterfaceFile(const CATUnicodeString& i_rFileName,
                              CATListOfCATUnicodeString& io_rMethods)
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
    if (TCAVTAHdrHasSubString(fLine, "virtual"))
    {
      int idx = fLine.SearchSubString("(");
      if (idx > 0)
      {
        fLine = fLine.SubString(0, idx);
        int idx1 = fLine.SearchSubString(" ", 0, CATUnicodeString::CATSearchModeBackward);
        if (idx1 > 0)
        {
          fLine = fLine.SubString(idx1 + 1, fLine.GetLengthInChar() - idx1 - 1);
          if (fLine != "")
          {
            io_rMethods.Append(fLine);
          }
        }
      }
    }
  }
  io_rMethods.RemoveDuplicates();
  return;
} // TCAVTAParceInterfaceFile

//-----------------------------------------------------------------------------
// TCAVTAParceHdrFile
//-----------------------------------------------------------------------------
void TCAVTAParceHdrFile(const CATUnicodeString& i_rFileName,
                        CATListOfCATUnicodeString& io_rMethods)
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
    if (TCAVTAHdrHasSubString(fLine, "ExportedByTCA"))
    {
      int idx = fLine.SearchSubString("(");
      if (idx > 0)
      {
        fLine = fLine.SubString(0, idx);
        int idx1 = fLine.SearchSubString(" ", 0, CATUnicodeString::CATSearchModeBackward);
        if (idx1 > 0)
        {
          fLine = fLine.SubString(idx1 + 1, fLine.GetLengthInChar() - idx1 - 1);
          if (fLine != "")
          {
            io_rMethods.Append(fLine);
          }
        }
      }
    }
  }
  io_rMethods.RemoveDuplicates();
  return;
} // TCAVTAParceHdrFile

//-----------------------------------------------------------------------------
// TCAVTAParceHdrFiles
//-----------------------------------------------------------------------------
void TCAVTAParceHdrFiles(const CATUnicodeString& i_rDir,
                         CATListOfCATUnicodeString& io_rMethods,
                         const CATBoolean& i_rInterfaces = FALSE)
{ 
  if (i_rDir != "" )
  {
    CATListOfCATUnicodeString hdrFiles;
    CATListOfCATUnicodeString dirEntries;
    TCAVTAReadHdrDir(i_rDir, dirEntries, TRUE);

    cerr << "dirEntries: " << dirEntries.Size() << endl;
    for (int d = 1; d <= dirEntries.Size(); d++)
    {
      CATUnicodeString fileHdr = dirEntries[d];
      int length = fileHdr.GetLengthInChar();
      int idx = fileHdr.SearchSubString(".h");
      int diff = length - idx;
      if (diff == 2)
      {
        hdrFiles.Append(fileHdr);
      }
    }

    int hdrCount = hdrFiles.Size();
    cerr << "hdrCount: " << hdrCount << endl;
    for (int p = 1; p <= hdrCount; p++)
    {
      CATListOfCATUnicodeString methods;
      if (i_rInterfaces)
      {
        TCAVTAParceInterfaceFile(hdrFiles[p], methods);
      }
      else
      {
        TCAVTAParceHdrFile(hdrFiles[p], methods);
      }
      io_rMethods.Append(methods);
    }
  }
  return;
} // TCAVTAParceHdrFiles

//-----------------------------------------------------------------------------
// TCAVTAParceInterfaceFiles
//-----------------------------------------------------------------------------
void TCAVTAParceInterfaceFiles(const CATUnicodeString& i_rDir, CATListOfCATUnicodeString& io_rMethods)
{ 
  if (i_rDir != "" )
  {
    CATListOfCATUnicodeString hdrFiles;
    CATListOfCATUnicodeString dirEntries;
    TCAVTAReadHdrDir(i_rDir, dirEntries, TRUE);

    cerr << "dirEntries: " << dirEntries.Size() << endl;
    for (int d = 1; d <= dirEntries.Size(); d++)
    {
      CATUnicodeString fileHdr = dirEntries[d];
      int length = fileHdr.GetLengthInChar();
      int idx = fileHdr.SearchSubString(".h");
      int diff = length - idx;
      if (diff == 2)
      {
        hdrFiles.Append(fileHdr);
      }
    }

    int hdrCount = hdrFiles.Size();
    cerr << "hdrCount: " << hdrCount << endl;
    for (int p = 1; p <= hdrCount; p++)
    {
      CATListOfCATUnicodeString methods;
      TCAVTAParceHdrFile(hdrFiles[p], methods);
      io_rMethods.Append(methods);
    }
  }
  return;
} // TCAVTAParceInterfaceFiles

//-----------------------------------------------------------------------------
// TCAVTAGetCppFiles
//-----------------------------------------------------------------------------
void TCAVTAGetCppFiles(const CATUnicodeString& i_rDir, CATListOfCATUnicodeString& o_rFiles)
{
  if (i_rDir != "")
  {
    CATListOfCATUnicodeString dirEntries;
    TCAVTAReadHdrDir(i_rDir, dirEntries, TRUE);
    for (int d = 1; d <= dirEntries.Size(); d++)
    {
      CATUnicodeString fileCpp = dirEntries[d];
      int length = fileCpp.GetLengthInChar();
      int idx = fileCpp.SearchSubString(".cpp");
      int diff = length - idx;
      if (diff == 4)
      {
        o_rFiles.Append(fileCpp);
      }
    }
  }
  return;
} // TCAVTAGetCppFiles

//----------------------------------------------------------------------------------
// TCAVTAIsMethodUsed
//----------------------------------------------------------------------------------
CATBoolean TCAVTAIsMethodUsed(const CATUnicodeString& i_rMethod, const CATListOfCATUnicodeString& i_rFiles)
{
  CATBoolean isUsed = FALSE;
  for (int f = 1; f <= i_rFiles.Size(); f++)
  {
    CATListOfCATUnicodeString fileLines;
    TCADRTReadFile(i_rFiles[f], fileLines, FALSE, FALSE);
    for (int i = 1; i <= fileLines.Size(); i++)
    {
      if (TCAVTAHdrHasSubString(fileLines[i], i_rMethod))
      {
        int idx1 = fileLines[i].SearchSubString(" ") + 1;
        int idx2 = fileLines[i].SearchSubString(i_rMethod);
        int idx3 = fileLines[i].SearchSubString("//");
        if (!(idx1 > 0 && idx2 == idx1) && !(idx3 > -1 && idx3 < idx2))
        {
          // cerr << i_rMethod << endl;
          // cerr << fileLines[i] << endl << endl;
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
} // TCAVTAIsMethodUsed

//----------------------------------------------------------------------------------
// main
//----------------------------------------------------------------------------------
int main5(int iArgc, char** iArgv)
{
  cerr << "*** VTA Test Hdr Code START ***" << endl << endl; 
  if (  iArgc > 1)
  {
    CATListOfCATUnicodeString methods;
    TCAVTAParceHdrFiles(iArgv[1], methods, TRUE);

    CATListOfCATUnicodeString cppFiles;
    TCAVTAGetCppFiles(iArgv[1], cppFiles);
    for (int m = 1; m <= methods.Size(); m++)
    {
      if (!TCAVTAIsMethodUsed(methods[m], cppFiles))
      {
        cerr << methods[m] << endl;
      }
    }
  }
  else
  {
    cerr << endl << "MISSING input directory" << endl;
  }
  cerr << endl << "*** VTA Test Hdr Code END   ***" << endl;
  return 0;
} // main