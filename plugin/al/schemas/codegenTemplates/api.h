//
// Copyright 2017 Pixar
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
#ifndef{ { Upper(libraryName) } } _API_H
#define{ { Upper(libraryName) } } _API_H

#include <pxr/base/arch/export.h>

#if defined(PXR_STATIC)
#define{ { Upper(libraryName) } } _API
#define{ { Upper(libraryName) } } _API_TEMPLATE_CLASS(...)
#define{ { Upper(libraryName) } } _API_TEMPLATE_STRUCT(...)
#define{ { Upper(libraryName) } } _LOCAL
#else
#if defined({ { Upper(libraryName) } } _EXPORTS)
#define{ { Upper(libraryName) } } _API ARCH_EXPORT
#define{ { Upper(libraryName) } } _API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#define{ { Upper(libraryName) } } _API_TEMPLATE_STRUCT(...) \
    ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#else
#define{ { Upper(libraryName) } } _API ARCH_IMPORT
#define{ { Upper(libraryName) } } _API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#define{ { Upper(libraryName) } } _API_TEMPLATE_STRUCT(...) \
    ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#endif
#define{ { Upper(libraryName) } } _LOCAL ARCH_HIDDEN
#endif

#endif
