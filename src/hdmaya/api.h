//
// Copyright 2018 Luma Pictures
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http:#www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef __HDMAYA_API_H__
#define __HDMAYA_API_H__

#ifdef __GNUC__
#   define HDMAYA_API_EXPORT __attribute__((visibility("default")))
#   define HDMAYA_API_IMPORT
#elif defined(_WIN32) || defined(_WIN64)
#   define HDMAYA_API_EXPORT __declspec(dllexport)
#   define HDMAYA_API_IMPORT __declspec(dllimport)
#else
#   define HDMAYA_API_EXPORT
#   define HDMAYA_API_IMPORT
#endif

#if defined(HDMAYA_STATIC)
#   define HDMAYA_API
#else
#   if defined(HDMAYA_EXPORT)
#      define HDMAYA_API HDMAYA_API_EXPORT
#   else
#      define HDMAYA_API HDMAYA_API_IMPORT
#   endif
#endif

#endif // __HDMAYA_API_H__
