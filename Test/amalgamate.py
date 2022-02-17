import io
import os.path

import re
startFile="HFSQtLi.h"
excludeHeaders=["sqlite3.h"]
# opened is a list filled with already parsed headers
# exclude is a list with includes that should be skipped (mainly the ones amalgamated into HFSqtLi.h)
# outHeader and outBody are file-like objects to fill with header (includes) and body of the parsed files
# included is a set that will be filled with already inserted includes in header (to avoid duplication)
# first is True only for the first .H or .CPP file parsed. For the first fill a copy of copyright notice and #pragma once will be made
# directory is the input directory to search for files
def parseUniversal(fname, opened, exclude, outHeader, outBody, included=None, first=False, directory="."):
  if included is None:
    included=set()
  fullFName=os.path.join(directory,fname)
  if not os.path.exists(fullFName) or fname in exclude:
    return False
  elif fname in opened:
    return True
  print("Parse",fname)
  f=open(fullFName, "r")
  opened.append(fname)
  start=True
  hadInitialComment=False
  inInitialComment=False
  initialComment=""
  for line in f:
    if inInitialComment:
      initialComment+=line
      if "*/" in line:
        if first:
          outHeader.write(initialComment)
        inInitialComment=False
      continue
    elif not hadInitialComment:
      if not line:
        continue
      elif line.startswith("/*"):
        initialComment=line
        inInitialComment=True
        continue
      else:
        hadInitialComment=True
    if line.startswith("#pragma") and "once" in line:
      if first:
        outHeader.write(line)
    elif line.startswith("#include"):
      refinclude=re.search("\"([^\"]+)\"", line)
      used=False
      if refinclude is not None:
        if refinclude.group(1) in exclude:
          used=False
        else:
          used=parseUniversal(refinclude.group(1), opened, exclude, outHeader, outBody, included=included, directory=directory)
      if not used and not line.strip() in included:
        outHeader.write(line)
        included.add(line.strip())
    else:
      outBody.write(line)
  return True      
curName=os.path.split(__file__)[0]

def htocpp(fname):
  return os.path.splitext(fname)[0]+".cpp"
def amalgamate(firstHeader, inDirectory, outDirectory):
  outH=open(os.path.join(os.path.join(outDirectory,startFile)),"w")
  outCPP=open(os.path.join(outDirectory,htocpp(startFile)),"w")

  hheader=io.StringIO()
  hbody=io.StringIO()
  headers=[]
  parseUniversal(startFile, headers, set(), hheader, hbody, first=True, directory=inDirectory)
  outH.write(hheader.getvalue()+hbody.getvalue())

  cppheader=io.StringIO()
  cppbody=io.StringIO()
  h2=[]
  included=set()
  first=True
  for h in headers:
    cppname=htocpp(h)
    if os.path.exists(os.path.join(inDirectory, cppname)):
      parseUniversal(cppname, headers, excludeHeaders, cppheader, cppbody, first=first, included=included, directory=inDirectory)
      first=False
  outCPP.write(cppheader.getvalue())
  outCPP.write("#include \"%s\"\n"%(firstHeader))
  outCPP.write(cppbody.getvalue())
  print("")
  print("Done")
curDir=os.path.split(__file__)[0]
outDir=os.path.join(curDir, "../HFSQtLi/")
amalgamate(startFile, curDir, outDir)
