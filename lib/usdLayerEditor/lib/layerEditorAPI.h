//
// Copyright 2024 Autodesk
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
#    ifdef LAYEREDITOR_EXPORTS
#        ifdef __GNUC__
#            define LAYEREDITOR_PUBLIC __attribute__((dllexport))
#        else
#            define LAYEREDITOR_PUBLIC __declspec(dllexport)
#        endif
#    else
#        ifdef __GNUC__
#            define LAYEREDITOR_PUBLIC __attribute__((dllimport))
#        else
#            define LAYEREDITOR_PUBLIC __declspec(dllimport)
#        endif
#    endif
#else
#    if __GNUC__ >= 4
#        define LAYEREDITOR_PUBLIC __attribute__((visibility("default")))
#    else
#        define LAYEREDITOR_PUBLIC
#    endif
#endif

#if defined _WIN32 || defined __CYGWIN__
#    ifdef LAYEREDITOR_UI_EXPORTS
#        ifdef __GNUC__
#            define LAYEREDITOR_UI_PUBLIC __attribute__((dllexport))
#        else
#            define LAYEREDITOR_UI_PUBLIC __declspec(dllexport)
#        endif
#    else
#        ifdef __GNUC__
#            define LAYEREDITOR_UI_PUBLIC __attribute__((dllimport))
#        else
#            define LAYEREDITOR_UI_PUBLIC __declspec(dllimport)
#        endif
#    endif
#else
#    if __GNUC__ >= 4
#        define LAYEREDITOR_UI_PUBLIC __attribute__((visibility("default")))
#    else
#        define LAYEREDITOR_UI_PUBLIC
#    endif
#endif
