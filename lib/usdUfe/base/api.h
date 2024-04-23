//
// Copyright 2023 Autodesk
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <pxr/base/arch/export.h>

// clang-format off
#if defined _WIN32 || defined __CYGWIN__

  // The main export symbol used for the UsdUfe library.
  #ifdef USDUFE_EXPORT
    #ifdef __GNUC__
      #define USDUFE_PUBLIC __attribute__ ((dllexport))
    #else
      #define USDUFE_PUBLIC __declspec(dllexport)
    #endif
  #else
    #ifdef __GNUC__
      #define USDUFE_PUBLIC __attribute__ ((dllimport))
    #else
      #define USDUFE_PUBLIC __declspec(dllimport)
    #endif
  #endif
#else
  #if __GNUC__ >= 4
    #define USDUFE_PUBLIC __attribute__ ((visibility ("default")))
#else
    #define USDUFE_PUBLIC
#endif
#endif
// clang-format on

// Convenience symbol versioning include: because api.h is widely
// included, this reduces the need to explicitly include mayaUsd.h.
#include <usdUfe/usdUfe.h>
