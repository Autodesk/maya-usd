//
// Copyright 2019 Autodesk
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

// We need a different export symbol for the plugin because when we are building
// the actual Maya plugin we must 'export' the two functions for plugin load/unload.
// And then we won't set the core export because we need to 'import' the symbols.
#ifdef MAYAUSD_PLUGIN_EXPORT
#ifdef __GNUC__
#define MAYAUSD_PLUGIN_PUBLIC __attribute__((dllexport))
#else
#define MAYAUSD_PLUGIN_PUBLIC __declspec(dllexport)
#endif
#else
#ifdef __GNUC__
#define MAYAUSD_PLUGIN_PUBLIC __attribute__((dllimport))
#else
#define MAYAUSD_PLUGIN_PUBLIC __declspec(dllimport)
#endif
#endif
#define MAYAUSD_PLUGIN_LOCAL
#else
#if __GNUC__ >= 4
#define MAYAUSD_PLUGIN_PUBLIC __attribute__((visibility("default")))
#define MAYAUSD_PLUGIN_LOCAL  __attribute__((visibility("hidden")))
#else
#define MAYAUSD_PLUGIN_PUBLIC
#define MAYAUSD_PLUGIN_LOCAL
#endif
#endif
