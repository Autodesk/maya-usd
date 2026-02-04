#!/usr/bin/env python
from __future__ import print_function

try:
   from shutil import which as find_executable
except:
   from distutils.spawn import find_executable

from glob import glob

import argparse
import contextlib
import codecs
import datetime
import multiprocessing
import os
import platform
import re
import shlex
import shutil
import subprocess
import sys
import time
import zipfile
import tarfile

############################################################
# Helpers for printing output
verbosity = 1

def Print(msg):
    if verbosity > 0:
        print(msg)

def PrintWarning(warning):
    if verbosity > 0:
        print("WARNING:", warning)

def PrintStatus(status):
    if verbosity >= 1:
        print("STATUS:", status)

def PrintInfo(info):
    if verbosity >= 2:
        print("INFO:", info)

def PrintCommandOutput(output):
    if verbosity >= 3:
        sys.stdout.write(output)

def PrintError(error):
    if verbosity >= 3 and sys.exc_info()[1] is not None:
        import traceback
        traceback.print_exc()
    print("ERROR:", error)

def strtobool(val):
    """Convert a string representation of truth to bool.

    Accepted true values are: 'y', 'yes', 't', 'true', 'on', '1'
    Accepted false values are: 'n', 'no', 'f', 'false', 'off', '0'
    Case-insensitive; raises ValueError for invalid values.
    """
    if isinstance(val, bool):
        return val
    v = str(val).strip().lower()
    if v in ("y", "yes", "t", "true", "on", "1"):
        return True
    if v in ("n", "no", "f", "false", "off", "0"):
        return False
    raise ValueError("Invalid truth value: {0}".format(val))

def Python3():
    return sys.version_info.major == 3
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

def GetGitHeadInfo(context):
    """Returns HEAD commit id and date."""
    try:
        with CurrentWorkingDirectory(context.mayaUsdSrcDir):
            commitSha = subprocess.check_output('git rev-parse HEAD', shell = True).decode()
            commitDate = subprocess.check_output('git show -s HEAD --format="%ad"', shell = True).decode()
            return commitSha, commitDate
    except Exception as exp:
        PrintError("Failed to run git commands : {exp}".format(exp=exp))
        sys.exit(1)

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
            r"(\d+)\.(\d+)",
            os.environ.get("VisualStudioVersion", ""))
        if match:
            return (msvcCompiler, tuple(int(v) for v in match.groups()))
    return None

def IsVisualStudioVersionOrGreater(desiredVersion):
    if not Windows():
        return False

    msvcCompilerAndVersion = GetVisualStudioCompilerAndVersion()
    if msvcCompilerAndVersion:
        _, version = msvcCompilerAndVersion
        return version >= desiredVersion
    return False

def IsVisualStudio2022OrGreater():
    VISUAL_STUDIO_2022_VERSION = (17, 0)
    return IsVisualStudioVersionOrGreater(VISUAL_STUDIO_2022_VERSION)

def IsVisualStudio2019OrGreater():
    VISUAL_STUDIO_2019_VERSION = (16, 0)
    return IsVisualStudioVersionOrGreater(VISUAL_STUDIO_2019_VERSION)

def IsVisualStudio2017OrGreater():
    VISUAL_STUDIO_2017_VERSION = (15, 0)
    return IsVisualStudioVersionOrGreater(VISUAL_STUDIO_2017_VERSION)

def GetCPUCount():
    try:
        return multiprocessing.cpu_count()
    except NotImplementedError:
        return 1

def Run(context, cmd):
    """Run the specified command in a subprocess."""
    PrintInfo('Running "{cmd}"'.format(cmd=cmd))

    with codecs.open(context.logFileLocation, "a", "utf-8") as logfile:
        logfile.write("#####################################################################################" + "\n")
        logfile.write("log date: " + datetime.datetime.now().strftime("%Y-%m-%d %H:%M") + "\n")
        commitID,commitData = GetGitHeadInfo(context)
        logfile.write("commit sha: " + commitID)
        logfile.write("commit date: " + commitData)
        logfile.write("#####################################################################################" + "\n")
        logfile.write("\n")
        logfile.write(cmd)
        logfile.write("\n")

        # Let exceptions escape from subprocess calls -- higher level
        # code will handle them.
        if context.redirectOutstreamFile:
            p = subprocess.Popen(shlex.split(cmd), stdout=subprocess.PIPE,
                                 stderr=subprocess.STDOUT)
            encoding = sys.stdout.encoding or "UTF-8"
            while True:
                l = p.stdout.readline().decode(encoding)
                if l != "":
                    # Avoid "UnicodeEncodeError: 'ascii' codec can't encode 
                    # character" errors by serializing utf8 byte strings.
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

def StartBuild():
    global start_time
    start_time = time.time()

def StopBuild():
    end_time = time.time()
    elapsed_seconds = end_time - start_time
    hours, remainder = divmod(elapsed_seconds, 3600)
    minutes, seconds = divmod(remainder, 60)
    print("Elapsed time: {:02}:{:02}:{:02}".format(int(hours), int(minutes), int(seconds)))

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
        if IsVisualStudio2022OrGreater():
            generator = "Visual Studio 17 2022"
        elif IsVisualStudio2019OrGreater():
            generator = "Visual Studio 16 2019"
        elif IsVisualStudio2017OrGreater():
            generator = "Visual Studio 15 2017 Win64"
        else:
            generator = "Visual Studio 14 2015 Win64"

    if generator is not None:
        generator = '-G "{gen}"'.format(gen=generator)

    if generator and 'Visual Studio' in generator and IsVisualStudio2019OrGreater():
        generator = generator + " -A x64"

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
                '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON '
                '{generator} '
                '{extraArgs} '
                '"{srcDir}"'
                .format(instDir=instDir,
                        variant=variant,
                        srcDir=srcDir,
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

def RunCTest(context, extraArgs=None, runOnlyFailed=False):
    buildDir = context.buildDir
    variant = BuildVariant(context)

    with CurrentWorkingDirectory(buildDir):
        Run(context,
            'ctest '
            '--output-on-failure ' 
            '--timeout 500 '
            '-C {variant} {rerun} '
            '{extraArgs} '
            .format(variant=variant,
                    rerun = '--rerun-failed' if runOnlyFailed else '',
                    extraArgs=(" ".join(extraArgs) if extraArgs else "")))

def RunMakeZipArchive(context):
    installDir = context.instDir
    buildDir = context.buildDir
    pkgDir = context.pkgDir
    variant = BuildVariant(context)

    # extract version from mayausd_version.info
    mayaUsdVerion = [] 
    cmakeInfoDir = os.path.join(context.mayaUsdSrcDir, 'cmake')
    filename = os.path.join(cmakeInfoDir, 'mayausd_version.info')
    with open(filename, 'r') as filehandle:
        content = filehandle.readlines()
        for current_line in content:
            digitList = re.findall(r'\d+', current_line)
            versionStr = ''.join(str(e) for e in digitList)
            mayaUsdVerion.append(versionStr)

    majorVersion = mayaUsdVerion[0]
    minorVersion = mayaUsdVerion[1]
    patchLevel   = mayaUsdVerion[2]  

    pkgName = 'MayaUsd' + '-' + majorVersion + '.' + minorVersion + '.' + patchLevel + '-' + (platform.system()) + '-' + variant
    with CurrentWorkingDirectory(buildDir):
        shutil.make_archive(pkgName, 'zip', installDir)

        # copy zip file to package directory
        if not os.path.exists(pkgDir):
            os.makedirs(pkgDir)

        for file in os.listdir(buildDir):
            if file.endswith(".zip"):
                zipFile = os.path.join(buildDir, file)
                try:
                    shutil.copy(zipFile, pkgDir)
                except Exception as exp:
                    PrintError("Failed to write to directory {pkgDir} : {exp}".format(pkgDir=pkgDir,exp=exp))
                    sys.exit(1)

def SetupMayaQt(context):
    def haveQtHeaders(rootPath):
        if os.path.exists(rootPath):
            # MayaUsd uses these components from Qt (so at a minimum we must find them).
            qtComponentsToFind = ['QtCore', 'QtGui', 'QtWidgets']
            # Qt6 includes the entire Qt in a single zip file, which when extracted ends in folder 'Qt'.
            startDir = os.path.join(rootPath, 'Qt', 'include') if os.path.exists(os.path.join(rootPath, 'Qt')) else os.path.join(rootPath, 'include')
            for root,dirs,files in os.walk(startDir):
                if 'qt' not in root.lower() or not files:
                    continue
                if not any(root.endswith(qtComp) for qtComp in qtComponentsToFind):
                    # Skip any folders that aren't the components we are looking for.
                    continue

                for qtComp in qtComponentsToFind[:]:    # Loop over slice copy as we remove items
                    if qtComp in root and '{comp}version.h'.format(comp=qtComp.lower()) in files:
                        qtComponentsToFind.remove(qtComp)
                        PrintInfo('Found {comp} in {dir}'.format(comp=qtComp, dir=root))
                        break   # Once we've found (and removed) a component, move to the next os.walk

                if not qtComponentsToFind:  # Once we've found them all, we are done.
                    return True

    def safeTarfileExtract(members):
        """Use a function to look for bad paths in the tarfile archive to fix
        security/bandit B202: tarfile_unsafe_members."""

        def isBadPath(path, base):
            return not os.path.realpath(os.path.abspath(os.path.join(base, path))).startswith(base)
        def isBadLink(info, base):
            # Links are interpreted relative to the directory containing the link.
            tip = os.path.realpath(os.path.abspath(os.path.join(base, os.path.dirname(info.name))))
            return isBadPath(info.linkname, base=tip)

        base = os.path.realpath(os.path.abspath('.'))
        result = []
        for finfo in members:
            # If any bad paths for links are found in the tarfile, print an error
            # and don't extract anything from tarfile.
            if isBadPath(finfo.name, base):
                PrintError('Found illegal path {path} in tarfile, blocking tarfile extraction.'.format(path=finfo.name))
                return []
            elif (finfo.issym() or finfo.islnk()) and isBadLink(finfo, base):
                PrintError('Found illegal link {link} in tarfile, blocking tarfile extraction.'.format(link=finfo.linkname))
                return []
            else:
                result.append(finfo)
        return result

    def safeZipfileExtract(zip_file, extract_path='.'):
        with zipfile.ZipFile(zip_file, 'r') as zf:
            for member in zf.infolist():
                file_path = os.path.realpath(os.path.join(extract_path, member.filename))
                if file_path.startswith(os.path.realpath(extract_path)):
                    zf.extract(member, extract_path)

    # The list of directories (in order) that we'll search. This list matches the one
    # in FindMayaQt.cmake.
    dirsToSearch = [context.devkitLocation]
    if 'MAYA_DEVKIT_LOCATION' in os.environ:
        dirsToSearch.append(os.path.expandvars('$MAYA_DEVKIT_LOCATION'))
    dirsToSearch.append(context.mayaLocation)
    if 'MAYA_LOCATION' in os.environ:
        dirsToSearch.append(os.path.expandvars('$MAYA_LOCATION'))

    # Check if the Qt zip file has been extracted (we need the Qt headers).
    for dirToSearch in dirsToSearch:
        if haveQtHeaders(dirToSearch):
            PrintStatus('Found Maya Qt headers in: {dir}'.format(dir=dirToSearch))
            return

    # Qt5
    # Didn't find Qt headers, so try and extract qt_5.x-include.zip (the first one we find).
    qtIncludeArchiveName = 'qt_5*-include.zip' if Windows() else 'qt_5*-include.tar.gz'
    for dirToSearch in dirsToSearch:
        qtZipFiles = glob(os.path.join(dirToSearch, 'include', qtIncludeArchiveName))
        if qtZipFiles:
            qtZipFile = qtZipFiles[0]
            baseDir = os.path.dirname(qtZipFile)
            if os.access(baseDir, os.W_OK):
                qtZipDirName = os.path.basename(qtZipFile)
                qtZipDirName = qtZipDirName.replace('.zip', '').replace('.tar.gz', '')
                qtZipDirFolder = os.path.join(baseDir, qtZipDirName)
                PrintStatus("Could not find Maya Qt headers.")
                PrintStatus("  Extracting '{zip}' to '{dir}'".format(zip=qtZipFile, dir=qtZipDirFolder))
                if not os.path.exists(qtZipDirFolder):
                    os.makedirs(os.path.join(baseDir, qtZipDirFolder))
                try:
                    # We only need certain Qt components so we only extract the headers for those.
                    if Windows():
                        zipArchive = zipfile.ZipFile(qtZipFile, mode='r')
                        files = [n for n in zipArchive.namelist()
                            if (n.startswith('QtCore/') or n.startswith('QtGui/') or n.startswith('QtWidgets/'))]
                        zipArchive.extractall(qtZipDirFolder, files)
                        zipArchive.close()
                    else:
                        tarArchive = tarfile.open(qtZipFile, mode='r')
                        files = [n for n in tarArchive.getmembers()
                            if (n.name.startswith('./QtCore/') or n.name.startswith('./QtGui/') or n.name.startswith('./QtWidgets/'))]
                        tarArchive.extractall(qtZipDirFolder, members=safeTarfileExtract(files))
                        tarArchive.close()
                except zipfile.BadZipfile as error:
                    PrintError(str(error))
                except tarfile.TarError as error:
                    PrintError(str(error))

            # We found and extracted the Qt5 include zip - we are done.
            return

    # Qt6
    # The entire Qt is in a single zip file, which we extract to 'Qt'.
    # Then we can simply use find_package(Qt6) on it.
    for dirToSearch in dirsToSearch:
        # Oct 2024:
        # Qt archive was originally named Qt.tar.gz on all platforms.
        # Was eventually renamed to Qt.zip (Windows) and Qt.tgz (Linux/Osx).
        qtArchiveNames = ['Qt.zip', 'Qt.tar.gz'] if Windows() else ['Qt.tgz', 'Qt.tar.gz']
        for qtArchiveName in qtArchiveNames:
            qtArchive = os.path.join(dirToSearch, qtArchiveName)
            if os.path.exists(qtArchive):
                ext = os.path.splitext(qtArchiveName)[1]
                qtZipDirFolder = os.path.dirname(qtArchive)
                if os.access(qtZipDirFolder, os.W_OK):
                    PrintStatus("Could not find Maya Qt6.")
                    PrintStatus("  Extracting '{zip}' to '{dir}'".format(zip=qtArchive, dir=qtZipDirFolder))
                    try:
                        if ext == '.zip':
                            safeZipfileExtract(qtArchive, qtZipDirFolder)
                        else:
                            archive = tarfile.open(qtArchive, mode='r')
                            archive.extractall(qtZipDirFolder, members=safeTarfileExtract(archive.getmembers()))
                            archive.close()
                    except zipfile.BadZipfile as error:
                        PrintError(str(error))
                    except tarfile.TarError as error:
                        PrintError(str(error))
                    return

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

        if context.materialxEnabled and context.pxrUsdLocation:
            extraArgs.append('-DCMAKE_WANT_MATERIALX_BUILD=ON')
            extraArgs.append('-DCMAKE_PREFIX_PATH="{materialxLocation}"'
                             .format(materialxLocation=context.pxrUsdLocation))
        else:
            extraArgs.append('-DCMAKE_WANT_MATERIALX_BUILD=OFF')

        # Many people on Windows may not have python with the 
        # debugging symbol (python27_d.lib) installed, this is the common case.
        if context.buildDebug and context.debugPython:
            extraArgs.append('-DMAYAUSD_DEFINE_DEBUG_PYTHON_FLAG=ON')
        else:
            extraArgs.append('-DMAYAUSD_DEFINE_DEBUG_PYTHON_FLAG=OFF')

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
        Print('Success MayaUsd build and install!')

def RunTests(context, extraArgs):
    # Clean out any previous ctest logs before running ctest.
    lastTestLog = os.path.join(context.buildDir, 'Testing', 'Temporary','LastTest.log')
    if os.path.exists(lastTestLog):
        os.remove(lastTestLog)
    lastTestsFailedLog = os.path.join(context.buildDir, 'Testing', 'Temporary', 'LastTestsFailed.log')
    if os.path.exists(lastTestsFailedLog):
        os.remove(lastTestsFailedLog)

    try:
        RunCTest(context, extraArgs)
    except RuntimeError as e:
        PrintWarning('One (or more) tests failed, re-running only failed tests.\n')
        try:
            RunCTest(context, extraArgs, runOnlyFailed=True)
        except:
            PrintWarning('One (or more) tests failed on re-run!\n')
            raise e # Re-raise the original failure.
        else:
            Print('Successfully re-ran failed tests!')

    Print('Success running MayaUsd tests!')

def Package(context):
    RunMakeZipArchive(context)
    Print('Success packaging MayaUsd!')
    Print('Archived package is available in {pkgDir}'.format(pkgDir=context.pkgDir))

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

parser.add_argument("--materialx", dest="build_materialx", action="store_true", default=True,
                    help="Build with MaterialX features enabled (default). Requires USD built with MaterialX support.")
parser.add_argument("--no-materialx", dest="build_materialx", action="store_false",
                    help="Do not build MaterialX support in MayaUsd.")

varGroup = parser.add_mutually_exclusive_group()
varGroup.add_argument("--build-debug", dest="build_debug", action="store_true",
                    help="Build in Debug mode (default: %(default)s)")

varGroup.add_argument("--build-release", dest="build_release", action="store_true",
                    help="Build in Release mode (default: %(default)s)")

varGroup.add_argument("--build-relwithdebug", dest="build_relwithdebug", action="store_true", default=True,
                    help="Build in RelWithDebInfo mode (default: %(default)s)")

parser.add_argument("--debug-python", dest="debug_python", action="store_true",
                      help="Define Python Debug if your Python library comes with Debugging symbols (default: %(default)s).")

parser.add_argument("--qt-location", type=str,
                    help="DEPRECATED: Qt is found automatically in Maya devkit.")

parser.add_argument("--build-args", type=str, nargs="*", default=[],
                   help=("Comma-separated list of arguments passed into CMake when building libraries"))

parser.add_argument("--ctest-args", type=str, nargs="*", default=[],
                   help=("Comma-separated list of arguments passed into CTest.(e.g -VV, --output-on-failure)"))

parser.add_argument("--stages", type=str, nargs="*", default=['clean','configure','build','install'],
                   help=("Comma-separated list of stages to execute. Possible stages: clean, configure, build, install, test, package."))

parser.add_argument("-j", "--jobs", type=int, default=GetCPUCount(),
                    help=("Number of build jobs to run in parallel. "
                          "(default: # of processors [{0}])"
                          .format(GetCPUCount())))

parser.add_argument("--redirect-outstream-file", type=strtobool, dest="redirect_outstream_file", default=True,
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

        self.debugPython = args.debug_python

        # Workspace directory 
        self.workspaceDir = os.path.abspath(args.workspace_location)

        # Build directory
        self.buildDir = (os.path.abspath(args.build_location) if args.build_location
                         else os.path.join(self.workspaceDir, "build", BuildVariant(self)))

        # Install directory
        self.instDir = (os.path.abspath(args.install_location) if args.install_location
                         else os.path.join(self.workspaceDir, "install", BuildVariant(self)))

        # Package directory
        self.pkgDir = (os.path.join(self.workspaceDir, "package", BuildVariant(self)))

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

        # DEPRECATED: Qt Location
        if args.qt_location:
            PrintWarning("--qt-location flag is deprecated as Qt is found automatically in Maya devkit.")

        # MaterialX
        self.materialxEnabled = args.build_materialx

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
      Python Debug              {debugPython}
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
        debugPython=("On" if context.debugPython else "Off"),
        cmakeGenerator=("Default" if not context.cmakeGenerator
                        else context.cmakeGenerator)
    )

    Print(summaryMsg)

    # Make sure Qt from Maya devkit is ready.
    if 'configure' in context.stagesArgs:
        # If not building the MayaUsd library we don't need Qt.
        if '-DBUILD_MAYAUSD_LIBRARY=OFF' not in context.buildArgs:
            SetupMayaQt(context)

    # BuildAndInstall
    if any(stage in ['clean', 'configure', 'build', 'install'] for stage in context.stagesArgs):
        StartBuild()
        BuildAndInstall(context, context.buildArgs, context.stagesArgs)
        StopBuild()

    # Run Tests
    if 'test' in context.stagesArgs:
        RunTests(context, context.ctestArgs)

    # Package
    if 'package' in context.stagesArgs:
        Package(context)
