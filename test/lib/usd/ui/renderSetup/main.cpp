//
// Copyright 2026 Autodesk
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

// MayaRendererProvider issues MEL "renderer" queries and looks up
// defaultRenderGlobals, so (unlike the other C++ unit tests under
// test/lib/mayaUsd/utils) this test executable needs a real Maya session.
// Bring one up in library mode before handing control to gtest.

#include <maya/MGlobal.h>
#include <maya/MLibrary.h>

#include <gtest/gtest.h>

#include <cstdio>

int main(int argc, char** argv)
{
    MStatus status = MLibrary::initialize(argv[0], /*useBatchLicense*/ true);
    if (!status) {
        status.perror("MLibrary::initialize");
        return 1;
    }

    // Registers UsdSettingsNode/UsdDefaultRenderDescription (used by
    // MayaRendererProvider's Hydra-side currentRenderer()/switchRenderer())
    // and the setCurrentRenderer MEL proc that switchRenderer() invokes.
    if (!MGlobal::executeCommand("loadPlugin \"mayaUsdPlugin\"")) {
        fprintf(stderr, "Failed to load mayaUsdPlugin.\n");
        MLibrary::cleanup(1);
        return 1;
    }

    ::testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();

    MLibrary::cleanup(result);
    return result;
}
