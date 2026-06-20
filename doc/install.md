---
layout: page
title: Installation
---

# Installation: Tools and C++ Runtime

<div style="float: right"><a class="groups_link" style="color: #fff"
href="https://groups.google.com/group/zap-announce">Get Notified of Updates</a></div>

The Zap tools, including the compiler (which takes `.zap` files and generates source code
for them), are written in C++.  Therefore, you must install the C++ package even if your actual
development language is something else.

This package is licensed under the [MIT License](http://opensource.org/licenses/MIT).

## Prerequisites

### Supported Compilers

Zap makes extensive use of C++20 language features. As a result, it requires a relatively
new version of a well-supported compiler. The minimum versions are:

* GCC 10.0*
* Clang 14.0
* Visual C++ 2022

*: Zap 2.0 and above cannot currently compile with GCC due to https://gcc.gnu.org/bugzilla/show_bug.cgi?id=102051

If your system's default compiler is older that the above, you will need to install a newer
compiler and set the `CXX` environment variable before trying to build Zap. For example,
after installing GCC 10, you could set `CXX=g++-10` to use this compiler.

### Supported Operating Systems

In theory, Zap should work on any POSIX platform supporting one of the above compilers,
as well as on Windows. We test every Zap release on the following platforms:

* Android
* Linux
* Mac OS X
* Windows - MinGW-w64
* Windows - Visual C++

**Windows users:** Zap requires Visual Studio 2022 or newer. All features
of Zap -- including serialization, dynamic API, RPC, and schema parser -- are now supported.

**Mac OS X users:** You should use the latest Xcode with the Xcode command-line
tools (Xcode menu > Preferences > Downloads).  Alternatively, the command-line tools
package from [Apple](https://developer.apple.com/downloads/) or compiler builds from
[Macports](http://www.macports.org/), [Fink](http://www.finkproject.org/), or
[Homebrew](http://brew.sh/) are reported to work.

## Installation: Unix

**From Release Tarball**

You may download and install the release version of Zap like so:

<pre><code>curl -O <a href="https://zap.org/zap-c++-0.0.0.tar.gz">https://zap.org/zap-c++-0.0.0.tar.gz</a>
tar zxf zap-c++-0.0.0.tar.gz
cd zap-c++-0.0.0
./configure
make -j6 check
sudo make install</code></pre>

This will install `zap`, the Zap command-line tool.  It will also install `libzap`,
`libzapc`, and `libkj` in `/usr/local/lib` and headers in `/usr/local/include/zap` and
`/usr/local/include/kj`.

**From Package Managers**

Some package managers include Zap packages.

Note: These packages are not maintained by us and are sometimes not up to date with the latest Zap release.

* Debian / Ubuntu: `apt-get install zap`
* Arch Linux: `sudo pacman -S zap`
* Homebrew (OSX): `brew install zap`

**From Git**

If you download directly from Git, you will need to have the GNU autotools --
[autoconf](http://www.gnu.org/software/autoconf/),
[automake](http://www.gnu.org/software/automake/), and
[libtool](http://www.gnu.org/software/libtool/) -- installed.

    git clone -b master https://github.com/zap/zap.git
    cd zap/c++
    autoreconf -i
    ./configure
    make -j6 check
    sudo make install

## Installation: Windows

**From Release Zip**

1. Download Zap Win32 build:

   <pre><a href="https://zap.org/zap-c++-win32-0.0.0.zip">https://zap.org/zap-c++-win32-0.0.0.zip</a></pre>

2. Find `zap.exe`, `zapc-c++.exe`, and `zapc-zap.exe` under `zap-tools-win32-0.0.0` in
   the zip and copy them somewhere.

3. If your `.zap` files will import any of the `.zap` files provided by the core project, or
   if you use the `stream` keyword (which implicitly imports `zap/stream.zap`), then you need
   to put those files somewhere where the zap compiler can find them. To do this, copy the
   directory `zap-c++-0.0.0/src` to the location of your choice, then make sure to pass the
   flag `-I <that location>` to `zap` when you run it.

If you don't care about C++ support, you can stop here. The compiler exe can be used with plugins
provided by projects implementing Zap in other languages.

If you want to use Zap in C++ with Visual Studio, do the following:

1. Make sure that you are using Visual Studio 2022 or newer, with all updates installed. Zap
    uses C++20 language features that did not work in previous versions of Visual Studio,
   and the updates include many bug fixes that Zap requires.

2. Install [CMake](http://www.cmake.org/) version 3.16 or later.

3. Use CMake to generate Visual Studio project files under `zap-c++-0.0.0` in the zip file.
   You can use the CMake UI for this or run this shell command:

       cmake -G "Visual Studio 17 2022"

3. Open the "Zap" solution in Visual Studio.

4. Adjust the project build options (e.g., choice of C++ runtime library, enable/disable exceptions
   and RTTI) to match the options of the project in which you plan to use Zap.

5. Build the solution (`ALL_BUILD`).

6. Build the `INSTALL` project to copy the compiled libraries, tools, and header files into
   `CMAKE_INSTALL_PREFIX`.

   Alternatively, find the compiled `.lib` files in the build directory under
   `src/{zap,kj}/{Debug,Release}` and place them somewhere where your project can link against them.
   Also add the `src` directory to your search path for `#include`s, or copy all the headers to your
   project's include directory.

Zap can also be built with MinGW or Cygwin, using the Unix/autotools build instructions.

**From Git**

The C++ sources are located under `c++` directory in the git repository. The build instructions are
otherwise the same as for the release zip.

