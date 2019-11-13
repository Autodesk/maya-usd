from distutils.spawn import find_executable

import argparse
import contextlib
import datetime
import distutils
import multiprocessing
import os
import platform
import re
import shlex
import shutil
import subprocess
import sys
import distutils.util

############################################################
# Helpers for printing output
verbosity = 1

def Print(msg):
    if verbosity > 0:
        print msg

def PrintWarning(warning):
    if verbosity > 0:
        print "WARNING:", warning

def PrintStatus(status):
    if verbosity >= 1:
        print "STATUS:", status

def PrintInfo(info):
    if verbosity >= 2:
        print "INFO:", info

def PrintCommandOutput(output):
    if verbosity >= 3:
        sys.stdout.write(output)

def PrintError(error):
    if verbosity >= 3 and sys.exc_info()[1] is not None:
        import traceback
        traceback.print_exc()
    print "ERROR:", error

############################################################

def Windows():
    return platform.system() == "Windows"

def Linux():
    return platform.system() == "Linux"

def MacOS():
    return platform.system() == "Darwin"

def GetCommandOutput(command):
    """Executes the specified command and returns output or None."""
    try:
        return subprocess.check_output(
            shlex.split(command), stderr=subprocess.STDOUT).strip()
    except subprocess.CalledProcessError:
        pass
    return None

def GetXcodeDeveloperDirectory():
    """Returns the active developer directory as reported by 'xcode-select -p'.
    Returns None if none is set."""
    if not MacOS():
        return None

    return GetCommandOutput("xcode-select -p")

def GetVisualStudioCompilerAndVersion():
    """Returns a tuple containing the path to the Visual Studio compiler
    and a tuple for its version, e.g. (14, 0). If the compiler is not found
    or version number cannot be determined, returns None."""
    if not Windows():
        return None

    msvcCompiler = find_executable('cl')
    if msvcCompiler:
        # VisualStudioVersion environment variable should be set by the
        # Visual Studio Command Prompt.
        match = re.search(
            "(\d+).(\d+)",
            os.environ.get("VisualStudioVersion", ""))
        if match:
            return (msvcCompiler, tuple(int(v) for v in match.groups()))
    return None

def IsVisualStudio2017OrGreater():
    VISUAL_STUDIO_2017_VERSION = (15, 0)
    msvcCompilerAndVersion = GetVisualStudioCompilerAndVersion()
    if msvcCompilerAndVersion:
        _, version = msvcCompilerAndVersion
        return version >= VISUAL_STUDIO_2017_VERSION
    return False

def GetPythonInfo():
    """Returns a tuple containing the path to the Python executable, shared
    library, and include directory corresponding to the version of Python
    currently running. Returns None if any path could not be determined. This
    function always returns None on Windows or Linux.

    This function is primarily used to determine which version of
    Python USD should link against when multiple versions are installed.
    """
    # We just skip all this on Windows. Users on Windows are unlikely to have
    # multiple copies of the same version of Python, so the problem this
    # function is intended to solve doesn't arise on that platform.
    if Windows():
        return None

    # We also skip all this on Linux. The below code gets the wrong answer on
    # certain distributions like Ubuntu, which organizes libraries based on
    # multiarch. The below code yields /usr/lib/libpython2.7.so, but
    # the library is actually in /usr/lib/x86_64-linux-gnu. Since the problem
    # this function is intended to solve primarily occurs on macOS, so it's
    # simpler to just skip this for now.
    if Linux():
        return None

    try:
        import distutils.sysconfig

        pythonExecPath = None
        pythonLibPath = None

        pythonPrefix = distutils.sysconfig.PREFIX
        if pythonPrefix:
            pythonExecPath = os.path.join(pythonPrefix, 'bin', 'python')
            pythonLibPath = os.path.join(pythonPrefix, 'lib', 'libpython2.7.dylib')

        pythonIncludeDir = distutils.sysconfig.get_python_inc()
    except:
        return None

    if pythonExecPath and pythonIncludeDir and pythonLibPath:
        # Ensure that the paths are absolute, since depending on the version of
        # Python being run and the path used to invoke it, we may have gotten a
        # relative path from distutils.sysconfig.PREFIX.
        return (
            os.path.abspath(pythonExecPath),
            os.path.abspath(pythonLibPath),
            os.path.abspath(pythonIncludeDir))

    return None

def GetCPUCount():
    try:
        return multiprocessing.cpu_count()
    except NotImplementedError:
        return 1

def Run(context, cmd):
    """Run the specified command in a subprocess."""
    PrintInfo('Running "{cmd}"'.format(cmd=cmd))

    with open(context.logFileLocation, "a") as logfile:
        logfile.write(datetime.datetime.now().strftime("%Y-%m-%d %H:%M"))
        logfile.write("\n")
        logfile.write(cmd)
        logfile.write("\n")

        # Let exceptions escape from subprocess calls -- higher level
        # code will handle them.
        if context.redirectOutstreamFile:
            p = subprocess.Popen(shlex.split(cmd), stdout=subprocess.PIPE,
                                 stderr=subprocess.STDOUT)
            while True:
                l = p.stdout.readline()
                if l != "":
                    logfile.write(l)
                    PrintCommandOutput(l)
                elif p.poll() is not None:
                    break
        else:
            p = subprocess.Popen(shlex.split(cmd))
            p.wait()

    if p.returncode != 0:
        # If verbosity >= 3, we'll have already been printing out command output
        # so no reason to print the log file again.
        if verbosity < 3:
            with open(context.logFileLocation, "r") as logfile:
                Print(logfile.read())
        raise RuntimeError("Failed to run '{cmd}'\nSee {log} for more details."
                           .format(cmd=cmd, log=os.path.abspath(context.logFileLocation)))

def BuildVariant(context): 
    if context.buildDebug:
        return "Debug"
    elif context.buildRelease:
        return "Release"
    elif context.buildRelWithDebug:
        return "RelWithDebInfo"
    return "RelWithDebInfo"

def FormatMultiProcs(numJobs, generator):
    tag = "-j"
    if generator:
        if "Visual Studio" in generator:
            tag = "/M:"
        elif "Xcode" in generator:
            tag = "-j "

    return "{tag}{procs}".format(tag=tag, procs=numJobs)

def onerror(func, path, exc_info):
    """
    If the error is due to an access error (read only file)
    add write permission and then retries.
    If the error is for another reason it re-raises the error.
    """
    import stat
    if not os.access(path, os.W_OK):
        os.chmod(path, stat.S_IWUSR)
        func(path)
    else:
        raise
############################################################
# contextmanager
@contextlib.contextmanager
def CurrentWorkingDirectory(dir):
    """Context manager that sets the current working directory to the given
    directory and resets it to the original directory when closed."""
    curdir = os.getcwd()
    os.chdir(dir)
    try:
        yield
    finally:
        os.chdir(curdir)

############################################################
# CMAKE
def RunCMake(context, extraArgs=None, stages=None):
    """Invoke CMake to configure, build, and install a library whose 
    source code is located in the current working directory."""

    srcDir = os.getcwd()
    instDir = context.instDir
    buildDir = context.buildDir

    if 'clean' in stages and os.path.isdir(buildDir):
        shutil.rmtree(buildDir, onerror=onerror)

    if 'clean' in stages and os.path.isdir(instDir):
        shutil.rmtree(instDir)

    if not os.path.isdir(buildDir):
        os.makedirs(buildDir)

    generator = context.cmakeGenerator

    # On Windows, we need to explicitly specify the generator to ensure we're
    # building a 64-bit project. (Surely there is a better way to do this?)
    # TODO: figure out exactly what "vcvarsall.bat x64" sets to force x64
    if generator is None and Windows():
        if IsVisualStudio2017OrGreater():
            generator = "Visual Studio 15 2017 Win64"
        else:
            generator = "Visual Studio 14 2015 Win64"

    if generator is not None:
        generator = '-G "{gen}"'.format(gen=generator)

    # On MacOS, enable the use of @rpath for relocatable builds.
    osx_rpath = None
    if MacOS():
        osx_rpath = "-DCMAKE_MACOSX_RPATH=ON"

    # get build variant 
    variant= BuildVariant(context)

    with CurrentWorkingDirectory(buildDir):
        # recreate build_log.txt everytime the script runs
        if os.path.isfile(context.logFileLocation):
            os.remove(context.logFileLocation)

        if 'configure' in stages:
            Run(context,
                'cmake '
                '-DCMAKE_INSTALL_PREFIX="{instDir}" '
                '-DCMAKE_BUILD_TYPE={variant} '
                '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON'
                '{osx_rpath} '
                '{generator} '
                '{extraArgs} '
                '"{srcDir}"'
                .format(instDir=instDir,
                        variant=variant,
                        srcDir=srcDir,
                        osx_rpath=(osx_rpath or ""),
                        generator=(generator or ""),
                        extraArgs=(" ".join(extraArgs) if extraArgs else "")))
 
        installArg = ""
        if 'install' in stages:
            installArg = "--target install"

        if 'build' in stages or 'install' in stages:
            Run(context, "cmake --build . --config {variant} {installArg} -- {multiproc}"
                .format(variant=variant,
                        installArg=installArg,
                        multiproc=FormatMultiProcs(context.numJobs, generator)))

def RunCTest(context, extraArgs=None):
    buildDir = context.buildDir
    variant = BuildVariant(context)
    #TODO we can't currently run tests in parallel, something to revisit.
    numJobs = 1

    with CurrentWorkingDirectory(buildDir):
        Run(context,
            'ctest '
            '--output-on-failure ' 
            '--timeout 300 '
            '-j {numJobs} '
            '-C {variant} '
            '{extraArgs} '
            .format(numJobs=numJobs,
                    variant=variant,
                    extraArgs=(" ".join(extraArgs) if extraArgs else "")))


def BuildAndInstall(context, buildArgs, stages):
    with CurrentWorkingDirectory(context.mayaUsdSrcDir):
        extraArgs = []
        stagesArgs = []
        if context.mayaLocation:
            extraArgs.append('-DMAYA_LOCATION="{mayaLocation}"'
                             .format(mayaLocation=context.mayaLocation))

        if context.pxrUsdLocation:
            extraArgs.append('-DPXR_USD_LOCATION="{pxrUsdLocation}"'
                             .format(pxrUsdLocation=context.pxrUsdLocation))

        if context.devkitLocation:
            extraArgs.append('-DMAYA_DEVKIT_LOCATION="{devkitLocation}"'
                             .format(devkitLocation=context.devkitLocation))

        extraArgs += buildArgs
        stagesArgs += stages

        RunCMake(context, extraArgs, stagesArgs)

        # Ensure directory structure is created and is writable.
        for dir in [context.workspaceDir, context.buildDir, context.instDir]:
            try:
                if os.path.isdir(dir):
                    testFile = os.path.join(dir, "canwrite")
                    open(testFile, "w").close()
                    os.remove(testFile)
                else:
                    os.makedirs(dir)
            except Exception as e:
                PrintError("Could not write to directory {dir}. Change permissions "
                           "or choose a different location to install to."
                           .format(dir=dir))
                sys.exit(1)
        Print("""Success MayaUSD build and install !!!!""")

def RunTests(context,extraArgs):
    RunCTest(context,extraArgs)
    Print("""Success running MayaUSD tests !!!!""")

############################################################
# ArgumentParser
parser = argparse.ArgumentParser(
    formatter_class=argparse.RawDescriptionHelpFormatter)

parser.add_argument("workspace_location", type=str,
                    help="Directory where the project use as a workspace to build and install plugin/libraries.")

parser.add_argument("--generator", type=str,
                    help=("CMake generator to use when building libraries with "
                          "cmake"))

parser.add_argument("-v", "--verbosity", type=int, default=verbosity,
                    help=("How much output to print while building: 0 = no "
                          "output; 1 = warnings + status; 2 = info; 3 = "
                          "command output and tracebacks (default: "
                          "%(default)s)"))

parser.add_argument("--build-location", type=str,
                    help=("Set Build directory "
                          "(default: <workspace_location>/build-location)"))

parser.add_argument("--install-location", type=str,
                    help=("Set Install directory "
                          "(default: <workspace_location>/install-location)"))

parser.add_argument("--maya-location", type=str,
                    help="Directory where Maya is installed.")

parser.add_argument("--pxrusd-location", type=str,
                    help="Directory where Pixar USD is installed.")

parser.add_argument("--devkit-location", type=str,
                    help="Directory where Maya Devkit is installed.")

parser.add_argument("--build-debug", dest="build_debug", action="store_true",
                    help="Build in Debug mode")

parser.add_argument("--build-release", dest="build_release", action="store_true",
                    help="Build in Release mode")

parser.add_argument("--build-relwithdebug", dest="build_relwithdebug", action="store_true",
                    help="Build in RelWithDebInfo mode")

parser.add_argument("--build-args", type=str, nargs="*", default=[],
                   help=("Comma-separated list of arguments passed into CMake when building libraries"))

parser.add_argument("--ctest-args", type=str, nargs="*", default=[],
                   help=("Comma-separated list of arguments passed into CTest.(e.g -VV, --output-on-failure)"))

parser.add_argument("--stages", type=str, nargs="*", default=['clean','configure','build','install'],
                   help=("Comma-separated list of stages to execute.(possible stages: clean, configure, build, install, test)"))

parser.add_argument("-j", "--jobs", type=int, default=GetCPUCount(),
                    help=("Number of build jobs to run in parallel. "
                          "(default: # of processors [{0}])"
                          .format(GetCPUCount())))

parser.add_argument("--redirect-outstream-file", type=distutils.util.strtobool, dest="redirect_outstream_file", default=True,
                    help="Redirect output stream to a file. Set this flag to false to redirect output stream to console instead.")

args = parser.parse_args()
verbosity = args.verbosity

############################################################
# InstallContext
class InstallContext:
    def __init__(self, args):

        # Assume the project's top level cmake is in the current source directory
        self.mayaUsdSrcDir = os.path.normpath(
            os.path.join(os.path.abspath(os.path.dirname(__file__))))

        # Build type
        # Must be done early, so we can call BuildVariant(self)
        self.buildDebug = args.build_debug
        self.buildRelease = args.build_release
        self.buildRelWithDebug = args.build_relwithdebug

        # Workspace directory 
        self.workspaceDir = os.path.abspath(args.workspace_location)

        # Build directory
        self.buildDir = (os.path.abspath(args.build_location) if args.build_location
                         else os.path.join(self.workspaceDir, "build", BuildVariant(self)))

        # Install directory
        self.instDir = (os.path.abspath(args.install_location) if args.install_location
                         else os.path.join(self.workspaceDir, "install", BuildVariant(self)))

        # CMake generator
        self.cmakeGenerator = args.generator

        # Number of jobs
        self.numJobs = args.jobs
        if self.numJobs <= 0:
            raise ValueError("Number of jobs must be greater than 0")

        # Maya Location
        self.mayaLocation = (os.path.abspath(args.maya_location)
                                if args.maya_location else None)

        # PXR USD Location
        self.pxrUsdLocation = (os.path.abspath(args.pxrusd_location)
                                if args.pxrusd_location else None)

        # Maya Devkit Location
        self.devkitLocation = (os.path.abspath(args.devkit_location)
                                if args.devkit_location else None)

        # Log File Name
        logFileName="build_log.txt"
        self.logFileLocation=os.path.join(self.buildDir, logFileName)

        # Build arguments
        self.buildArgs = list()
        for argList in args.build_args:
            for arg in argList.split(","):
                self.buildArgs.append(arg)

        # Stages arguments
        self.stagesArgs = list()
        for argList in args.stages:
            for arg in argList.split(","):
                self.stagesArgs.append(arg)

        # CTest arguments
        self.ctestArgs = list()
        for argList in args.ctest_args:
            for arg in argList.split(","):
                self.ctestArgs.append(arg)

        # Redirect output stream to file
        self.redirectOutstreamFile = args.redirect_outstream_file
try:
    context = InstallContext(args)
except Exception as e:
    PrintError(str(e))
    sys.exit(1)

if __name__ == "__main__":
    # Summarize
    summaryMsg = """
    Building with settings:
      Source directory          {mayaUsdSrcDir}
      Workspace directory       {workspaceDir}
      Build directory           {buildDir}
      Install directory         {instDir}
      Variant                   {buildVariant}
      CMake generator           {cmakeGenerator}"""

    if context.redirectOutstreamFile:
      summaryMsg += """
      Build Log                 {logFileLocation}"""

    if context.buildArgs:
      summaryMsg += """
      Build arguments           {buildArgs}"""

    if context.stagesArgs:
      summaryMsg += """
      Stages arguments          {stagesArgs}"""

    if context.ctestArgs:
      summaryMsg += """
      CTest arguments           {ctestArgs}"""

    summaryMsg = summaryMsg.format(
    mayaUsdSrcDir=context.mayaUsdSrcDir,
    workspaceDir=context.workspaceDir,
    buildDir=context.buildDir,
    instDir=context.instDir,
    logFileLocation=context.logFileLocation,
    buildArgs=context.buildArgs,
    stagesArgs=context.stagesArgs,
        ctestArgs=context.ctestArgs,
    buildVariant=BuildVariant(context),
    cmakeGenerator=("Default" if not context.cmakeGenerator
                    else context.cmakeGenerator)
    )

    Print(summaryMsg)

    # BuildAndInstall
    if any(stage in ['clean', 'configure', 'build', 'install'] for stage in context.stagesArgs):
        BuildAndInstall(context, context.buildArgs, context.stagesArgs)

    # Run Tests
    if 'test' in context.stagesArgs:
        RunTests(context, context.ctestArgs)

