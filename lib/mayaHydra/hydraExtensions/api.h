//
// Copyright 2019 Luma Pictures
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
#ifndef MAYAHYDRALIB_API_H
#define MAYAHYDRALIB_API_H

#ifdef __GNUC__
#define MAYAHYDRALIB_API_EXPORT __attribute__((visibility("default")))
#define MAYAHYDRALIB_API_IMPORT
#elif defined(_WIN32) || defined(_WIN64)
#define MAYAHYDRALIB_API_EXPORT __declspec(dllexport)
#define MAYAHYDRALIB_API_IMPORT __declspec(dllimport)
#else
#define MAYAHYDRALIB_API_EXPORT
#define MAYAHYDRALIB_API_IMPORT
#endif

#if defined(MAYAHYDRALIB_STATIC)
#define MAYAHYDRALIB_API
#else
#if defined(MAYAHYDRALIB_EXPORT)
#define MAYAHYDRALIB_API MAYAHYDRALIB_API_EXPORT
#else
#define MAYAHYDRALIB_API MAYAHYDRALIB_API_IMPORT
#endif
#endif

#endif // MAYAHYDRALIB_API_H
