This document outlines coding guidelines for contributions to the  [maya-usd](https://github.com/autodesk/maya-usd) project.

# C++ Coding Guidelines
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

### Include directive
All included public header files from outside and inside the project should be `#include`’d using angle brackets. For example:
```cpp
#include <pxr/base/tf/stringUtils.h>
#include <mayaUsd/nodes/stageData.h>
```

Private project’s header files should be `#include`'d using the file name when in the same folder. Private headers may live in sub-directories, but they should never be included using "._" or ".._" as part of a relative paths. For example:
```cpp
#include “privateUtils.h"
#include “pvt/helperFunctions.h"
```

### Include order
Headers should be included in the following order, with each section separated by a blank line and files sorted alphabetically:

1. Related header
2. C system headers
3. C++ standard library headers
4. Other libraries’ headers
5. Autodesk + Maya headers
6. Pixar + USD headers
7. Your project’s headers
8. Conditional includes

```cpp
#include “exportTranslator.h"
 
#include <string>
 
#include <maya/MFileObject.h>
#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>
#include <maya/MString.h>

#include <mayaUsd/fileio/jobs/jobArgs.h>
#include <mayaUsd/fileio/jobs/writeJob.h>
#include <mayaUsd/fileio/shading/shadingModeRegistry.h>
#include <mayaUsd/fileio/utils/writeUtil.h>

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
	* `USD_VERSION_NUM` is the macro to test USD version (`USD_MAJOR_VERSION` * 10000 + `USD_MINOR_VERSION` * 100 + `USD_PATCH_VERSION`)

Respect the minimum supported version for Maya and USD stated in [build.md](https://github.com/Autodesk/maya-usd/blob/dev/doc/build.md) .

### std over boost
Recent extensions to the C++ standard introduce many features previously only found in [boost](http://boost.org). To avoid introducing additional dependencies, developers should strive to use functionality in the C++ std over boost. If you encounter usage of boost in the code, consider converting this to the equivalent std mechanism. 
Our library currently has the following boost dependencies:
* `boost::python`
* `boost::hash_combine` (see  [this proposal](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0814r0.pdf) )
* `boost::filesystem` (preferable to replace with Pixar USD arch/fileSystem)
* `boost::system`
* `boost::make_shared` (preferable to replace with `std::shared_ptr`)

## Modern C++
Our goal is to develop [maya-usd](https://github.com/autodesk/maya-usd) following modern C++ practices. We’ll follow the [C++ Core Guidelines](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines) and pay attention to:
* `using` (vs `typedef`) keyword
* `virtual`, `override` and `final` keyword
* `default` and `delete` keywords
* `auto` keyword
* initialization - `{}`
* `nullptr` keyword
* …

