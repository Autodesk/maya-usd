// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================

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

// Convenience symbol versioning include: because MayaUsdApi.h is widely
// included, this reduces the need to explicitly include MayaUsd.h.
#include "mayaUsdCore/mayaUsd.h"
