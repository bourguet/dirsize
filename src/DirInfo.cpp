// DirInfo.cpp
//
// ----------------------------------------------------------------------------
//
// Copyright (C) 2012 -- 2021  Jean-Marc Bourguet
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//   * Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//
//   * Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//
//   * Neither the name of Jean-Marc Bourguet nor the names of other
//     contributors may be used to endorse or promote products derived from
//     this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
// IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
// PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
// TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// ----------------------------------------------------------------------------

#include "DirInfo.hpp"

#include <algorithm>
#include <dirent.h>
#include <errno.h>
#include <fnmatch.h>
#include <iomanip>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "info.hpp"

namespace
{

struct IsSmallerThan
{
    IsSmallerThan(size_t limit) : myLimit(limit) {}
    bool operator()(DirInfo* dir)
    {
        return dir->size() < myLimit;
    }
private:
    size_t myLimit;
};

bool isBigger(DirInfo* l, DirInfo* r)
{
    return l->size() > r->size();
}

}

// ----------------------------------------------------------------------------

std::set<std::string> DirInfo::ourIgnoredDirectories;

// ----------------------------------------------------------------------------

DirInfo::DirInfo(size_t size, size_t max, std::string const& name, DirInfo* parent)
    : myName("(directory)"),
      myParent(parent),
      mySize(size),
      myDirectSize(size)
{
    if (!name.empty()) {
        std::ostringstream os;
        os << "(directory content, max: " << displaySize(max)
           << " for " << name << ")";
        myName = os.str();
    }
} // DirInfo

// ----------------------------------------------------------------------------

DirInfo::DirInfo(std::string const& pName, std::string const& pPath, DirInfo* parent)
    : myName(pName),
      myParent(parent),
      mySize(0),
      myDirectSize(0)
{
    size_t maxDirectEntry = 0;
    std::string maxDirectEntryName;

    message("Reading " + pPath);
    {
        struct stat info;
        if (lstat(pPath.c_str(), &info) != 0) {
            error("Error while getting information about " + pPath);
        } else {
            myDirectSize += getSize(info);
            maxDirectEntry = myDirectSize;
            maxDirectEntryName = "";
        }
    }

    DIR* dirIter = opendir(pPath.c_str());
    if (dirIter == NULL) {
        error("Unable to open " + pPath);
    } else {
        dirent* entry;
        for (errno = 0, entry = readdir(dirIter);
             entry != NULL;
             errno = 0, entry = readdir(dirIter))
        {
            std::string const eName(entry->d_name);
            if (eName != "." && eName != "..")
            {
                std::string const ePath(pPath + '/' + eName);
                struct stat info;
                if (lstat(ePath.c_str(), &info) != 0) {
                    error("Error while getting information about " + ePath);
                } else {
                    if (S_ISDIR(info.st_mode) && !ignored(eName, ePath))
                    {
                        DirInfo* subInfo = new DirInfo(eName, ePath, this);
                        message("Reading " + pPath);
                        mySize += subInfo->mySize;
                        mySubDirs.push_back(subInfo);
                    } else {
                        myDirectSize += getSize(info);
                        if (maxDirectEntryName.empty() || getSize(info) > maxDirectEntry) {
                            maxDirectEntry = getSize(info);
                            maxDirectEntryName = eName;
                        }
                    }
                }
            }
        }
        if (errno != 0) {
            error("Error while reading " + pPath);
        }
        closedir(dirIter);
    }
    if (mySize != 0) {
        mySubDirs.push_back
            (new DirInfo(myDirectSize, maxDirectEntry, maxDirectEntryName, this));
    } else if (!maxDirectEntryName.empty()) {
        std::ostringstream os;
        os << myName << " (max: " << displaySize(maxDirectEntry)
           << " for " << maxDirectEntryName << ")";
        myName = os.str();
    }
    mySize += myDirectSize;
} // DirInfo

// ----------------------------------------------------------------------------

DirInfo* DirInfo::parent() const
{
    return myParent;
} // parent

// ----------------------------------------------------------------------------

std::string DirInfo::name() const
{
    return myName;
} // name

// ----------------------------------------------------------------------------

std::string DirInfo::path() const
{
    return myParent != NULL ? myParent->path() + '/' + myName : myName;
} // path

// ----------------------------------------------------------------------------

size_t DirInfo::size() const
{
    return displaySize(mySize);
} // size

// ----------------------------------------------------------------------------

size_t DirInfo::directSize() const
{
    return displaySize(myDirectSize);
} // directSize

// ----------------------------------------------------------------------------

std::deque<DirInfo*> const& DirInfo::subDirs() const
{
    return mySubDirs;
} // subDirs

// ----------------------------------------------------------------------------

std::deque<DirInfo*>& DirInfo::subDirs()
{
    return mySubDirs;
} // subDirs

// ----------------------------------------------------------------------------

void DirInfo::collect(size_t minSize, std::deque<DirInfo*>& dirs, size_t minDepth) const
{
    for (std::deque<DirInfo*>::const_iterator i = mySubDirs.begin(), e = mySubDirs.end();
         i != e; ++i)
    {
        if ((*i)->size() >= minSize || minDepth > 0) {
            dirs.push_back(*i);
            (*i)->collect(minSize, dirs, minDepth-1);
        }
    }
} // collect

// ----------------------------------------------------------------------------

bool DirInfo::ignored(std::string const& name, std::string const& path)
{
    if (ourIgnoredDirectories.find(name) != ourIgnoredDirectories.end()
        || ourIgnoredDirectories.find(path) != ourIgnoredDirectories.end())
    {
        return true;
    }
    for (std::set<std::string>::iterator i = ourIgnoredDirectories.begin(),
             e = ourIgnoredDirectories.end();
         i != e; ++i)
    {
        if (fnmatch((*i).c_str(), name.c_str(), FNM_PATHNAME) == 0
            || fnmatch((*i).c_str(), path.c_str(), FNM_PATHNAME) == 0)
        {
            return true;
        }
    }
    return false;
} // ignored

// ----------------------------------------------------------------------------

void DirInfo::showTree(std::ostream& os, size_t minSize, size_t minDepth) const
{
    std::deque<bool> hasOtherDirs;
    showTree(os, minSize, 0, minDepth, hasOtherDirs);
}

// ----------------------------------------------------------------------------

void DirInfo::showTree
     (std::ostream& os, size_t minSize, size_t level, size_t minDepth,
      std::deque<bool> hasOtherDirs)
    const
{
    os << std::setw(15) << format(size()) << " ";
    for (std::size_t i = 0; i+1 < level; ++i)
    {
        os << (hasOtherDirs[i] ? "| " : "  ");
    }
    if (level > 0)
        os << "+ ";
    os << name() << '\n';
    std::deque<DirInfo*> selectedSubDir;
    if (minDepth <= level) {
        std::remove_copy_if(mySubDirs.begin(), mySubDirs.end(),
                            std::back_inserter(selectedSubDir),
                            IsSmallerThan(minSize));
    } else {
        std::copy(mySubDirs.begin(), mySubDirs.end(),
                  std::back_inserter(selectedSubDir));
    }
    std::sort(selectedSubDir.begin(), selectedSubDir.end(), isBigger);
    hasOtherDirs.push_back(selectedSubDir.begin() != selectedSubDir.end());
    for (std::deque<DirInfo*>::iterator i = selectedSubDir.begin(),
             e = selectedSubDir.end();
         i != e; ++i)
    {
        hasOtherDirs.back() = (i+1) != e;
        (*i)->showTree(os, minSize, level+1, minDepth, hasOtherDirs);
    }
}

// ----------------------------------------------------------------------------

void DirInfo::addIgnoredDirectory(std::string const& name)
{
    ourIgnoredDirectories.insert(name);
} // addIgnoredDirectory
