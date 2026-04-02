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
#include "TCAXmlUtils.h"


//-----------------------------------------------------------------------------
// TCAVTAReadCmtDir
//-----------------------------------------------------------------------------
void TCAVTAReadCmtDir(const CATUnicodeString& i_DirName, CATListOfCATUnicodeString& o_rList, CATBoolean i_All = FALSE)
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
          TCAVTAReadCmtDir(fullPathName, o_rList, TRUE);
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
        TCAVTAReadCmtDir(fullPathName, o_rList);
      }
    }
    CATCloseDirectory(&Dir);
  }
  return;
} // TCAVTAReadCmtDir

//-----------------------------------------------------------------------------
// TCAVTACmtHasSubString
//-----------------------------------------------------------------------------
CATBoolean TCAVTACmtHasSubString(const CATUnicodeString& i_rLine,
                                 const CATUnicodeString& i_rSubStr)
{
  CATBoolean hasSubStr = FALSE;
  if (i_rLine != "" && i_rSubStr != "")
  {
    hasSubStr = i_rLine.SearchSubString(i_rSubStr) > -1;
  }
  return hasSubStr;
} // TCAVTACmtHasSubString

//-----------------------------------------------------------------------------
// TCAVTACmtHasSubString
//-----------------------------------------------------------------------------
CATUnicodeString TCAVTACmtGetInclude(const CATUnicodeString& i_rLine)
{
  CATUnicodeString includeString = i_rLine;
  includeString.ReplaceAll("#include", "");
  includeString.ReplaceAll("\"", "");
  includeString.ReplaceAll(" ", "");
  includeString.ReplaceAll(".h", "");
  return includeString;
} // TCAVTACmtHasSubString

//-----------------------------------------------------------------------------
// TCAVTAWriteCmtFile
//-----------------------------------------------------------------------------
void TCAVTAWriteCmtFile(const CATUnicodeString& i_FileName,
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
} // TCAVTAWriteCmtFile


//-----------------------------------------------------------------------------
// TCAGenComment
//-----------------------------------------------------------------------------
void TCAGenComment(const CATUnicodeString& i_rMethod, const CATUnicodeString& i_rPref, CATListOfCATUnicodeString& o_rLines)
{
  CATUnicodeString method = i_rMethod;
  method.ReplaceAll("TCAVTA", "");
  method.ReplaceAll("TCAPWP", "");
  CATUnicodeString funcTxt;
  for (int c = 0; c < method.GetLengthInChar(); c++)
  {
    CATUnicodeChar uChar = method[c];
    CATUnicodeChar uCharP;
    CATUnicodeChar uCharN;
    if (uChar.IsUpper())
    {
      funcTxt.Append(" ");
    }
    funcTxt.Append(uChar);
  }

  funcTxt.ReplaceAll("P W P", "VTA");
  funcTxt.ReplaceAll("V T A", "VTA");
  funcTxt.ReplaceAll("V T I", "VTI");
  funcTxt.ReplaceAll("X M L", "XML");
  funcTxt.ReplaceAll("Z S B", "ZSB");
  funcTxt.ReplaceAll("C A T ", "CAT");
  funcTxt.ReplaceAll("N L S", "NLS");
  funcTxt.ReplaceAll("T C A", "");

  o_rLines.Append(i_rPref + "/**");
  o_rLines.Append(i_rPref + "* " + funcTxt);
  o_rLines.Append(i_rPref + "*/");

  for (int n = 1; n <= o_rLines.Size(); n++)
  {
    cerr << o_rLines[n] << endl;
  }
  return;
} // TCAGenComment

//-----------------------------------------------------------------------------
// TCAVTAWriteCmtFile
//-----------------------------------------------------------------------------
void TCAVTAWriteCmtFile(const CATUnicodeString& i_FileName,
                        const CATListOfCATUnicodeString i_rFileLines)
{

  cerr << endl << "*** i_FileName *** " << endl;
  ofstream outputFileStream;
  outputFileStream.open(i_FileName.ConvertToChar(), ios::out);
  if (outputFileStream)
  {
    for (int l = 1; l <= i_rFileLines.Size(); l++)
    {
      outputFileStream << i_rFileLines[l].ConvertToChar() << endl;
    }
    outputFileStream.close();
  }
  cerr << "*** END *** " << endl << endl;
  return;
} // TCAVTAWriteCmtFile

//-----------------------------------------------------------------------------
// TCAGetFuncIndex
//-----------------------------------------------------------------------------
int TCAGetFuncIndex(const CATUnicodeString& i_rLine)
{
  CATUnicodeString strippedLine = i_rLine.Strip(CATUnicodeString::CATStripModeAll);
  if (strippedLine == "")
  {
    return -1;
  }

  if (!strippedLine.SearchSubString("#") || !strippedLine.SearchSubString("class") || !strippedLine.SearchSubString("//"))
  {
    return -1;
  }


  CATListOfCATUnicodeString listToFind;
  listToFind.Append("virtual");
  listToFind.Append("inline ");
  listToFind.Append("static ");
  listToFind.Append("ExportedByTCA");
  listToFind.Append("void ");
  listToFind.Append("HRESULT ");
  listToFind.Append("CAT");
  listToFind.Append("int");
  listToFind.Append("double");
  listToFind.Append("friend");
  listToFind.Append("const");
  listToFind.Append("unsigned");

  int idx = -1;
  for (int i = 1; i <= listToFind.Size(); i++)
  {
    idx = i_rLine.SearchSubString(listToFind[i]);
    if (idx > -1)
    {
      CATUnicodeString pref = i_rLine.SubString(0, idx).Strip(CATUnicodeString::CATStripModeAll);
      pref.ReplaceAll("\t", "");
      // cerr << i_rLine << endl;
      // cerr << pref << endl;
      if (pref.GetLengthInChar() > 0)
      {
        idx = -1;
      }
    }
    if (idx > -1)
    {
      break;
    }
  }
  return idx;
} // TCAGetFuncIndex

//-----------------------------------------------------------------------------
// TCAVTAParceCommentFle
//-----------------------------------------------------------------------------
void TCAVTAParceCommentFle(const CATUnicodeString& i_rFileName,
                           CATListOfCATUnicodeString& io_rMethods)
{
  CATBoolean isModified = FALSE;
  CATListOfInt indexes;
  CATListOfCATUnicodeString includes;
  CATListOfCATUnicodeString fileLines;
  CATListOfCATUnicodeString newFileLines;
  TCADRTReadFile(i_rFileName, fileLines, FALSE, FALSE);
  CATUnicodeString compNameLine;
  for (int l = 1; l <= fileLines.Size(); l++)
  {
    CATListOfCATUnicodeString addLines;
    CATUnicodeString fLine = fileLines[l];
    CATBoolean commentExists = FALSE;
    CATBoolean toCheck = TRUE;
    if (l > 1 && (TCAVTACmtHasSubString(fileLines[l - 1], "*/") || TCAVTACmtHasSubString(fileLines[l - 1], "#define") || TCAVTACmtHasSubString(fileLines[l - 1], "//") || TCAVTACmtHasSubString(fileLines[l - 1], "\\")))
    {
      commentExists = TRUE;
    }
    if (TCAVTACmtHasSubString(fileLines[l], "#define") || TCAVTACmtHasSubString(fileLines[l], "//") || TCAVTACmtHasSubString(fileLines[l], "\\"))
    {
      commentExists = TRUE;
    }
    int idxV = -1;
    if (!commentExists)
    {
      idxV = TCAGetFuncIndex(fLine);
    }
    if (idxV > -1 && !commentExists)
    {
      CATUnicodeString pref = fLine.SubString(0, idxV);
      while (true)
      {
        int ln1 = fLine.GetLengthInChar();
        fLine.ReplaceAll(" (", "(");
        if (fLine.GetLengthInChar() == ln1)
        {
          break;
        }
      }
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
            TCAGenComment(fLine, pref, addLines);
            // io_rMethods.Append(fLine);
          }
        }
      }
    }
    if (addLines.Size() > 0)
    {
      isModified = TRUE;
      newFileLines.Append(addLines);
    }
    newFileLines.Append(fileLines[l]); 
  }

  if (isModified)
  {
    TCAVTAWriteCmtFile(i_rFileName, newFileLines);
  }

  return;
} // TCAVTAParceCommentFle 

//-----------------------------------------------------------------------------
// TCAVTAParceCmtFiles
//-----------------------------------------------------------------------------
void TCAVTAParceCmtFiles(const CATUnicodeString& i_rDir,
                         CATListOfCATUnicodeString& io_rMethods,
                         const CATBoolean& i_rInterfaces = FALSE)
{
  if (i_rDir != "")
  {
    CATListOfCATUnicodeString hdrFiles;
    CATListOfCATUnicodeString dirEntries;
    TCAVTAReadCmtDir(i_rDir, dirEntries, TRUE);

    cerr << "dirEntries: " << dirEntries.Size() << endl;
    for (int d = 1; d <= dirEntries.Size(); d++)
    {
      CATUnicodeString fileCmt = dirEntries[d];
      int length = fileCmt.GetLengthInChar();
      int idx = fileCmt.SearchSubString(".h");
      int diff = length - idx;
      if (diff == 2)
      {
        hdrFiles.Append(fileCmt);
      }
    }

    int hdrCount = hdrFiles.Size();
    cerr << "hdrCount: " << hdrCount << endl;
    for (int p = 1; p <= hdrCount; p++)
    {
      CATListOfCATUnicodeString methods;
      TCAVTAParceCommentFle(hdrFiles[p], methods);
      // io_rMethods.Append(methods);
    }
  }
  return;
} // TCAVTAParceCmtFiles 
//----------------------------------------------------------------------------------
// main
//----------------------------------------------------------------------------------
int main(int iArgc, char** iArgv)
{
  cerr << "*** VTA Test Cmt Code START ***" << endl << endl;
  if (iArgc > 1)
  {
    CATListOfCATUnicodeString methods;
    TCAVTAParceCmtFiles(iArgv[1], methods, TRUE);
  }
  else
  {
    cerr << endl << "MISSING input directory" << endl;
  }
  cerr << endl << "*** VTA Test Cmt Code END   ***" << endl;
  return 0;
} // main