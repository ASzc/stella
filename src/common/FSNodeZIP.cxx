//============================================================================
//
//   SSSS    tt          lll  lll
//  SS  SS   tt           ll   ll
//  SS     tttttt  eeee   ll   ll   aaaa
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2019 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#if defined(ZIP_SUPPORT)

#include <set>

#include "bspf.hxx"
#include "Bankswitch.hxx"
#include "OSystem.hxx"
#include "FSNodeFactory.hxx"
#include "FSNodeZIP.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FilesystemNodeZIP::FilesystemNodeZIP()
  : _error(zip_error::NOT_A_FILE),
    _numFiles(0),
    _isDirectory(false),
    _isFile(false)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FilesystemNodeZIP::FilesystemNodeZIP(const string& p)
  : _error(zip_error::NONE),
    _numFiles(0),
    _isDirectory(false),
    _isFile(false)
{
  // Extract ZIP file and virtual file (if specified)
  size_t pos = BSPF::findIgnoreCase(p, ".zip");
  if(pos == string::npos)
    return;

  _zipFile = p.substr(0, pos+4);

  // Open file at least once to initialize the virtual file count
  try
  {
    myZipHandler->open(_zipFile);
  }
  catch(const runtime_error&)
  {
    // TODO: Actually present the error passed in back to the user
    //       For now, we just indicate that no ROMs were found
    _error = zip_error::NO_ROMS;
    return;
  }
  _numFiles = myZipHandler->romFiles();
  if(_numFiles == 0)
  {
    _error = zip_error::NO_ROMS;
    return;
  }

  // We always need a virtual file/path
  // Either one is given, or we use the first one
  if(pos+5 < p.length())
  {
    _virtualPath = p.substr(pos+5);
    _isFile = Bankswitch::isValidRomName(_virtualPath);
    _isDirectory = !_isFile;
  }
  else if(_numFiles == 1)
  {
    bool found = false;
    while(myZipHandler->hasNext() && !found)
    {
      const string& file = myZipHandler->next();
      if(Bankswitch::isValidRomName(file))
      {
        _virtualPath = file;
        _isFile = true;

        found = true;
      }
    }
    if(!found)
      return;
  }
  else
    _isDirectory = true;

  // Create a concrete FSNode to use
  // This *must not* be a ZIP file; it must be a real FSNode object that
  // has direct access to the actual filesystem (aka, a 'System' node)
  // Behind the scenes, this node is actually a platform-specific object
  // for whatever system we are running on
  _realNode = FilesystemNodeFactory::create(_zipFile, FilesystemNodeFactory::Type::SYSTEM);

  setFlags(_zipFile, _virtualPath, _realNode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FilesystemNodeZIP::FilesystemNodeZIP(
    const string& zipfile, const string& virtualpath,
    AbstractFSNodePtr realnode, bool isdir)
  : _error(zip_error::NONE),
    _numFiles(0),
    _isDirectory(isdir),
    _isFile(!isdir)
{
  setFlags(zipfile, virtualpath, realnode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FilesystemNodeZIP::setFlags(const string& zipfile,
                                 const string& virtualpath,
                                 AbstractFSNodePtr realnode)
{
  _zipFile = zipfile;
  _virtualPath = virtualpath;
  _realNode = realnode;

  _path = _realNode->getPath();
  _shortPath = _realNode->getShortPath();

  // Is a file component present?
  if(_virtualPath.size() != 0)
  {
    _path += ("/" + _virtualPath);
    _shortPath += ("/" + _virtualPath);
  }
  _name = lastPathComponent(_path);

  _error = zip_error::NONE;
  if(!_realNode->isFile())
    _error = zip_error::NOT_A_FILE;
  if(!_realNode->isReadable())
    _error = zip_error::NOT_READABLE;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FilesystemNodeZIP::getChildren(AbstractFSList& myList, ListMode mode) const
{
  // Files within ZIP archives don't contain children
  if(!isDirectory() || _error != zip_error::NONE)
    return false;

  std::set<string> dirs;
  myZipHandler->open(_zipFile);
  while(myZipHandler->hasNext())
  {
    // Only consider entries that start with '_virtualPath'
    // Ignore empty filenames and '__MACOSX' virtual directories
    const string& next = myZipHandler->next();
    if(BSPF::startsWithIgnoreCase(next, "__MACOSX") || next == EmptyString)
      continue;
    if(BSPF::startsWithIgnoreCase(next, _virtualPath))
    {
      // First strip off the leading directory
      const string& curr = next.substr(_virtualPath == "" ? 0 : _virtualPath.size()+1);
      // Only add sub-directory entries once
      auto pos = curr.find_first_of("/\\");
      if(pos != string::npos)
        dirs.emplace(curr.substr(0, pos));
      else
        myList.emplace_back(new FilesystemNodeZIP(_zipFile, next, _realNode, false));
    }
  }
  for(const auto& dir: dirs)
  {
    // Prepend previous path
    const string& vpath = _virtualPath != "" ? _virtualPath + "/" + dir : dir;
    myList.emplace_back(new FilesystemNodeZIP(_zipFile, vpath, _realNode, true));
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 FilesystemNodeZIP::read(ByteBuffer& image) const
{
  switch(_error)
  {
    case zip_error::NONE:         break;
    case zip_error::NOT_A_FILE:   throw runtime_error("ZIP file contains errors/not found");
    case zip_error::NOT_READABLE: throw runtime_error("ZIP file not readable");
    case zip_error::NO_ROMS:      throw runtime_error("ZIP file doesn't contain any ROMs");
  }

  myZipHandler->open(_zipFile);

  bool found = false;
  while(myZipHandler->hasNext() && !found)
    found = myZipHandler->next() == _virtualPath;

  return found ? uInt32(myZipHandler->decompress(image)) : 0; // TODO: 64bit
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AbstractFSNodePtr FilesystemNodeZIP::getParent() const
{
  if(_virtualPath == "")
    return _realNode ? _realNode->getParent() : nullptr;

  const char* start = _path.c_str();
  const char* end = lastPathComponent(_path);

  return make_shared<FilesystemNodeZIP>(string(start, end - start - 1));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
unique_ptr<ZipHandler> FilesystemNodeZIP::myZipHandler = make_unique<ZipHandler>();

#endif  // ZIP_SUPPORT
