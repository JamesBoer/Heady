# Heady
Heady is a small utility and library designed to scan a set of C++ files and generate a single header file from all source files.  This is useful for library developers who wish to offer a single combined library header, but wish to develop the library using a more traditional structure with multiple header and source files.

Heady automatically detects any #include directives in the form ```#include "filename.h"```, in which quotes are used instead of angle brackets <>.  No alteration to the source content is made aside from stripping out the ```#include "filename.h"``` directives and replacing it with the contents of the header file.  As such, source files must be written in such a matter as to be "header-friendly".

## Requirements
Heady is written in standard C++ 17, and should be compliant with any compiler and standard library which fully implements this standard.  Due to a current lack of support for <filesystem> on macOS (as of 2019-01-04), you may not be able to compile on that platform.  Ubuntu 18.04 also does not provide gcc 8.x by default, which is required to compile the new filesystem API, but this can be obtained without too much effort.

## Using Heady
Heady is a small utility designed to be integrated as part of a build or post-build process.

    usage:
    Heady options

    where options are:
        -s, --source <folder>       folder containing source files
        -e, --excluded <files>      exclude specific files
        -i, --inline <inline>       inline macro substitution
        -o, --output <file>         generated header file
        -r, --recursive             recursively scan source folder
        -?, -h, --help              display usage information

    Example usage:
    Heady --source "Source" --excluded "Main.cpp clara.hpp" --output "Include\Heady.hpp"

## Building Heady
Heady uses CMake for building projects on each supported platform.  Make sure CMake (minimum v10) is installed, then run the corresponding batch or script file in ```/Bin```.

## Heady as a Library
Naturally, the heady library is available as an amalgamated single header file, generated by Heady from its own source.  You can find the merged header file in ```/Include/Heady.hpp```

## Techniques for Creating a Single Header Library
No utility (and certainly not Heady) is smart enough to convert any arbitrary library into a header only library without some preparatory work.  Here are the techniques required to ensure your library can be easily amalgamated into a single header file from its original source files.

### No "using namespace" in source files
This rule should be reasonably obvious, since you never want ```using namespace``` to pollute a header file.  Instead of ```using namespace``` in your source files, convert them instead to:

    // MyLib.cpp
    namespace MyLib
    {
        Foo::Foo()
        {
        }
    }
    
You could also choose to explicitly prefix your code with the appropriate namespace.

    MyLib::Foo::Foo()
    {
    }

### Internal APIs should be placed into a nested namespace
Since all APIs will be placed into a public-facing header, it's important that those APIs not meant for public use be clearly marked as "internal" in some fashion, to signal intent and prevent inadvertent usage.  The namespace "detail" is often used.  I occasionally use "impl" as well.

Before:

    namespace MyLib
    {
        class Foo
        {
            public:
            Foo() {}
        };
    } 
    
After (method 1):

    namespace MyLib
    {
        namespace Impl
        {
            class Foo
            {
                public:
                Foo() {}
            };
        }
    } 

After (method 2):

    namespace MyLib::Impl
    {
        class Foo
        {
            public:
            Foo() {}
        };
    } 


C++ 17 allows compound namespaces, which makes things a bit easier in the conversion, and doesn't waste as much whitespace.

### Convert standalone static data to static class members
Static data defined inside source files must be converted to static members of a class or struct.  This allows it to perform the same function of storing shared internal data, but will work as expected from a single header.  Fortunately, C++ 17 allows us to do this in a clean, simple fashion, without requiring some of the more convoluted hacks required in previous versions of C++.

Before:

    // MyLib.cpp
    namespace MyLib
    {
        static int s_data = 3;    
    
        int GetData()
        {
            return s_data;
        }
    }
    
After:

    // MyLib.cpp
    
    namespace MyLib::Impl
    {
        struct Data
        {
            static inline int data = 3;
        }
        
        inline_t int GetData()
        {
            return Data::data;
        }
    }

### Functions defined in .cpp files must be marked to allow inlining.
Functions, member functions, constructors, and destructors defined in a header (but outside of the class definition) must be marked as ```inline```, or they will violate the [one-definition rule](https://en.wikipedia.org/wiki/One_Definition_Rule).

Heady doesn't parse and identify all functions defined in source files.  Instead, it relies on a simple, unique identifier that it can transform into the ```inline``` keyword when the files are moved into the amalgamated header file.

By default, Heady looks for ```inline_t```, and replaces it with ```inline``` during transformation.

Example:

    // MyLib.h
    #define inline_t

    // MyLib.cpp
    namespace MyLib::Impl
    {
        inline_t Foo::Foo()
        {
        }
    }
    
All functions, member functions, constructors and destructors must be marked with the ```inline_t``` macro.  You may also define your own macro, and pass that in as a command-line argument.
   
### That's It!
Just follow those four simple rules, and you can have your library compiling as an amalgamated header in no time!
