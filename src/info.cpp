// info.cpp
//
// ----------------------------------------------------------------------------
//
// Copyright (C) 2012 -- 2021 Jean-Marc Bourguet
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

#include "info.hpp"

#include <errno.h>
#include <iostream>
#include <iomanip>
#include <string.h>
#include <sys/stat.h>
#include <sstream>

namespace
{
long long const blockSize = 512;

bool theSilent = false;
bool theLogicalSize = false;
bool theUseReadableNumbers = false;
}

// ----------------------------------------------------------------------------

void setSilent(bool s)
{
    theSilent = s;
} // setSilent

// ----------------------------------------------------------------------------

bool isSilent()
{
    return theSilent;
} // isSilent

// ----------------------------------------------------------------------------

size_t displaySize(size_t sz)
{
    if (useLogicalSize())
        return sz;
    else
        return sz*blockSize;
} // displaySize

// ----------------------------------------------------------------------------

size_t getSize(struct stat& buf)
{
    if (useLogicalSize())
        return size_t(buf.st_size);
    else
        return size_t(buf.st_blocks);
} // getSize

// ----------------------------------------------------------------------------

void setLogicalSize(bool v)
{
    theLogicalSize = v;
} // setLogicalSize

// ----------------------------------------------------------------------------

void setUseReadableNumbers(bool v)
{
    theUseReadableNumbers = v;
} // setUseReadableNumbers

// ----------------------------------------------------------------------------

bool useLogicalSize()
{
    return theLogicalSize;
} // useLogicalSize

// ----------------------------------------------------------------------------

std::string format(size_t sz)
{
    char const* suffix = "";
    if (theUseReadableNumbers) {
        static char const* suffixes[] = { "B", "KiB", "MiB", "GiB", "TiB", "PiB", "EiB" };
        size_t fact = 1;
        std::size_t suffixIndex = 0;
        while (sz >= fact*10240 && suffixIndex < sizeof(suffixes)/sizeof(*suffixes)) {
            fact *= 1024;
            ++suffixIndex;
        }
        sz = (sz + fact/2)/fact;
        suffix = suffixes[suffixIndex];
    }
    std::ostringstream os;
    os << sz << ' ' << suffix;
    return os.str();
}

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
