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
#ifndef __HDMAYA_API_H__
#define __HDMAYA_API_H__

#ifdef __GNUC__
#define HDMAYA_API_EXPORT __attribute__((visibility("default")))
#define HDMAYA_API_IMPORT
#elif defined(_WIN32) || defined(_WIN64)
#define HDMAYA_API_EXPORT __declspec(dllexport)
#define HDMAYA_API_IMPORT __declspec(dllimport)
#else
#define HDMAYA_API_EXPORT
#define HDMAYA_API_IMPORT
#endif

#if defined(HDMAYA_STATIC)
#define HDMAYA_API
#else
#if defined(HDMAYA_EXPORT)
#define HDMAYA_API HDMAYA_API_EXPORT
#else
#define HDMAYA_API HDMAYA_API_IMPORT
#endif
#endif

#endif // __HDMAYA_API_H__
