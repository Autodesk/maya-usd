# AL_MayaTest library
AL_USDMaya (as of 0.30.1) contains a library called "AL_MayaTest", see [here](../mayatest). This provides 2 modules:
+ [testHelpers](../mayatest/AL/maya/test/testHelpers.h) - Some useful helper methods for writing your own tests
+ [testHarness](../mayatest/AL/maya/test/testHarness.h)  - some glue to allow you to easily create a maya plugin which is a "wrapper" around [googletest](https://github.com/abseil/googletest) (Note: we're currently using googletest-2.a2.0.1 internally). 

## testHarness
This module is used by AL_USDMaya's own tests, but can also be used to test your own AL_USDMaya translators, or indeed any maya code at all. All you need to do is to use write a simple plugin entry point that uses the testHarness, and compile in your test files - google test is able to find them without any additional registration scheme.

Plugins created with this module can be run in a number of ways:
+ Interactively from inside Maya
+ In batch mode (we have some convenience MEL/Shell scripts to bootstrap this)


### Usage
The plugin registers a single command which runs the tests:
```
AL_usdmaya_UnitTestHarness [flags]
```

Most of the flags are taken directly from googletest (see [here](https://github.com/abseil/googletest/blob/26743363be8f579ee7d637e5b15cbf73e9e18a4a/googletest/include/gtest/gtest.h#L100). Some have been been renamed slightly or inverted. 

(We should/could look at just having a passthrough --googleargs "googleargs here" approach rather than passing each one explicitly as a maya command argument)

| Flag | Type |   Description                      |
| -------------------------- | ------ | ----------------------------------------------------- |
| -bof-break_on_failure |    | see [googletest](https://github.com/abseil/googletest/blob/master/googletest/docs/advanced.md#turning-assertion-failures-into-break-points) |
| -f -filter | String   | see [googletest](https://github.com/abseil/googletest/blob/master/googletest/docs/advanced.md#running-a-subset-of-the-tests) e.g AL_usdmaya_UnitTestHarness -filter "translators_CameraTranslator.*"|
| -ff -flag_file  |  String  | maps to gtest_flagfile= to allow args to come from a file|
| -ktf -keep_temp_files      | bool   | remove temp files after test run |
| -nc -no_colour      |    | see [googletest](https://github.com/abseil/googletest/blob/master/googletest/docs/advanced.md#colored-terminal-output) |
| -ne -no_catch_exceptions |    |  see [googletest](https://github.com/abseil/googletest/blob/master/googletest/docs/advanced.md#disabling-catching-test-thrown-exceptions) |
|  -o -output  |  String  | This flag controls whether Google Test emits a detailed XML report to a file in addition to its normal textual output. See [googletest](https://github.com/abseil/googletest/blob/26743363be8f579ee7d637e5b15cbf73e9e18a4a/googletest/include/gtest/gtest.h#L131) |
|  -rp -repeat   |  Int   | see [googletest](https://github.com/abseil/googletest/blob/master/googletest/docs/advanced.md#repeating-the-tests) |
| -rs -random_seed       | Int   |  see [googletest](https://github.com/abseil/googletest/blob/master/googletest/docs/advanced.md#shuffling-the-tests) |
| -std -stack_trace_depth  | Int   |  This flag specifies the maximum number of stack frames to be printed in a failure message. See [googletest](https://github.com/abseil/googletest/blob/26743363be8f579ee7d637e5b15cbf73e9e18a4a/googletest/include/gtest/gtest.h#L156) |
| -tof -throw_on_failure   |    | see [googletest](https://github.com/abseil/googletest/blob/26743363be8f579ee7d637e5b15cbf73e9e18a4a/googletest/include/gtest/gtest.h#L161) |
| -l -list                 |    | list all tests (stdout) see [googletest](https://github.com/abseil/googletest/blob/master/googletest/docs/advanced.md#listing-test-names) |
| -nt                      |    | print execution time see [googletest](https://github.com/abseil/googletest/blob/master/googletest/docs/advanced.md#suppressing-the-elapsed-time)|
    

## testHelpers
contains general purpose test utility functionality that could be reused by your own  plugins or any additional tests

# AL_USDMaya tests

The AL_MayaTest library is used in a couple of places in AL_USDMaya: 
## AL_USDMayaTestPlugin
see [here](../plugin/AL_USDMayaTestPlugin/) 

This contains most of the unit tests for the core AL_USDMaya library, and uses the AL_MayaTest library to create a test plugin to run them (see [plugin](../plugin/AL_USDMayaTestPlugin/plugin.cpp))

It also contains a **run_mayaplugin_tests.mel** script which loads the plugin, and runs the test command with no args. 
You can use this to run the tests from batch mode, and additionally use the **run_mayaplugin_tests.sh** to call the same from the command line or a CI system - there are CMake targets for using with [ctest](https://cmake.org/cmake/help/latest/manual/ctest.1.html) which we use to run our automated tests



## AL_MayaUtils
see [here](../mayautils/AL/maya/tests/mayaplugintest/) 

Similar to the above
