//
// Copyright 2017 Animal Logic
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

#if defined _WIN32 || defined __CYGWIN__
#ifdef AL_EVENT_EXPORT
#ifdef __GNUC__
#define AL_EVENT_PUBLIC __attribute__((dllexport))
#else
#define AL_EVENT_PUBLIC __declspec(dllexport)
#endif
#else
#ifdef __GNUC__
#define AL_EVENT_PUBLIC __attribute__((dllimport))
#else
#define AL_EVENT_PUBLIC __declspec(dllimport)
#endif
#endif
#define AL_EVENT_LOCAL
#else
#if __GNUC__ >= 4
#define AL_EVENT_PUBLIC __attribute__((visibility("default")))
#define AL_EVENT_LOCAL  __attribute__((visibility("hidden")))
#else
#define AL_EVENT_PUBLIC
#define AL_EVENT_LOCAL
#endif
#endif