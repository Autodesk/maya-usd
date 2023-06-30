# C++ Test Framework for Maya Hydra

This test framework allows to run C++ tests from an interactive maya. The [`mayaHydraCppTests` Plugin](./mayaHydraCppTestsCmd.h) provides a command called `mayaHydraCppTest` which allows to run only a subset of tests. This command can be called from Python scripts.

## mayaHydraCppTests Plugin Usage

The plugin registers a single command which runs the tests:

```
mayaHydraCppTest [flags]
```
Currently, the only flag available is `-f` (long flag version `-filter`), which takes a String and allows to filter tests. For example, to run all tests in the `CppFramework` test suite only, one would call `mayaHydraCppTest -f CppFramework.*`. To run only the `pass` test, one can call `mayaHydraCppTest -f CppFramework.pass`. By default, all tests will run and no filter will be applied.Refer to [GoogleTest documentation](https://github.com/google/googletest/blob/main/docs/advanced.md#running-a-subset-of-the-tests) for more information on filters.

Currently, the plugin can only be invoked from Python tests. See [Known defects](#known-defects) section for more details.

## Contributing to tests

Introducing a new C++ test involves:

1. Creating a new C++ test suite using GoogleTest. See [testCppFramework.cpp](./testCppFramework.cpp).
2. Creating a corresponding Python test suite that will interactively call the C++ tests from Maya. This Python script can setup the Maya scene before running the C++ tests. [testCppFramework.py](./testCppFramework.py) shows how to load the plugin and run its command.
3. Adding the Python test to `TEST_SCRIPT_FILES` in [CMakeLists.txt](../CMakeLists.txt). Notice `testCppFramework` being added as `cpp/testCppFramework.py`.

## Known defects
The `MayaHydraCppTests` plugin cannot be loaded directly from Maya because Maya's PATH environment variable currently is missing GTest's DLL path. This is a known defect that should be fixed in the future. However, the plugin does serve its main purpose: to launch Maya Hydra C++ tests from an interactive Maya via Python scripts. 

