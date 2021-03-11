This document outlines coding guidelines for contributions to the  [maya-usd](https://github.com/autodesk/maya-usd) project.

# C++ Coding Guidelines

Many of the C++ coding guidelines below are validated and enforced through the use of `clang-format` which is provided by the [LLVM project](https://github.com/llvm/llvm-project). Since the adjustments made by `clang-format` can vary from version to version, we standardize this project on a single `clang-format` version to ensure consistent results for all contributions made to maya-usd.

|                      | Version | Source Code | Release |
|:--------------------:|:-------:|:-----------:|:-------:|
|  `clang-format`/LLVM |  10.0.0 | [llvmorg-10.0.0 Tag](https://github.com/llvm/llvm-project/tree/llvmorg-10.0.0) | [LLVM 10.0.0](https://github.com/llvm/llvm-project/releases/tag/llvmorg-10.0.0) |

## Foundation/Critical
### License notice
Every file should start with the Apache 2.0 licensing statement:
```cpp
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an “AS IS” BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
```

### Copyright notice
* Every file should contain at least one Copyright line at the top, which can be to Autodesk or the individual/company that contributed the code.
* Multiple copyright lines are allowed, and if a significant new contribution is made to an existing file then an individual or company can append a new line to the copyright section.
* The year the original contribution is made should be included but there is no requirement to update this every year.
* There is no requirement that an Autodesk copyright line should be in all files. If an individual or company contributes new files they do not have to add both their name and Autodesk's name.
* If existing code is being refactored or moved within the repo then the original copyright lines should be maintained. You should only append a new copyright line if significant new changes are added. For example, if some utility code is being moved from a plugin into a common area, and the work involved is only minor name changes or include path updates, then the original copyright should be maintained. In this case, there is no requirement to add a new copyright for the person that handled the refactoring.

### #pragma once vs include guard
Do not use `#pragma once` to avoid files being `#include’d` multiple times (which can cause duplication of definitions & compilation errors). While it's widely supported, it's a non-standard and non-portable extension. In some cases using `#pragma once` may include two copies of the same file when they are accessible via multiple different paths.

All code should use include guards instead `#pragma once`. For example:
```cpp
// file foobar.h:
#ifndef LIBRARY_FOOBAR_H
#define LIBRARY_FOOBAR_H
// … declarations …
#endif // LIBRARY_FOOBAR_H
```

Never use reserved identifiers, like underscore followed immediately by an uppercase letter or double underscore. For example, the following is illegal:
```cpp
// file foobar.h:
#ifndef __LIBRARY_FOOBAR_H
#define __LIBRARY_FOOBAR_H
// … declarations …
#endif // __LIBRARY_FOOBAR_H

// file foobar2.h:
#ifndef _LIBRARY_FOOBAR2_H
#define _LIBRARY_FOOBAR2_H
// … declarations …
#endif // _LIBRARY_FOOBAR2_H
```

### Naming (file, type, variable, constant, function, namespace, macro, template parameters, schema names)
**General Naming Rules**
MayaUsd strives to use “camel case” naming. That is, each word is capitalized, except possibly the first word:
* UpperCamelCase
* lowerCamelCase

While underscores in names (_) (e.g., as separator) are not strictly forbidden, they are strongly discouraged.
Optimize for readability by selecting names that are clear to others (e.g., people on different teams.)
Use names that describe the purpose or intent of the object. Do not worry about saving horizontal space. It is more important to make your code easily understandable by others. Minimize the use of abbreviations that would likely be unknown to someone outside your project (especially acronyms and initialisms).

**File Names**
Filenames should be lowerCamelCase and should not include underscores (_) or dashes (-).
C++ files should end in .cpp and header files should end in .h.
In general, make your filenames as specific as possible. For example:
```
stageData.cpp
stageData.h
```

**Type Names**
All type names (i.e., classes, structs, type aliases, enums, and type template parameters) should use UpperCamelCase, with no underscores. For example:
```cpp
class MayaUsdStageData;
class ImportData;
enum Roles;
```

**Variable Names**
Variables names, including function parameters and data members should use lowerCamelCase, with no underscores. For example:
```cpp
const MDagPath& dagPath
const MVector& rayDirection
bool* drawRenderPurpose
```

**Class/Struct Data Members**
Non-static data members of classes/structs are named like ordinary non-member variables with leading “_”. For example:
```cpp
UsdMayaStageNoticeListener _stageNoticeListener;
std::map<UsdTimeCode, MBoundingBox> _boundingBoxCache;
```

For static data members the leading underscore should be omitted. 
```cpp
static const MTypeId typeId;
```

**Constant Names**
Variables declared constexpr or const, whose value is fixed for the duration of the program, should be named with a leading “k” followed by UpperCamelCase. For example:
```cpp
const int kDaysInAWeek = 7;
const int kMyMagicNumber = 42;
```

**Function/Method Names**
All functions should be lowerCamelCase. This avoids inconsistencies with the MayaAPI and virtual methods. For example:
```cpp
MString name() const override;
void registerExitCallback();
```

**Namespace Names**
Namespace names should be UpperCamelCase. Top-level namespace names are based on the project name.
```cpp
namespace MayaAttrs {}
```

**Enumerator Names**
Enumerators (for both scoped and unscoped enums) should be named like constants (i.e., `kEnumName`.)

The enumeration name, `StringPolicy` is a type and therefore mixed case.
```cpp
enum class StringPolicy
{
  kStringOptional,
  kStringMustHaveValue
};
```

**Macro Names**
In general, macros should be avoided (see [Modern C++](https://docs.google.com/document/d/1Jvbpfh2WNzHxGQtjqctZ1K1lnpaAtHOUwm0kmmEcxjY/edit#heading=h.ynbggnv41p3) ). However, if they are absolutely needed, macros should be all capitals, with words separated by underscores.
```cpp
#define ROUND(x) …
#define PI_ROUNDED 3.0
```

**Schema Names**
<Not yet defined, proposals are welcome>

### Documentation (class, method, variable, comments)
* [Doxygen ](http://www.doxygen.nl/index.html) will be used to generate documentation from mayaUsd C++ sources.
* Doxygen tags must be used to document classes, function parameters, function return values, and thrown exceptions.
* The mayaUsd project does require the use of any Doxygen formatting style ( [Doxygen built-in formatting](http://www.doxygen.nl/manual/commands.html) )
* Comments for users of classes and functions must be written in headers files. Comments in definition files are meant for contributors and maintainers.

### Namespaces

#### In header files (e.g. .h)

* **Required:** to use fully qualified namespace names. Global scope using directives are not allowed.  Inline code can use using directives in implementations, within a scope, when there is no other choice (e.g. when using macros, which are not namespaced).

```cpp
// In aFile.h
inline PXR_NS::UsdPrim prim() const
{
    PXR_NAMESPACE_USING_DIRECTIVE
    TF_AXIOM(fItem != nullptr);
    return fItem->prim();
}
```

#### In implementation files (e.g. .cpp)

* **Recommended:** to use fully qualified namespace names, unless clarity or readability is degraded by use of explicit namespaces, in which case a using directive is acceptable.
* **Recommended:** to use the existing namespace style, and not make gratuitous changes. If the file is using explicit namespaces, new code should follow this style, unless the changes are so significant that clarity or readability is degraded.  If the file has one or more using directives, new code should follow this style.

### Include directive
For source files (.cpp) with an associated header file (.h) that resides in the same directory, it should be `#include`'d with double quotes and no path.  This formatting should be followed regardless of with whether the associated header is public or private. For example:
```cpp
// In foobar.cpp
#include "foobar.h"
```

All included public header files from outside and inside the project should be `#include`’d using angle brackets. For example:
```cpp
#include <pxr/base/tf/stringUtils.h>
#include <mayaUsd/nodes/stageData.h>
```

Private project’s header files should be `#include`'d using double quotes, and a relative path. Private headers may live in the same directory or sub-directories, but they should never be included using "._" or ".._" as part of a relative path. For example:
```cpp
#include "privateUtils.h"
#include "pvt/helperFunctions.h"
```

### Include order
Headers should be included in the following order, with each section separated by a blank line and files sorted alphabetically:

1. Related header
2. All private headers
3. All public headers from this repository (maya-usd)
4. Pixar + USD headers
5. Autodesk + Maya headers
6. Other libraries' headers
7. C++ standard library headers
8. C system headers
9. Conditional includes

```cpp
#include "exportTranslator.h"

#include "private/util.h"
 
#include <mayaUsd/fileio/jobs/jobArgs.h>
#include <mayaUsd/fileio/jobs/writeJob.h>
#include <mayaUsd/fileio/shading/shadingModeRegistry.h>
#include <mayaUsd/fileio/utils/writeUtil.h>
 
#include <maya/MFileObject.h>
#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>
#include <maya/MString.h>

#include <string>

#if defined(WANT_UFE_BUILD)
  #include <ufe/ufe.h>
#endif
```

### Cross-platform development
* Consult the [build.md](https://github.com/Autodesk/maya-usd/blob/dev/doc/build.md) compatibility table to ensure you use the recommended tool (i.e., compiler, operating system, CMake, etc.) versions.
* Before pull requests (PRs) are considered for integration, they must compile and all tests should pass on all suppported platforms.

### Conditional compilation (Maya, USD, UFE version)
**Maya**
	* `MAYA_API_VERSION` is the consistent macro to test Maya version (`MAYA_APP_VERSION` * 10000 + `MAJOR_VERSION` * 100 + `MINOR_VERSION`)
	* `MAYA_APP_VERSION` is available only since Maya 2019 and is a simple year number, so it is not allowed.
**UFE**
	* `WANT_UFE_BUILD` equals 1 if UFE is found and it should be used for conditional compilation on codes depending on UFE.

**USD**
	* `PXR_VERSION` is the macro to test USD version (`PXR_MAJOR_VERSION` * 10000 + `PXR_MINOR_VERSION` * 100 + `PXR_PATCH_VERSION`)

Respect the minimum supported version for Maya and USD stated in [build.md](https://github.com/Autodesk/maya-usd/blob/dev/doc/build.md) .

### std over boost
Recent extensions to the C++ standard introduce many features previously only found in [boost](http://boost.org). To avoid introducing additional dependencies, developers should strive to use functionality in the C++ std over boost. If you encounter usage of boost in the code, consider converting this to the equivalent std mechanism. 
Our library currently has the following boost dependencies:
* `boost::python`
* `boost::make_shared` (preferable to replace with `std::shared_ptr`)

***Update:***
* `boost::filesystem` and `boost::system` are removed. Until the transition to C++17 std::filesystem, [ghc::filesystem](https://github.com/gulrak/filesystem) must be used as an alternative across the project.

* Dependency on `boost::thread` is removed from Animal Logic plugin.

* `boost::hash_combine` is replaced with `MayaUsd::hash_combine` and should be used instead across the project.

## Modern C++
Our goal is to develop [maya-usd](https://github.com/autodesk/maya-usd) following modern C++ practices. We’ll follow the [C++ Core Guidelines](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines) and pay attention to:
* `using` (vs `typedef`) keyword
* `virtual`, `override` and `final` keyword
* `default` and `delete` keywords
* `auto` keyword
* initialization - `{}`
* `nullptr` keyword
* …

# Coding guidelines for Python
We are adopting the [PEP-8](https://www.python.org/dev/peps/pep-0008) style for Python Code with the following modification:
* Mixed-case for variable and function names are allowed 

[Pylint](https://www.pylint.org/) is recommended for automation.


# Coding guidelines for CMake 
## Modern CMake
1. Target Build and Usage requirements should be very clear.
* build requirements ( everything that is needed to build the target )
* usage requirements ( everything that is needed to use this target as a dependency of another target)
2. Always use target_xxx() and make sure to add the PUBLIC/PRIVATE/INTERFACE keywords as appropriate. 
* target_sources
* target_compile_definitions
* target_compile_options
* target_include_directories
* target_link_libraries
* ...

Keyword meanings:
* **PRIVATE**: requirement should apply to just this target.
* **PUBLIC**: requirement should apply to this target and anything that links to it.
* **INTERFACE**: requirement should apply just to things that link to it.

3. Don't use Macros that affect all targets (e.g add_definitions, link_libraries, include_directories).
4. Prefer functions over macros whenever reasonable.
5. Treat warnings as errors.
6. Use cmake_parse_arguments as the recommended way for parsing the arguments given to the macro or function.
7. Don't use file(GLOB).
8. Be explicit by calling set_target_properties when it's appropriate.
9. Links against Boost or GTest using imported targets rather than variables:
e.g Boost::filesystem, Boost::system, GTest::GTest

## Compiler features/flags/definitions
1. Setting or appending compiler flags/definitions via CMAKE_CXX_FLAGS is NOT allowed.
2. Any front-end flags (e.g. -Wno-xxx, cxx_std_xx) should be added to cmake/compiler_config.cmake
3. All current targets, as well as newly added targets, must use mayaUsd_compile_config function in order to get the project-wide flags/definitions. These flags/definitions are added privately via target_compile_features, target_compile_options, target_compile_definitions. Individual targets are still allowed to call target_compile_definitions, target_compile_features individually for any additional flags/definitions by providing appropriate PUBLIC/INTERFACE/PRIVATE keywords. For example:
```cmake
# -----------------------------------------------------------------------------
# compiler configuration
# -----------------------------------------------------------------------------
add_library(${UFE_PYTHON_TARGET_NAME} SHARED)
target_compile_definitions(${UFE_PYTHON_TARGET_NAME}
   PRIVATE
       MFB_PACKAGE_NAME=${UFE_PYTHON_MODULE_NAME}
       MFB_ALT_PACKAGE_NAME=${UFE_PYTHON_MODULE_NAME}
       MFB_PACKAGE_MODULE="${PROJECT_NAME}.${UFE_PYTHON_MODULE_NAME}"
)
mayaUsd_compile_config(${UFE_PYTHON_TARGET_NAME})
```

## Dynamic linking and Run-time Search Path
Use provided mayaUsd_xxx_rpath() utility functions to handle run-time search path for both MacOSX and Linux.

## Unit Test
1. Use provided mayaUsd_add_test() function for adding either Gtest or Python tests.
2. Don't add find_package(GTest REQUIRED) in every CMake file with unit tests.  Gtest setup is handled automatically for you.
3. For targets that need to link against google test library. Simply add GTest::GTest to target_link_libraries. You don't need to add any Gtest include directory this is done automatically for you.

## Naming Conventions
 1. CMake commands are case-insensitive. Use lower_case for CMake functions and macros, Use upper_case for CMake variables
 ```cmake
# e.g cmake functions
add_subdirectory(schemas)
target_compile_definitions(....)

# e.g cmake variables
${CMAKE_CXX_COMPILER_ID}
${CMAKE_SYSTEM_NAME}
${CMAKE_INSTALL_PREFIX}
```

 2. Use upper_case for Option names
```cmake
# e.g for options names
option(BUILD_USDMAYA_SCHEMAS "Build optional schemas." ON)
option(BUILD_TESTS "Build tests." ON)
option(BUILD_HDMAYA "Build the Maya-To-Hydra plugin and scene delegate." ON)
```

3.  Use upper_case for Custom variables
```cmake
# e.g for options names
set(QT_VERSION "5.6")
 
set(HEADERS
    jobArgs.h
    modelKindProcessor.h
    readJob.h
    writeJob.h
)
 
set(RESOURCES_INSTALL_PATH ${CMAKE_INSTALL_PREFIX}/lib/usd/${TARGET_NAME}/resources)
 
set(USDTRANSACTION_PYTHON_LIBRARY_LOCATION ${AL_INSTALL_PREFIX}/lib/python/AL/usd/transaction)
```

4. Respect third-party variables ( don't change them )
```cmake
# e.g boost
set(BOOST_ROOT ${pxr_usd_location})
set(Boost_USE_DEBUG_PYTHON ON)
```

5. Avoid adding the optional name in endfunction/endmacro([]).
> Although there is no official guideline for Modern CMake practices, the following resources provide ample information on how to adopt these practices:
> [Modern cmake](https://cliutils.gitlab.io/modern-cmake/), [Effective Modern CMake](https://gist.github.com/mbinna/c61dbb39bca0e4fb7d1f73b0d66a4fd1)
