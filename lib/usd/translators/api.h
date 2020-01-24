//
// Copyright 2018 Pixar
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
#ifndef PXRUSDTRANSLATORS_API_H
#define PXRUSDTRANSLATORS_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define PXRUSDTRANSLATORS_API
#   define PXRUSDTRANSLATORS_API_TEMPLATE_CLASS(...)
#   define PXRUSDTRANSLATORS_API_TEMPLATE_STRUCT(...)
#   define PXRUSDTRANSLATORS_LOCAL
#else
#   if defined(PXRUSDTRANSLATORS_EXPORTS)
#       define PXRUSDTRANSLATORS_API ARCH_EXPORT
#       define PXRUSDTRANSLATORS_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define PXRUSDTRANSLATORS_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define PXRUSDTRANSLATORS_API ARCH_IMPORT
#       define PXRUSDTRANSLATORS_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define PXRUSDTRANSLATORS_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define PXRUSDTRANSLATORS_LOCAL ARCH_HIDDEN
#endif

#endif
