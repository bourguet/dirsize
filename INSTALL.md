# Installation

    mkdir build
    cd build
    cmake path/to/source -DCMAKE_BUILD_TYPE=Release
    cmake --build . --target all
    cmake --build . --target install

Install will install the program and the man page.  It is possible to
change the installation prefix from its original /opt/bourguet to whatever
is desired by adding `-DCMAKE_INSTAL_PREFIX=/usr/local` to the first
invocation of cmake.

# Additional targets

It is possible to have a postscript or pdf version of the man page if the
needed tools `groff` and `ps2pdf` are installed by building the targets ps
and pdf.
