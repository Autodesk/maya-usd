import sys
import traceback

import maya.cmds as cmds

try:
    def safeLoadPlugin(plugin):
        if not cmds.pluginInfo(plugin, q=1, loaded=1):
            cmds.loadPlugin(plugin)
    safeLoadPlugin("mtoh")

    def quit(code):
        sys.stdout.flush()
        sys.__stdout__.flush()
        sys.stderr.flush()
        sys.__stderr__.flush()
        cmds.quit(abort=1, exitCode=code)

    test_name = "<PY_TEST_NAME>"
    py_script_path = "<PY_SCRIPT_PATH>"
    print("executing: {}".format(py_script_path))
    execfile(py_script_path)
except BaseException as e:
    # Want to catch EVERYTHING, including SystemExit, which may be raised by
    # unittest.main
    if isinstance(e, SystemExit):
        # e.code is sometimes boolean
        quit(int(e.code))
    else:
        traceback.print_exc()
        quit(1)
else:
    quit(0)
