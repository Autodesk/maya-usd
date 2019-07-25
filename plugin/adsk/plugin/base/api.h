// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================

#if defined _WIN32 || defined __CYGWIN__

  // We need a different export symbol for the plugin because when we are building
  // the actual Maya plugin we must 'export' the two functions for plugin load/unload.
  // And then we won't set the core export because we need to 'import' the symbols.
  #ifdef MAYAUSD_PLUGIN_EXPORT
    #ifdef __GNUC__
      #define MAYAUSD_PLUGIN_PUBLIC __attribute__ ((dllexport))
    #else
      #define MAYAUSD_PLUGIN_PUBLIC __declspec(dllexport)
    #endif
  #else
    #ifdef __GNUC__
      #define MAYAUSD_PLUGIN_PUBLIC __attribute__ ((dllimport))
    #else
      #define MAYAUSD_PLUGIN_PUBLIC __declspec(dllimport)
    #endif
  #endif
  #define MAYAUSD_PLUGIN_LOCAL
#else
  #if __GNUC__ >= 4
    #define MAYAUSD_PLUGIN_PUBLIC __attribute__ ((visibility ("default")))
    #define MAYAUSD_PLUGIN_LOCAL  __attribute__ ((visibility ("hidden")))
#else
    #define MAYAUSD_PLUGIN_PUBLIC
    #define MAYAUSD_PLUGIN_LOCAL
#endif
#endif

// Convenience symbol versioning include: because api.h is widely
// included, this reduces the need to explicitly include mayaUsd.h.
#include "mayaUsd/mayaUsd.h"
