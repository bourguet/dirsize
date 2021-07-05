// dirsize
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
//
// ----------------------------------------------------------------------------
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

#include "info.hpp"
#include "DirInfo.hpp"

// ----------------------------------------------------------------------------

bool showHierInfo = false;
bool showFlatInfo = true;
bool useReadableNumbers = false;
size_t minimumSize = 0;
size_t minimumPercent = 0;
size_t minimumDepth = 0;

// ----------------------------------------------------------------------------

/// Exception class for evalString
class Not_A_Valid_Number: public std::domain_error
{
public:
    Not_A_Valid_Number(std::string const& n)
        : std::domain_error("\"" + n + "\n is not a valid number") {}
}; // Not_A_Valid_Number

// ----------------------------------------------------------------------------

/// Display simple usage information
void usage()
{
    std::cout << "Usage: dirsize [-hstblr] [-i dir] [-m minSize] [-p minPercent] [-d depth] dirs...\n";
} // usage

// ----------------------------------------------------------------------------

/// Display a full help message
void help()
{
    usage();
    std::cout <<
        "Show the size of directories trees\n"
        "\n"
        "-h          this help\n"
        "-i dir      ignore dir, may be specified several times\n"
        "-m minSize  show only directories whose size is above minSize\n"
        "-p percent  show only directories whose size if more than percent percent of total size\n"
        "-d depth    show at least all directories until depth\n"
        "-t          show a directory tree\n"
        "-b          show both a tree and a flat view\n"
        "-l          show logical size (instead of physical one)\n"
        "-r          show readable size (with SI units)\n"
        "-s          silent, don't show progress\n";
} // help

// ----------------------------------------------------------------------------

/// Evaluate a string
size_t evalString(std::string const& s, bool suffixes, bool binarySuffixes)
{
    char const* data = s.c_str();
    char* end;
    size_t result = strtoull(data, &end, 0);
    if (suffixes) {
        if (binarySuffixes || (*end != '\0' && end[1] == 'i')) {
            switch (*end) {
            case 'E': result *= 1024; /* fall through */
            case 'P': result *= 1024; /* fall through */
            case 'T': result *= 1024; /* fall through */
            case 'G': result *= 1024; /* fall through */
            case 'M': result *= 1024; /* fall through */
            case 'K': case 'k': result *= 1024;
                ++end;
                if (*end == 'i')
                    ++end;
                break;
            }
        } else {
            switch (*end) {
            case 'E': result *= 1000; /* fall through */
            case 'P': result *= 1000; /* fall through */
            case 'T': result *= 1000; /* fall through */
            case 'G': result *= 1000; /* fall through */
            case 'M': result *= 1000; /* fall through */
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
// FlatDirDisplayer
// ----------------------------------------------------------------------------

class FlatDirDisplayer: public std::iterator<std::output_iterator_tag, DirInfo*>
{
public:
    FlatDirDisplayer(std::ostream& os);

    FlatDirDisplayer& operator=(DirInfo* info);
    FlatDirDisplayer& operator++() { return *this; }
    FlatDirDisplayer& operator++(int) { return *this; }
    FlatDirDisplayer& operator*() { return *this; }
private:

    std::ostream* myOS;
}; // FlatDirDisplayer

// ----------------------------------------------------------------------------

FlatDirDisplayer::FlatDirDisplayer(std::ostream& os)
    : myOS(&os)
{
} // FlatDirDisplayer

// ----------------------------------------------------------------------------

FlatDirDisplayer& FlatDirDisplayer::operator=(DirInfo* info)
{
    *myOS << std::setw(15) << format(info->size()) << " " << info->path() << '\n';
    return *this;
} // operator=

// ----------------------------------------------------------------------------

bool isSmaller(DirInfo* l, DirInfo* r)
{
    return l->size() < r->size();
} // isSmaller

// ----------------------------------------------------------------------------

void handleDirectory(std::string const& dir)
{
    DirInfo topInfo(dir, dir, NULL);
    size_t minSize = minimumSize;
    if (!isSilent())
        std::cout << "Reading directory structure done\n";
    if (topInfo.size() * minimumPercent / 100 > minSize)
        minSize = topInfo.size() * minimumPercent / 100;
    if (showHierInfo) {
        topInfo.showTree(std::cout, minSize, minimumDepth);
    }
    if (showFlatInfo) {
        std::deque<DirInfo*> flatDirs;
        flatDirs.push_back(&topInfo);
        topInfo.collect(minSize, flatDirs, minimumDepth);
        std::sort(flatDirs.begin(), flatDirs.end(), isSmaller);
        std::copy(flatDirs.begin(), flatDirs.end(), FlatDirDisplayer(std::cout));
    }
} // handleDirectory

// ----------------------------------------------------------------------------

/// The main function
int main(int argc, char* argv[])
{
    int status = EXIT_SUCCESS;

    try {
        int c, errcnt = 0;
        std::locale::global(std::locale(""));
        std::cout.imbue(std::locale());

        while (c = getopt(argc, argv, "hstblri:m:p:d:"), c != -1) {
            switch (c) {
            case 'h':
                help();
                throw 0;
            case 's':
                setSilent(true);
                break;
            case 't':
                showHierInfo = true;
                showFlatInfo = false;
                break;
            case 'b':
                showFlatInfo = true;
                showHierInfo = true;
                break;
            case 'r':
                setUseReadableNumbers(true);
                break;
            case 'i':
                DirInfo::addIgnoredDirectory(optarg);
                break;
            case 'm':
                minimumSize = evalString(optarg, true /* accept suffixes */, true /* binary suffixes */);
                break;
            case 'p':
                minimumPercent = evalString(optarg, false, false);
                if (minimumPercent > 100) {
                    std::cerr << "Minimum percentage should be below 100\n";
                    errcnt++;
                }
                break;
            case 'd':
                minimumDepth = evalString(optarg, false, false);
                break;
            case 'l':
                setLogicalSize(true);
                break;
            case '?':
                errcnt++;
                break;
            default:
                std::cerr << "Unexpected result from getopts: " << c << '\n';
                errcnt++;
            }
        }

        if (errcnt > 0) {
            usage();
            throw EXIT_FAILURE;
        }

        if (optind < argc) {
            for ( ; optind < argc; ++optind) {
                handleDirectory(argv[optind]);
            }
        } else {
            handleDirectory(".");
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
