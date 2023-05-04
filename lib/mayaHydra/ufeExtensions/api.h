//
// Copyright 2023 Autodesk
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
#ifndef UFEEXTENSIONS_API_H
#define UFEEXTENSIONS_API_H

#ifdef __GNUC__
#define UFEEXTENSIONS_API_EXPORT __attribute__((visibility("default")))
#define UFEEXTENSIONS_API_IMPORT
#elif defined(_WIN32) || defined(_WIN64)
#define UFEEXTENSIONS_API_EXPORT __declspec(dllexport)
#define UFEEXTENSIONS_API_IMPORT __declspec(dllimport)
#else
#define UFEEXTENSIONS_API_EXPORT
#define UFEEXTENSIONS_API_IMPORT
#endif

#if defined(UFEEXTENSIONS_STATIC)
#define UFEEXTENSIONS_API
#else
#if defined(UFEEXTENSIONS_EXPORT)
#define UFEEXTENSIONS_API UFEEXTENSIONS_API_EXPORT
#else
#define UFEEXTENSIONS_API UFEEXTENSIONS_API_IMPORT
#endif
#endif

#endif // UFEEXTENSIONS_API_H
