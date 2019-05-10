import sys
import traceback

import maya.cmds as cmds

def mayaQuit(code):
    def doQuit():
        sys.stdout.flush()
        sys.__stdout__.flush()
        sys.stderr.flush()
        sys.__stderr__.flush()
        cmds.quit(abort=1, exitCode=code)
    # If for some reason quitting immediately doesn't work,
    # register an idleEvent callback to quit
    cmds.scriptJob(idleEvent=doQuit)
    doQuit()

try:
    def safeLoadPlugin(plugin):
        if not cmds.pluginInfo(plugin, q=1, loaded=1):
            cmds.loadPlugin(plugin)
    safeLoadPlugin("mtoh")

    # add test_src_dir to path, so we can import hdmaya_test_utils
    test_src_dir = "<PY_TEST_SRC_DIR>"
    if test_src_dir not in sys.path:
        sys.path.append(test_src_dir)

    py_script_path = "<PY_SCRIPT_PATH>"
    print("executing: {}".format(py_script_path))
    execfile(py_script_path)
except BaseException as e:
    # Want to catch EVERYTHING, including SystemExit, which may be raised by
    # unittest.main
    if isinstance(e, SystemExit):
        # e.code is sometimes boolean
        mayaQuit(int(e.code))
    else:
        traceback.print_exc()
        mayaQuit(1)
else:
    mayaQuit(0)
