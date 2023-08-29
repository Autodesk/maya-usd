//
// TM & (c) 2017 Lucasfilm Entertainment Company Ltd. and Lucasfilm Ltd.
// All rights reserved.  See LICENSE.txt for license.
//

#ifndef MATERIALX_COMBINEDVERSION_H
#define MATERIALX_COMBINEDVERSION_H

/// @file
/// GLSL fragment generator

#include <MaterialXCore/Generated.h>

#if !defined(MATERIALX_NAMESPACE_BEGIN)
// The above define was introduced in MaterialX 1.38.3. If it does not exist, we know we are on an
// earlier 1.38 MaterialX version that can use the same code we use for 1.38.3
#define MX_COMBINED_VERSION 13803
#else
#define MX_COMBINED_VERSION                                                  \
    ((MATERIALX_MAJOR_VERSION * 100 * 100) + (MATERIALX_MINOR_VERSION * 100) \
     + MATERIALX_BUILD_VERSION)
#endif

#endif
