// DirInfo.cpp
//
// ----------------------------------------------------------------------------
// 
// Copyright (C) 2012  Jean-Marc Bourguet
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

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <fnmatch.h>
#include <iomanip>
#include <algorithm>

#include "info.hpp"

namespace
{
long long const blockSize = 512;

struct IsSmallerThan
{
    IsSmallerThan(long long limit) : myLimit(limit) {}
    bool operator()(DirInfo* dir)
    {
        return dir->size() < myLimit;
    }
private:
    long long myLimit;
};

bool isBigger(DirInfo* l, DirInfo* r)
{
    return l->size() > r->size();
}

}

// ----------------------------------------------------------------------------

std::set<std::string> DirInfo::ourIgnoredDirectories;

// ----------------------------------------------------------------------------

DirInfo::DirInfo(long long size, DirInfo* parent)
    : myName("(direct files)"),
      myParent(parent),
      mySize(size),
      myDirectSize(size)
{    
} // DirInfo

// ----------------------------------------------------------------------------

DirInfo::DirInfo(std::string const& pName, std::string const& pPath, DirInfo* parent)
    : myName(pName),
      myParent(parent),
      mySize(0),
      myDirectSize(0)
{
    message("Reading " + pPath);
    {
        struct stat info;
        if (lstat(pPath.c_str(), &info) != 0) {
            error("Error while getting information about " + pPath);
        } else {
            myDirectSize += info.st_blocks;
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
                        myDirectSize += info.st_blocks;
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
        mySubDirs.push_back(new DirInfo(myDirectSize, this));
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

long long DirInfo::size() const
{
    return mySize * blockSize;
} // size

// ----------------------------------------------------------------------------

long long DirInfo::directSize() const
{
    return myDirectSize * blockSize;
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

void DirInfo::collect(long long minSize, std::deque<DirInfo*>& dirs) const
{
    for (std::deque<DirInfo*>::const_iterator i = mySubDirs.begin(), e = mySubDirs.end();
         i != e; ++i)
    {
        if ((*i)->size() >= minSize) {
            dirs.push_back(*i);
            (*i)->collect(minSize, dirs);
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

void DirInfo::showTree(std::ostream& os, long long minSize) const
{
    std::deque<bool> hasOtherDirs;
    showTree(os, minSize, 0, hasOtherDirs);
}

// ----------------------------------------------------------------------------

void DirInfo::showTree
     (std::ostream& os, long long minSize, int level, std::deque<bool> hasOtherDirs)
    const
{
    os << std::setw(15) << size() << " ";
    for (int i = 0; i+1 < level; ++i)
    {
        os << (hasOtherDirs[i] ? "| " : "  ");
    }
    if (level > 0)
        os << "+ ";
    os << name() << '\n';
    std::deque<DirInfo*> selectedSubDir;
    std::remove_copy_if(mySubDirs.begin(), mySubDirs.end(),
                        std::back_inserter(selectedSubDir),
                        IsSmallerThan(minSize));
    std::sort(selectedSubDir.begin(), selectedSubDir.end(), isBigger);
    hasOtherDirs.push_back(selectedSubDir.begin() != selectedSubDir.end());
    for (std::deque<DirInfo*>::iterator i = selectedSubDir.begin(),
             e = selectedSubDir.end();
         i != e; ++i)
    {
        hasOtherDirs.back() = (i+1) != e;
        (*i)->showTree(os, minSize, level+1, hasOtherDirs);
    }
}

// ----------------------------------------------------------------------------

void DirInfo::addIgnoredDirectory(std::string const& name)
{
    ourIgnoredDirectories.insert(name);
} // addIgnoredDirectory