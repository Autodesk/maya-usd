# change to git root dir
cd "$(git rev-parse --show-toplevel)"

# Some commit reference points:

# 4b46bfd3b5ea96c709547e830bb645d60c21fa29 - plugins/USD (initial import of pixar from submodule)
# 825ca13dd77af84872a063f146dee1799e8be25c - plugins/PXR_USDMaya (renamed dir)
# 141bab7eba1d380868e822a51f8c8f85e1c0b66f - plugins/PXR_USDMaya (identical contents as above)
# e5e10a28d0ba0535e83675399a5d15314fb79ec9 - plugin/pxr (renamed dir)

# This function will take a "stock" pixar USD repo, and rename / delete files
# and folders to make it "line up" with their locations in maya-usd

function renamePixarRepo ()
{
    cd "$(git rev-parse --show-toplevel)"

    # move everything in the root to plugin/pxr
    rm -rf plugin/pxr
    mkdir -p plugin/pxr
    git mv -k $(ls -A) plugin/pxr

    # Move back some of the cmake stuff to the root
    mkdir -p cmake
    git mv plugin/pxr/cmake/defaults/ cmake/
    git mv plugin/pxr/cmake/modules/ cmake/

    # Remove a bunch of files / folders
    git rm -f cmake/defaults/Packages.cmake
    git rm -f cmake/modules/FindGLEW.cmake
    git rm -f cmake/modules/FindPTex.cmake
    git rm -f cmake/modules/FindRenderman.cmake
    git rm -f plugin/pxr/.appveyor.yml
    git rm -f plugin/pxr/.gitignore
    git rm -f plugin/pxr/.travis.yml
    git rm -f plugin/pxr/BUILDING.md
    git rm -f plugin/pxr/CHANGELOG.md
    git rm -f plugin/pxr/CONTRIBUTING.md
    git rm -f plugin/pxr/NOTICE.txt
    git rm -f plugin/pxr/README.md
    git rm -f plugin/pxr/USD_CLA_Corporate.pdf
    git rm -f plugin/pxr/USD_CLA_Individual.pdf
    git rm -f plugin/pxr/cmake/macros/generateDocs.py
    git rm -f plugin/pxr/pxr/CMakeLists.txt
    git rm -f plugin/pxr/pxr/pxrConfig.cmake.in

    git rm -rf plugin/pxr/.github/
    git rm -rf plugin/pxr/build_scripts/
    git rm -rf plugin/pxr/extras/
    git rm -rf plugin/pxr/pxr/base/
    git rm -rf plugin/pxr/pxr/imaging/
    git rm -rf plugin/pxr/pxr/usd/
    git rm -rf plugin/pxr/pxr/usdImaging/
    git rm -rf plugin/pxr/third_party/houdini/
    git rm -rf plugin/pxr/third_party/katana/
    git rm -rf plugin/pxr/third_party/renderman-22/

    git mv plugin/pxr/third_party/maya plugin/pxr/maya

    python replace_lic.py --pxr
}

##############################
# branch: renamed_v1905
##############################
# Make a branch that's IDENTICAL to stock 19.05, except with directory renames/
# file moves / deletions to get files into the same place as in the master
# branch of Maya-USD
# 
# This branch is a useful reference point, and will be used to make a diff /
# patch file which will handy when doing merges.

git checkout -B renamed_v1905 v19.05
renamePixarRepo
git commit -a -m "Renamed / deleted files from 19.05 to match maya-usd layout"


##############################
# branch: renamed_pxr_dev
##############################
# Make a branch that's IDENTICAL to pixar's latest usd dev
# (b29152c2896b1b4d03fddbd9c3dcaad133d2c495), except with directory renames/
# file moves / deletions to get files into the same place as in the master
# branch of Maya-USD
# 
# This branch is a useful reference point, and will be used to make a diff /
# patch file which will handy when doing merges.

git checkout -B renamed_pxr_dev origin/pixar/dev
renamePixarRepo
git commit -a -m "Renamed / deleted files from pixar dev (b29152c2) to match maya-usd layout"

###############
# Make a patch that gives all changes between renamed_v1905 and renamed_pxr_dev
# ...this will be used when resolving merge conflicts

git diff renamed_v1905 renamed_pxr_dev > ../pixar_1905_dev.diff



###############
# Now that we have our helper diff, merge pixar-usd dev into latest maya-usd master

git checkout -B dev origin/master

# attempt the merge - this will give a lot of merge conflicts...
git merge origin/pixar/dev

# These files were removed / we don't care about:

git rm -f cmake/defaults/Packages.cmake
git rm -f cmake/modules/FindGLEW.cmake
git rm -f cmake/modules/FindPTex.cmake
git rm -f cmake/modules/FindRenderman.cmake
git rm -f .appveyor.yml
git rm -f .travis.yml
git rm -f BUILDING.md
git rm -f CHANGELOG.md
git rm -f CONTRIBUTING.md
git rm -f NOTICE.txt
git rm -f USD_CLA_Corporate.pdf
git rm -f USD_CLA_Individual.pdf
git rm -f cmake/macros/generateDocs.py
git rm -f pxr/CMakeLists.txt
git rm -f pxr/pxrConfig.cmake.in
git rm -rf .github/
git rm -rf build_scripts/
git rm -rf extras/
git rm -rf pxr/base/
git rm -rf pxr/imaging/
git rm -rf pxr/usd/
git rm -rf pxr/usdImaging/
git rm -rf third_party/houdini/
git rm -rf third_party/katana/
git rm -rf third_party/renderman-22

# These were new files, that we're moving into their proper places:

git mv third_party/maya/lib/pxrUsdMayaGL/testenv plugin/pxr/maya/lib/pxrUsdMayaGL
git mv third_party/maya/lib/usdMaya/testenv/UsdReferenceAssemblyChangeRepresentationsTest/* plugin/pxr/maya/lib/usdMaya/testenv/UsdReferenceAssemblyChangeRepresentationsTest
rm -rf third_party/maya/lib/usdMaya/testenv/UsdReferenceAssemblyChangeRepresentationsTest
git mv third_party/maya/lib/usdMaya/testenv/UsdExportAssemblyEditsTest plugin/pxr/maya/lib/usdMaya/testenv
git mv third_party/maya/lib/usdMaya/testenv/testUsdExportAssemblyEdits.py plugin/pxr/maya/lib/usdMaya/testenv/testUsdExportAssemblyEdits.py

git mv third_party/maya/plugin/pxrUsdTranslators/strokeWriter.* plugin/pxr/maya/plugin/pxrUsdTranslators
mkdir -p plugin/pxr/maya/plugin/pxrUsdTranslators/testenv/StrokeExportTest
git mv third_party/maya/plugin/pxrUsdTranslators/testenv/StrokeExportTest/StrokeExportTest.ma plugin/pxr/maya/plugin/pxrUsdTranslators/testenv/StrokeExportTest/StrokeExportTest.ma
git mv third_party/maya/plugin/pxrUsdTranslators/testenv/testPxrUsdTranslatorsStroke.py plugin/pxr/maya/plugin/pxrUsdTranslators/testenv/testPxrUsdTranslatorsStroke.py


# When I inspected these, determined we didn't care about any of the changes in
# these files between renamed_v1905 and renamed_pxr_dev - so just checking out
# old version

git checkout dev -- plugin/pxr/LICENSE.txt
git checkout dev -- CMakeLists.txt
git checkout dev -- cmake/defaults/Options.cmake


# Most of the rest of these seem to be files whose rename wasn't properly
# recorded by git - they're marked as modifications to deleted files. Solve by
# using the patch we made earlier.

function applyPixarRootDiff ()
{
    pxrPath="$1"
    adPath=plugin/pxr/"$1"
    git apply ../pixar_1905_dev.diff --include="$adPath"
    result="$?"
    if (( $result == 0 )); then
        echo "success!"
        git add "$adPath"
        git rm "$pxrPath"
    else
        echo
        echo '!!!!!!!!!!!'
        echo 'failure!'
        echo '!!!!!!!!!!!'
    fi
}

function applyPixarMayaDiff ()
{
    pxrPath="$1"
    adPath=$(echo "$pxrPath" | sed -e 's~third_party/maya~plugin/pxr/maya~')
    git apply ../pixar_1905_dev.diff --include="$adPath"
    result="$?"
    if (( $result == 0 )); then
        echo "success!"
        git add "$adPath"
        git rm "$pxrPath"
    else
        echo
        echo '!!!!!!!!!!!'
        echo 'failure!'
        echo '!!!!!!!!!!!'
    fi
}

applyPixarRootDiff cmake/macros/Private.cmake

applyPixarMayaDiff third_party/maya/lib/pxrUsdMayaGL/batchRenderer.cpp
applyPixarMayaDiff third_party/maya/lib/pxrUsdMayaGL/batchRenderer.h
applyPixarMayaDiff third_party/maya/lib/pxrUsdMayaGL/hdImagingShapeDrawOverride.cpp
applyPixarMayaDiff third_party/maya/lib/pxrUsdMayaGL/hdImagingShapeUI.cpp
applyPixarMayaDiff third_party/maya/lib/pxrUsdMayaGL/hdRenderer.cpp
applyPixarMayaDiff third_party/maya/lib/pxrUsdMayaGL/instancerShapeAdapter.cpp
applyPixarMayaDiff third_party/maya/lib/pxrUsdMayaGL/proxyDrawOverride.cpp
applyPixarMayaDiff third_party/maya/lib/pxrUsdMayaGL/proxyShapeDelegate.cpp
applyPixarMayaDiff third_party/maya/lib/pxrUsdMayaGL/proxyShapeUI.cpp
applyPixarMayaDiff third_party/maya/lib/pxrUsdMayaGL/sceneDelegate.cpp
applyPixarMayaDiff third_party/maya/lib/pxrUsdMayaGL/sceneDelegate.h
applyPixarMayaDiff third_party/maya/lib/pxrUsdMayaGL/shapeAdapter.cpp
applyPixarMayaDiff third_party/maya/lib/pxrUsdMayaGL/shapeAdapter.h
applyPixarMayaDiff third_party/maya/lib/pxrUsdMayaGL/usdProxyShapeAdapter.cpp
applyPixarMayaDiff third_party/maya/lib/usdMaya/CMakeLists.txt
applyPixarMayaDiff third_party/maya/lib/usdMaya/editUtil.cpp
applyPixarMayaDiff third_party/maya/lib/usdMaya/editUtil.h
applyPixarMayaDiff third_party/maya/lib/usdMaya/hdImagingShape.cpp
applyPixarMayaDiff third_party/maya/lib/usdMaya/hdImagingShape.h
applyPixarMayaDiff third_party/maya/lib/usdMaya/readJob.cpp
applyPixarMayaDiff third_party/maya/lib/usdMaya/referenceAssembly.cpp
applyPixarMayaDiff third_party/maya/lib/usdMaya/shadingModeImporter.h
applyPixarMayaDiff third_party/maya/lib/usdMaya/shadingModePxrRis.cpp
applyPixarMayaDiff third_party/maya/lib/usdMaya/shadingModeUseRegistry.cpp
applyPixarMayaDiff third_party/maya/lib/usdMaya/testenv/testUsdExportPackage.py
applyPixarMayaDiff third_party/maya/lib/usdMaya/testenv/testUsdExportRfMLight.py
applyPixarMayaDiff third_party/maya/lib/usdMaya/testenv/testUsdExportShadingModePxrRis.py
applyPixarMayaDiff third_party/maya/lib/usdMaya/testenv/testUsdImportRfMLight.py
applyPixarMayaDiff third_party/maya/lib/usdMaya/testenv/testUsdImportShadingModePxrRis.py
applyPixarMayaDiff third_party/maya/lib/usdMaya/testenv/testUsdMayaGetVariantSetSelections.py
applyPixarMayaDiff third_party/maya/lib/usdMaya/testenv/testUsdMayaXformStack.py
applyPixarMayaDiff third_party/maya/lib/usdMaya/testenv/testUsdReferenceAssemblyChangeRepresentations.py
applyPixarMayaDiff third_party/maya/lib/usdMaya/translatorModelAssembly.cpp
applyPixarMayaDiff third_party/maya/lib/usdMaya/translatorRfMLight.cpp
applyPixarMayaDiff third_party/maya/lib/usdMaya/translatorUtil.cpp
applyPixarMayaDiff third_party/maya/lib/usdMaya/translatorUtil.h
applyPixarMayaDiff third_party/maya/lib/usdMaya/translatorXformable.cpp
applyPixarMayaDiff third_party/maya/lib/usdMaya/util.cpp
applyPixarMayaDiff third_party/maya/lib/usdMaya/util.h
applyPixarMayaDiff third_party/maya/lib/usdMaya/wrapEditUtil.cpp
applyPixarMayaDiff third_party/maya/lib/usdMaya/writeJob.cpp
applyPixarMayaDiff third_party/maya/lib/usdMaya/writeJobContext.cpp
applyPixarMayaDiff third_party/maya/plugin/pxrUsdTranslators/CMakeLists.txt
applyPixarMayaDiff third_party/maya/plugin/pxrUsdTranslators/fileTextureWriter.cpp
applyPixarMayaDiff third_party/maya/plugin/pxrUsdTranslators/lightReader.cpp
applyPixarMayaDiff third_party/maya/plugin/pxrUsdTranslators/lightWriter.cpp

# all merges done, commit!

git commit