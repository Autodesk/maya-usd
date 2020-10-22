//
// Copyright 2019 Autodesk
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#include <pxr/base/arch/export.h>

// clang-format off
#if defined _WIN32 || defined __CYGWIN__

  // The main export symbol used for the core library.
  #ifdef MAYAUSD_CORE_EXPORT
    #ifdef __GNUC__
      #define MAYAUSD_CORE_PUBLIC __attribute__ ((dllexport))
    #else
      #define MAYAUSD_CORE_PUBLIC __declspec(dllexport)
    #endif
  #else
    #ifdef __GNUC__
      #define MAYAUSD_CORE_PUBLIC __attribute__ ((dllimport))
    #else
      #define MAYAUSD_CORE_PUBLIC __declspec(dllimport)
    #endif
  #endif
  #define MAYAUSD_CORE_LOCAL

  // We have a separate export symbol used in the helper macros.
  #ifdef MAYAUSD_MACROS_EXPORT
    #ifdef __GNUC__
      #define MAYAUSD_MACROS_PUBLIC __attribute__ ((dllexport))
    #else
      #define MAYAUSD_MACROS_PUBLIC __declspec(dllexport)
    #endif
  #else
    #ifdef __GNUC__
      #define MAYAUSD_MACROS_PUBLIC __attribute__ ((dllimport))
    #else
      #define MAYAUSD_MACROS_PUBLIC __declspec(dllimport)
    #endif
  #endif
  #define MAYAUSD_MACROS_LOCAL
#else
  #if __GNUC__ >= 4
    #define MAYAUSD_CORE_PUBLIC __attribute__ ((visibility ("default")))
    #define MAYAUSD_CORE_LOCAL  __attribute__ ((visibility ("hidden")))

    #define MAYAUSD_MACROS_PUBLIC __attribute__ ((visibility ("default")))
    #define MAYAUSD_MACROS_LOCAL  __attribute__ ((visibility ("hidden")))
#else
    #define MAYAUSD_CORE_PUBLIC
    #define MAYAUSD_CORE_LOCAL

    #define MAYAUSD_MACROS_PUBLIC
    #define MAYAUSD_MACROS_LOCAL
#endif
#endif

#ifdef MAYAUSD_CORE_EXPORT
  #define MAYAUSD_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
  #define MAYAUSD_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#else
  #define MAYAUSD_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
  #define MAYAUSD_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#endif
// clang-format on

// Convenience symbol versioning include: because api.h is widely
// included, this reduces the need to explicitly include mayaUsd.h.
#include <mayaUsd/mayaUsd.h>
