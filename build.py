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

def Run(cmd, logCommandOutput=True):
    """Run the specified command in a subprocess."""
    PrintInfo('Running "{cmd}"'.format(cmd=cmd))

    with open("build_log.txt", "a") as logfile:
        logfile.write(datetime.datetime.now().strftime("%Y-%m-%d %H:%M"))
        logfile.write("\n")
        logfile.write(cmd)
        logfile.write("\n")

        # Let exceptions escape from subprocess calls -- higher level
        # code will handle them.
        if logCommandOutput:
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
            with open("build_log.txt", "r") as logfile:
                Print(logfile.read())
        raise RuntimeError("Failed to run '{cmd}'\nSee {log} for more details."
                           .format(cmd=cmd, log=os.path.abspath("build_log.txt")))

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
def RunCMake(context, force, extraArgs=None):
    """Invoke CMake to configure, build, and install a library whose 
    source code is located in the current working directory."""
    # Create a directory for out-of-source builds in the build directory
    # using the name of the current working directory.
    srcDir = os.getcwd()
    instDir = (context.mayaUsdInstDir if srcDir == context.mayaUsdSrcDir
               else srcDir == context.mayaUsdSrcDir)
    buildDir = os.path.join(context.buildDir, os.path.split(srcDir)[1])
    if force and os.path.isdir(buildDir):
        shutil.rmtree(buildDir)

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
        Run('cmake '
            '-DCMAKE_INSTALL_PREFIX="{instDir}" '
            '-DCMAKE_BUILD_TYPE={variant} '
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
        Run("cmake --build . --config {variant} --target install -- {multiproc}"
            .format(variant=variant,
                    multiproc=FormatMultiProcs(context.numJobs, generator)))

############################################################
# Maya USD
def InstallMayaUSD(context, force, buildArgs):
    with CurrentWorkingDirectory(context.mayaUsdSrcDir):
        extraArgs = []

        if context.mayaLocation:
            extraArgs.append('-DMAYA_LOCATION="{mayaLocation}"'
                             .format(mayaLocation=context.mayaLocation))

        if context.pxrUsdLocation:
            extraArgs.append('-DPXR_USD_LOCATION="{pxrUsdLocation}"'
                             .format(pxrUsdLocation=context.pxrUsdLocation))

        # Add on any user-specified extra arguments.
        extraArgs += buildArgs

        RunCMake(context, force, extraArgs)

############################################################
# ArgumentParser

parser = argparse.ArgumentParser(
    formatter_class=argparse.RawDescriptionHelpFormatter)

parser.add_argument("install_dir", type=str,
                    help="Directory where project will be installed")

parser.add_argument("--generator", type=str,
                    help=("CMake generator to use when building libraries with "
                          "cmake"))

parser.add_argument("--build_dir", type=str,
                    help=("Build directory for project"
                          "(default: <install_dir>/build_dir)"))

parser.add_argument("--maya-location", type=str,
                    help="Directory where Maya is installed.")

parser.add_argument("--pxrusd-location", type=str,
                    help="Directory where Pixar USD is is installed.")

parser.add_argument("--build-debug", dest="build_debug", action="store_true",
                    help="Build in Debug mode")

parser.add_argument("--build-release", dest="build_release", action="store_true",
                    help="Build in Release mode")

parser.add_argument("--build-relwithdebug", dest="build_relwithdebug", action="store_true",
                    help="Build in RelWithDebInfo mode")

parser.add_argument("--build-args", type=str, nargs="*", default=[],
                   help=("Custom arguments to pass to build system when "
                         "building libraries"))

parser.add_argument("-j", "--jobs", type=int, default=GetCPUCount(),
                    help=("Number of build jobs to run in parallel. "
                          "(default: # of processors [{0}])"
                          .format(GetCPUCount())))

parser.add_argument("--force", type=str, action="append", dest="force_clean_build",
                    default=[],
                    help=("Force clean build."))

args = parser.parse_args()

############################################################
# InstallContext
class InstallContext:
    def __init__(self, args):

        # Assume the project's top level cmake is in the current source directory
        self.mayaUsdSrcDir = os.path.normpath(
            os.path.join(os.path.abspath(os.path.dirname(__file__))))

        # Directory where plugins and libraries  will be installed
        self.mayaUsdInstDir = os.path.abspath(args.install_dir)

        # Directory where plugins and libraries will be built
        self.buildDir = (os.path.abspath(args.build_dir) if args.build_dir
                         else os.path.join(self.mayaUsdInstDir, "build"))

        # Build type
        self.buildDebug = args.build_debug
        self.buildRelease = args.build_release
        self.buildRelWithDebug = args.build_relwithdebug

        # Forced to be built
        self.forceBuild = args.force_clean_build

        # CMake generator
        self.cmakeGenerator = args.generator

        # Number of jobs
        self.numJobs = args.jobs
        if self.numJobs <= 0:
            raise ValueError("Number of jobs must be greater than 0")

        # - Maya Location
        self.mayaLocation = (os.path.abspath(args.maya_location)
                             if args.maya_location else None)

        # - PXR USD Location
        self.pxrUsdLocation = (os.path.abspath(args.pxrusd_location)
                               if args.pxrusd_location else None)

        # Build arguments
        self.buildArgs = list()
        for args in args.build_args:
            argList = args.split(",")
            for arg in argList:
                self.buildArgs.append(arg)
try:
    context = InstallContext(args)
except Exception as e:
    PrintError(str(e))
    sys.exit(1)

# Summarize
summaryMsg = """
Building with settings:
  Source directory          {mayaUsdSrcDir}
  Install directory         {mayaUsdInstDir}
  Build directory           {buildDir}
  Variant                   {buildVariant}
  CMake generator           {cmakeGenerator}
  Build Log                 {buildDir}/build_log.txt"""

if context.buildArgs:
  summaryMsg += """
  Extra Build arguments     {buildArgs}"""

summaryMsg = summaryMsg.format(
    mayaUsdSrcDir=context.mayaUsdSrcDir,
    mayaUsdInstDir=context.mayaUsdInstDir,
    buildDir=context.buildDir,
    buildArgs=context.buildArgs,
    buildVariant=BuildVariant(context),
    cmakeGenerator=("Default" if not context.cmakeGenerator
                    else context.cmakeGenerator)
)

Print(summaryMsg)

# Install MayaUSD
InstallMayaUSD(context, context.forceBuild, context.buildArgs)

# Ensure directory structure is created and is writable.
for dir in [context.mayaUsdInstDir, context.buildDir]:
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

Print("""Success Maya USD build and install !!!!""")
