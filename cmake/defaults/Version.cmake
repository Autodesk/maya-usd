#
# Copyright 2016 Pixar
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Versioning information
set(PXR_MAJOR_VERSION "0")
set(PXR_MINOR_VERSION "19")
set(PXR_PATCH_VERSION "11")

math(EXPR PXR_VERSION "${PXR_MAJOR_VERSION} * 10000 + ${PXR_MINOR_VERSION} * 100 + ${PXR_PATCH_VERSION}")
