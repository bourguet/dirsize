// -*- C++ -*-
//
// dirsize
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
// Show the directory usage
//
// ----------------------------------------------------------------------------

#include <algorithm>
#include <dirent.h>
#include <errno.h>
#include <errno.h>
#include <exception>
#include <iostream>
#include <set>
#include <stdexcept>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <iomanip>
#include <iterator>

// stat returns info in number of blocks of 512 bytes
long long const blockSize = 512;

// ----------------------------------------------------------------------------

/// Exception class for evalString
class Not_A_Valid_Number: public std::domain_error
{
public:
    Not_A_Valid_Number(std::string const& n)
        : std::domain_error("\"" + n + "\n is not a valid number") {}
}; // Not_A_Valid_Number


// ----------------------------------------------------------------------------
// Info
// ----------------------------------------------------------------------------

class Info
{
public:
    Info(std::string const& pName, long long pSize)
        : name(pName), size(pSize) {}
    // Info(Info const&);
    // Info& operator=(Info const&);
    // ~Info();

    std::string name;
    long long   size;
}; // Info

// ----------------------------------------------------------------------------

std::ostream& operator<<(std::ostream& os, Info const& i)
{
    os << std::setw(15) << i.size*blockSize << " " << i.name;
} // operator<<

// ----------------------------------------------------------------------------
// IsSmallerThan
// ----------------------------------------------------------------------------

class IsSmallerThan
{
public:
    IsSmallerThan(long long limit) : myLimit(limit) {}
    // IsSmallerThan(IsSmallerThan const&);
    // IsSmallerThan& operator=(IsSmallerThan const&);
    // ~IsSmallerThan();

    bool operator()(Info const& i) const;
private:
    long long myLimit;
}; // IsSmallerThan

// ----------------------------------------------------------------------------

bool IsSmallerThan::operator()(Info const& i) const
{
    return i.size*blockSize < myLimit;
} // IsSmallerThan::operator()

// ----------------------------------------------------------------------------
// HierInfo
// ----------------------------------------------------------------------------

class HierInfo
{
public:
    // HierInfo();
    // HierInfo(HierInfo const&);
    // HierInfo& operator=(HierInfo const&);
    // ~HierInfo();

    std::string name;
    long long   size;
    std::vector<HierInfo> subInfo;

    void show(int indent, std::vector<bool> need);
}; // HierInfo


// ----------------------------------------------------------------------------
// IsHierSizeSmallerThan
// ----------------------------------------------------------------------------

class IsHierSizeSmallerThan
{
public:
    IsHierSizeSmallerThan(long long limit) : myLimit(limit) {}
    // IsHierSizeSmallerThan(IsHierSizeSmallerThan const&);
    // IsHierSizeSmallerThan& operator=(IsHierSizeSmallerThan const&);
    // ~IsHierSizeSmallerThan();
    bool operator()(HierInfo const& i) const;
private:
    long long myLimit;
}; // IsHierSizeSmallerThan

// ----------------------------------------------------------------------------

bool IsHierSizeSmallerThan::operator()(HierInfo const& i) const
{
    return i.size*blockSize < myLimit;
} // IsSmallerThan::operator()

// ----------------------------------------------------------------------------

bool isHierBigger(HierInfo const& left, HierInfo const& right)
{
    return left.size > right.size;
} // isSmaller

// ----------------------------------------------------------------------------

std::set<std::string> theIgnoredDirectories;
bool theSilent = false;
long long theMinimunSize = 0;
std::vector<Info> theFlatInfo;

// ----------------------------------------------------------------------------

void HierInfo::show(int indent, std::vector<bool> need = std::vector<bool>())
{
    std::cout << std::setw(15) << size*blockSize << " ";
    for (int i = 0; i+1 < indent; ++i)
        if (need[i])
            std::cout << "| ";
        else
            std::cout << "  ";
    if (indent > 0)
        std::cout << "+ ";
    std::cout << name << '\n';
    subInfo.erase(std::remove_if(subInfo.begin(), subInfo.end(),
                                 IsHierSizeSmallerThan(theMinimunSize)),
                  subInfo.end());
    std::sort(subInfo.begin(), subInfo.end(), isHierBigger);
    need.push_back(true);
    for (std::vector<HierInfo>::iterator i = subInfo.begin();
         i != subInfo.end(); ++i)
    {
        need.back() = (i+1) != subInfo.end();
        i->show(indent+1, need);
    }
} // show

// ----------------------------------------------------------------------------

/// Display simple usage information
void usage()
{
    std::cout << "Usage: dirsize [-hs] [-i dir] [-m minSize]\n";
} // usage

// ----------------------------------------------------------------------------

/// Display a full help message
void help()
{
    usage();
    std::cout <<
        "Show the size of a tree of directories\n"
        "\n"
        "-h          this help\n"
        "-i dir      ignore dir, may be specified several times\n"
        "-m minSize  show only directories whose size is above minSize\n"
        "-s          silent, don't show progress\n";
} // help

// ----------------------------------------------------------------------------

/// Evaluate a string
long long evalString(std::string const& s, bool suffixes, bool binarySuffixes)
{
    char const* data = s.c_str();
    char* end;
    long result = strtoll(data, &end, 0);
    if (suffixes) {
        if (binarySuffixes || (*end != '\0' && end[1] == 'i')) {
            switch (*end) {
            case 'E': result *= 1024;
            case 'P': result *= 1024;
            case 'T': result *= 1024;
            case 'G': result *= 1024;
            case 'M': result *= 1024;
            case 'K': case 'k': result *= 1024;
                ++end;
                if (*end == 'i')
                    ++end;
                break;
            }
        } else {
            switch (*end) {
            case 'E': result *= 1000;
            case 'P': result *= 1000;
            case 'T': result *= 1000;
            case 'G': result *= 1000;
            case 'M': result *= 1000;
            case 'K': case 'k': result *= 1000;
                ++end;
                break;
            }
        }
    }
    while (isspace(*end))
        ++end;
    if (*end != '\0')
        throw Not_A_Valid_Number(s);
    return result;
} // evalString

// ----------------------------------------------------------------------------

void message(std::string const& msg)
{
    if (theSilent)
        return;
    char const* clearToEol = "\033[K";
    std::cout << msg << clearToEol << '\r' << std::flush;
} // message

// ----------------------------------------------------------------------------

void error(std::string const& msg)
{
    std::string info(strerror(errno));
    std::cerr << '\n' << msg << ": " << info << '\n' << std::flush;
} // error

// ----------------------------------------------------------------------------

HierInfo readDirectoryStructure(std::string const& dir)
{
    HierInfo result;
    long long dirSize = 0;
    long long subDirSize = 0;
    message("Reading " + dir);
    DIR* dirIter = opendir(dir.c_str());
    if (dirIter == NULL) {
        error("Unable to open " + dir);
    } else {
        dirent* entry;
        for (errno = 0, entry = readdir(dirIter);
             entry != NULL;
             errno = 0, entry = readdir(dirIter))
        {
            std::string name(entry->d_name);
            std::string path(dir + '/' + name);
            if (name != "." && name != "..")
            {
                struct stat info;
                if (lstat(path.c_str(), &info) != 0) {
                    message("Error while getting information about " + path);
                } else {
                    dirSize += info.st_blocks;
                    if (S_ISDIR(info.st_mode)
                        && theIgnoredDirectories.find(path) == theIgnoredDirectories.end())
                    {
                        HierInfo subInfo = readDirectoryStructure(path);
                        subInfo.name = name;
                        subDirSize += subInfo.size;
                        result.subInfo.push_back(subInfo);
                    } else {
                        HierInfo subInfo;
                        subInfo.name = name;
                        subInfo.size = info.st_blocks;
                        result.subInfo.push_back(subInfo);
                    }
                }
            }
        }
        if (errno != 0) {
            error("Error while reading " + dir);
        }
        closedir(dirIter);
    }
    theFlatInfo.push_back(Info(dir, dirSize+subDirSize));
    if (subDirSize != 0) {
        theFlatInfo.push_back(Info(dir + " /files/", dirSize));
        HierInfo files;
        files.name = "/files/";
        files.size = dirSize;
        result.subInfo.push_back(files);
    }
    result.size = dirSize + subDirSize;    
    return result;
} // readDirectoryStructure

// ----------------------------------------------------------------------------

bool isSmaller(Info const& left, Info const& right)
{
    return left.size < right.size;
} // isSmaller

// ----------------------------------------------------------------------------

/// The main function
int main(int argc, char* argv[])
{
    int status = EXIT_SUCCESS;
    
    try {
        int c, errcnt = 0;
        bool showHierInfo = false;
        bool showFlatInfo = true;
        
        std::locale::global(std::locale(""));
        
        while (c = getopt(argc, argv, "hsi:m:tb"), c != -1) {
            switch (c) {
            case 'h':
                help();
                throw 0;
            case 's':
                theSilent = true;
                break;
            case 't':
                showHierInfo = true;
                showFlatInfo = false;
                break;
            case 'b':
                showFlatInfo = true;
                showHierInfo = true;
                break;
            case 'i':
                theIgnoredDirectories.insert(optarg);
                break;
            case 'm':
                theMinimunSize = evalString(optarg, true /* accept suffixes */, true /* binary suffixes */);
                break;
            case '?':
                errcnt++;
                break;
            default:
                std::cerr << "Unexpected result from getopts: " << c << '\n';
                errcnt++;
            }
        }
        
        if (optind < argc) {
            errcnt++;
        }

        if (errcnt > 0) {
            usage();
            throw EXIT_FAILURE;
        }

        HierInfo info = readDirectoryStructure(".");
        if (!theSilent)
            std::cout << "Reading directory structure done\n";
        info.name = ".";
        if (showHierInfo)
            info.show(0);
        if (showFlatInfo) {
            theFlatInfo.erase(std::remove_if(theFlatInfo.begin(), theFlatInfo.end(), IsSmallerThan(theMinimunSize)), theFlatInfo.end());
            std::sort(theFlatInfo.begin(), theFlatInfo.end(), isSmaller);
            std::copy(theFlatInfo.begin(), theFlatInfo.end(), std::ostream_iterator<Info>(std::cout, "\n"));
        }
    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        status = EXIT_FAILURE;
    } catch (int s) {
        status = s;
    } catch (...) {
        std::cerr << "Unexpected exception.\n";
        status = EXIT_FAILURE;
    }

    return status;
} // main

// ----------------------------------------------------------------------------
