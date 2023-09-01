# MayaHydra testing

Tests for MayaHydra are based on the [Python unittest framework](https://docs.python.org/3/library/unittest.html) and [Google's GoogleTest framework](https://google.github.io/googletest/), and run in an interactive Maya session.

The tests interact with Maya through its Python commands. MEL scripts/commands can be run by calling `maya.mel.eval("[some MEL script]")`.

There are two types of tests : Python-only tests and Python/C++ tests. GoogleTest is only used in the context of Python/C++ tests. Note that since C++ tests require a Python test to drive them, mentions of C++ tests or testing also imply usage of Python.

Complementary info specific to C++ tests can be found in the [README.md](./cpp/README.md) of the cpp subfolder.

# Running tests

You will first need to build MayaHydra in order to run tests (see [build.md](../../../../../doc/build.md)).

Then, from your build directory (e.g. `.../build/[RelWithDebInfo|Debug]`), run the following command (pick `RelWithDebInfo` or `Debug` according to your build variant) : 

```ctest -C [RelWithDebInfo|Debug] -VV --output-on-failure```

To run a specific test suite, use the `-R` option :

```ctest -C [RelWithDebInfo|Debug] -VV --output-on-failure -R [yourTestFileName]```

For example, to run testVisibility on RelWithDebInfo :

```ctest -C RelWithDebInfo -VV --output-on-failure -R testVisibility```

# MayaHydra tests folder structure

The [current folder](./) contains :
- Python test files for the Python-only tests
- Folders for the Python-only test suites (used to store content files, e.g. reference images for comparison tests)

The [cpp subfolder](./cpp) contains : 
- Python test files for the C++ tests
- C++ test files
- Folders for the C++ test suites (used to store content files, e.g. reference images for comparison tests)

The [/test/testUtils](../../../../testUtils/) folder contains :
- Python utility files for testing

# Adding tests

Looking to add a C++ test suite? See the corresponding [README.md](./cpp/README.md) in the cpp subfolder.

To add a new Python-only test suite : 

1. Create a new test[...].py file in the current folder.
2. Create a test class that derives from unittest.TestCase (doesn't need to directly inherit from it).
3. Add `test_[...]` methods for each test case in your test suite.
4. Add the following snippet at the bottom of your file :
    ```python
    if __name__ == '__main__':
        fixturesUtils.runTests(globals())
    ```
5. Add the name of your test[...].py file to the list of `TEST_SCRIPT_FILES` in the [current folder's CMakeLists.txt](./CMakeLists.txt)

Some important notes :
- By default, the mayaHydra plugin will not be loaded. An easy way to load it is to have your test class derive from `MayaHydraBaseTestCase`, which will automatically load mayaHydra when running tests. Don't forget to add the line `_file = __file__` in your class.
- After loading mayaHydra, the renderer will still be on Viewport 2.0. If you want to use another renderer, your test will have to switch to it. If you want to use HdStorm, an easy way to do it is to have your test class derive from `MayaHydraBaseTestCase` and call `self.setHdStormRenderer()`

# Image comparison

Image comparison is done using [idiff from OpenImageIO](https://openimageio.readthedocs.io/en/latest/idiff.html).

Some utilities exist to facilitate image comparison tests. Here is the simplest way to get going : 
1. Make your test class derive from `MtohTestCase`.
2. Don't forget to add the line `_file = __file__` in your class.
3. Create a folder called [TestName]Test (e.g. for testVisibility, create a folder VisibilityTest)
4. Put the images you want to use for comparison in that folder
5. Call `self.assertImagesClose`, `self.assertImagesEqual` and/or `self.assertSnapshotClose` with the file names of the images to compare with.

You will need to pass in a fail threshold for individual pixels and a fail percentage for the whole image. Other parameters are also available for you to specify. You can find more information on these parameters on the [idiff documentation page](https://openimageio.readthedocs.io/en/latest/idiff.html).

:warning: **Important note** : While there *is* an `assertSnapshotEqual` method, its use is discouraged, as renders can differ very slightly between renderer architectures.

# Utilities

Utility files are located under [/test/testUtils](../../../../testUtils/). Note that at the time of writing this (2023/08/16), many of the utils files and their contents were inherited from the USD plugin, and are not all used. Here are some of the main utilities you might find useful :

- mtohutils.py :
    - `checkForMayaUsdPlugin()` will try to load the MayaUsd plugin and return a boolean indicating if the plugin was loaded successfully. By decorating a test case with the following line, you can run the test only if the MayaUsd plugin is found and loaded : 
    ```@unittest.skipUnless(mtohUtils.checkForMayaUsdPlugin(), "Requires Maya USD Plugin.")```
    - `MayaHydraBaseTestCase` is a base class you can use for your test classes that will provide you with some useful functionality, such as automatically loading the mayaHydra plugin. It also provides a method to use the HdStorm renderer (`setHdStormRenderer`), and another to create a simple scene with a cube (`makeCubeScene`).
    - `MtohTestCase` is a class you can use as a base for your test classes that will provide you with image comparison functionality (see [Image comparison](#Image-comparison) section). Note that this class already derives from MayaHydraBaseTestCase, so it will also automatically load the mayaHydra plugin.
