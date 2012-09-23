// DirInfo.hpp
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
//
// ----------------------------------------------------------------------------
//
// Class containing collected directory information
//
// ----------------------------------------------------------------------------

#ifndef DIR_INFO_HPP
#define DIR_INFO_HPP

#include <string>
#include <set>
#include <deque>
#include <ostream>

// ----------------------------------------------------------------------------
// DirInfo
// ----------------------------------------------------------------------------

class DirInfo
{
public:
    DirInfo(std::string const& pName, std::string const& pPath, DirInfo* parent);
    // ~DirInfo();

    std::string name() const;
    std::string path() const;
    DirInfo* parent() const;
    long long size() const;
    long long directSize() const;
    std::deque<DirInfo*> const& subDirs() const;
    std::deque<DirInfo*>& subDirs();

    void collect(long long minSize, std::deque<DirInfo*>& dirs) const;
    void showTree(std::ostream& os, long long minSize) const;
    static void addIgnoredDirectory(std::string const& name);
private: // and not implemented
    DirInfo(DirInfo const&);
    DirInfo& operator=(DirInfo const&);

private:
    static std::set<std::string> ourIgnoredDirectories;

    static bool ignored(std::string const& name, std::string const& path);

    DirInfo(long long pSize, DirInfo* parent);
    
    std::string myName;
    DirInfo* myParent;
    long long mySize;
    long long myDirectSize;
    std::deque<DirInfo*> mySubDirs;

    void showTree
         (std::ostream& os, long long minSize, int level, std::deque<bool> hasOtherDirs)
        const;
    
}; // DirInfo

// ----------------------------------------------------------------------------

#endif
