#!/usr/bin/env python

'''Run a python test script inside of a maya session
'''


import argparse
import inspect
import json
import os
import platform
import subprocess
import shutil
import sys
import tempfile

from subprocess import CalledProcessError


THIS_FILE = inspect.getsourcefile(lambda: None)
THIS_DIR = os.path.dirname(THIS_FILE)


def mel_python_repr(input_str):
    '''Returns a mel-representation of the python repr of the input string'''
    # do repr, then we mel-encode it. For the mel encoding, we just worry
    # about double-qoutes (") and backslashes (\), since the python-repr
    # should have gotten rid of all other cases.
    mel_repr = repr(input_str).replace("\\", "\\\\").replace('"', '\\"')
    return '"{}"'.format(mel_repr)


def write_wrapper_scripts(test_name, scriptpath, tempdir):
    # our call chain will be:
    #   1) maya -script melwrapper
    #   2) melwrapper calls pywrapper
    #   3) pywrapper calls original pythonscript

    # We need the melwrapper because "maya -script" only accepts mel
    # We add the pywrapper because we want to add some standard code (for
    # exiting with an exitcode or not) to all scripts"

    pywrapper_template_path = os.path.join(
        THIS_DIR, "run_maya_test_pywrapper_template.py")
    with open(pywrapper_template_path, "r") as f:
        pywrapper_contents = f.read()
    py_wrapper_contents = pywrapper_contents.replace(
        '"<PY_SCRIPT_PATH>"', repr(scriptpath))
    py_wrapper_contents = py_wrapper_contents.replace(
        '"<PY_TEST_SRC_DIR>"', repr(THIS_DIR))
    py_wrapper_path = os.path.join(tempdir, "run_maya_test_pywrapper.py")
    with open(py_wrapper_path, "w") as f:
        f.write(py_wrapper_contents)

    mel_wrapper_template_path = os.path.join(
        THIS_DIR, "run_maya_test_melwrapper_template.mel")
    with open(mel_wrapper_template_path, "r") as f:
        mel_wrapper_contents = f.read()
    mel_wrapper_contents = mel_wrapper_contents.replace(
        '"<PY_WRAPPER_PATH>"', mel_python_repr(py_wrapper_path))
    mel_wrapper_path = os.path.join(tempdir, "run_maya_test_melwrapper.py")
    with open(mel_wrapper_path, "w") as f:
        f.write(mel_wrapper_contents)

    return mel_wrapper_path


def get_environ(test_script, tempdir):
    env = dict(os.environ)
    # Make things written to scriptEditor also go to stdout
    # TODO: make cross-platform
    env['MAYA_CMD_FILE_OUTPUT'] = "/dev/stdout"
    env['MAYA_APP_DIR'] = os.path.join(tempdir, "maya_app_dir")
    env['HDMAYA_TEST_TEMPDIR'] = tempdir
    env['HDMAYA_TEST_SCRIPT'] = test_script

    TEST_ENV_PREFIX = "HDMAYA_TEST_ENV_"
    # cmake / ctest may be invoked in a "build" environment which
    # some env vars which are useful for building, but may not be
    # appropriate for running a maya session in - give users
    # a chance to reset / alter vars with HDMAYA_TEST_ENV_* vars,
    # that are ONLY set for test portion, not the build portion
    for key, value in os.environ.iteritems():
        if key.startswith(TEST_ENV_PREFIX):
            overrideKey = key[len(TEST_ENV_PREFIX):]
            env[overrideKey] = value
    return env


def find_test(test_name):
    testdir = os.path.join(THIS_DIR, test_name)
    if not os.path.isdir(testdir):
        testdir = THIS_DIR
    scriptpath = os.path.join(testdir, test_name + ".py")
    if not os.path.isfile(scriptpath):
        raise ValueError("Could not find test script at: {!r}".format(scriptpath))
    return scriptpath


def run_maya_test(test_name, gui=True, maya_bin=None, keep_tempdir=False):
    if maya_bin is None:
        maya_loc = os.environ.get('MAYA_LOCATION')
        if not maya_loc:
            raise RuntimeError(
                "maya_bin not given, and MAYA_LOCATION not in the environ")
        if not os.path.isdir(maya_loc):
            raise RuntimeError(
                "maya_bin not given, and MAYA_LOCATION was not a valid "
                "directory: {}".format(maya_loc))
        exename = 'maya'
        if platform.system() == 'Windows':
            exename += '.exe'
        maya_bin = os.path.join(maya_loc, 'bin', exename)
        if not os.path.isfile(maya_bin):
            raise RuntimeError(
                "maya_bin not given, and could not find maya executable using "
                "MAYA_LOCATION as a hint: {}".format(maya_bin))
    elif not os.path.isfile(maya_bin):
        raise RuntimeError("given maya_bin not found: {}".format(maya_bin))

    scriptpath = find_test(test_name)

    tempdir = tempfile.mkdtemp("run_maya_test_" + test_name)
    if keep_tempdir:
        print "created tempdir: {}".format(tempdir)
    try:
        mel_wrapper_path = write_wrapper_scripts(test_name, scriptpath,
                                                 tempdir)
        env = get_environ(scriptpath, tempdir)
        subprocess.check_call([maya_bin, "-script", mel_wrapper_path],
                              env=env, cwd=tempdir)
    finally:
        if not keep_tempdir:
            shutil.rmtree(tempdir)


def get_parser():
    # NOTE: it is important that the first line of the description is a
    # brief sentence, usually starting with a verb (Get, Download, Run),
    # optionally followed by two newlines and a more detailed description.
    # This ensure that it is parsed by our documentation generator.
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('test', help='Name of test to run')
    parser.add_argument('--nogui', dest='gui', action='store_false',
                        help="don't run inside of a gui maya session (use "
                        "mayapy)")
    parser.add_argument('--maya-bin', help='path to the maya executable')
    parser.add_argument('--keep-tempdir', action='store_true',
                        help='whether to keep the temporary directory that '
                        'tests are run in, instead of deleting it')
    return parser


def main(argv=None):
    if argv is None:
        argv = sys.argv[1:]
    parser = get_parser()
    args = parser.parse_args(argv)
    try:
        keep_tempdir = (args.keep_tempdir
            or bool(os.environ.get('HDMAYA_TEST_KEEP_TEMPDIR')))
        run_maya_test(args.test, gui=args.gui, maya_bin=args.maya_bin,
                      keep_tempdir=keep_tempdir)
    except subprocess.CalledProcessError as e:
        print "Test {!r} failed".format(args.test)
        return e.returncode
    except Exception:
        print "Unknown error running test {!r}:".format(args.test)
        import traceback
        traceback.print_exc()
        return 987
    print "Test {!r} finished successfully!".format(args.test)
    return 0


if __name__ == '__main__':
    sys.exit(main())
