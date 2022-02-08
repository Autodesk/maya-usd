#!/usr/bin/env python

'''Run clang-format on files in this repository

By default, runs on all files, but may pass specific files.
'''

from __future__ import absolute_import, division, print_function, unicode_literals

import argparse
import inspect
import fnmatch
import io
import os
import re
import stat
import sys
import platform
import time

from subprocess import check_call, check_output

if sys.version_info[0] < 3:
    # Python-2 check_output doesn't have encoding
    def check_output(*args, **kwargs):
        import subprocess
        kwargs.pop('encoding')
        return subprocess.check_output(*args, **kwargs)


THIS_FILE = os.path.normpath(os.path.abspath(inspect.getsourcefile(lambda: None)))
THIS_DIR = os.path.dirname(THIS_FILE)
# THIS_DIR = REPO_ROOT/.github
REPO_ROOT = os.path.dirname(THIS_DIR)

UPDATE_INTERVAL = .2


_last_update_len = 0
_on_update_line = False


def update_status(text):
    global _last_update_len
    global _on_update_line
    sys.stdout.write('\r')
    text_len = len(text)
    extra_chars = _last_update_len - text_len
    if extra_chars > 0:
        text += (' ' * extra_chars)
    sys.stdout.write(text)
    _last_update_len = text_len
    sys.stdout.flush()
    _on_update_line = True


def post_update_print(text):
    global _on_update_line
    if _on_update_line:
        print()
    print(text)
    _on_update_line = False


def regex_from_file(path, glob=False):
    with io.open(path, 'r') as f:
        patterns = f.read().splitlines()
    # ignore comment lines
    patterns = [x for x in patterns if x.strip() and not x.lstrip().startswith('#')]
    if glob:
        patterns = [fnmatch.translate(x) for x in patterns]
    regex = '({})'.format('|'.join(patterns))
    return re.compile(regex)

if platform.system() == "Windows" and sys.version_info >= (3, 6):
    import pathlib  # Python 3.6 is required for pathlib.Path
    def canonicalpath(path):
        path = os.path.abspath(os.path.realpath(os.path.normpath(os.path.normcase(path))))
        realpath = str(pathlib.Path(path).resolve())  # To get a properly cased path ie: from r'C:\WiNdOwS\SyStEm32\DeSkToP.iNi' get r'C:\Windows\System32\desktop.ini'
        if len(path) == len(realpath): # This is to avoid following symbolic links, there is still the possibility that they could be equal.
            path = realpath
        if os.path.isabs(path) and path[0].upper() != path[0]:
            path = path[0].upper() +path[1:] # path.capitalize()
        path = path.replace('\\', '/')
        return path
else:
    def canonicalpath(path):
        path = os.path.abspath(os.path.realpath(os.path.normpath(os.path.normcase(path))))
        return path.replace('\\', '/')

def run_clang_format(paths=(), verbose=False, commit=None):
    """Runs clang-format in-place on repo files

    Returns
    -------
    List[str]
        Files altered by clang-format
    """
    if not paths and not commit:
        paths = [REPO_ROOT]

    if commit:
        check_call(['git', 'checkout', commit], cwd=REPO_ROOT)
        text = check_output(
            ['git', 'diff-tree', '--no-commit-id', '--name-only', '-r',
             commit], cwd=REPO_ROOT, encoding=sys.stdout.encoding)
        commit_paths = text.splitlines()
        paths.extend(os.path.join(REPO_ROOT, p) for p in commit_paths)
    
    files = set()
    folders = set()

    include_path = os.path.join(REPO_ROOT, '.clang-format-include')
    include_regex = regex_from_file(include_path)

    ignore_path = os.path.join(REPO_ROOT, '.clang-format-ignore')
    ignore_regex = regex_from_file(ignore_path, glob=True)

    # tried to parse .gitignore with regex_from_file, but it has
    # too much special git syntax. Instead just using `git ls-files`
    # as a filter...
    git_files = check_output(['git', 'ls-files'], cwd=REPO_ROOT,
        encoding=sys.stdout.encoding)
    git_files = set(canonicalpath(x.strip()) for x in git_files.splitlines())

    def print_path(p):
        if p.startswith(REPO_ROOT):
                p = os.path.relpath(p, REPO_ROOT)
        return p

    def passes_filter(path):
        rel_path = os.path.relpath(path, REPO_ROOT)
        match_path = os.path.join('.', rel_path)
        # standardize on '/', because that's what's used in files,
        # and output by `git ls-files`
        match_path = match_path.replace('\\', '/')
        if not include_regex.search(match_path):
            return False
        if ignore_regex.pattern != '()' and ignore_regex.search(match_path):
            return False
        return True

    # parse the initial fed-in paths, which may be files or folders
    for path in paths:
        # Get the stat, so we only do one filesystem call, instead of
        # two for os.path.isfile() + os.path.isdir()
        try:
            st_mode = os.stat(path).st_mode
            if stat.S_ISDIR(st_mode):
                folders.add(path)
            elif stat.S_ISREG(st_mode):
                if canonicalpath(path) in git_files:
                    files.add(path)
        except Exception:
            print("Given path did not exist: {}".format(path))
            raise

    for folder in folders:
        # we already have list of potential files in git_files...
        # filter to ones in this folder
        folder = canonicalpath(folder) + '/'
        files.update(x for x in git_files if x.startswith(folder))

    # We apply filters even to fed-in paths... this is to aid
    # in use of globs on command line
    files = sorted(x for x in files if passes_filter(x))

    clang_format_executable = os.environ.get('CLANG_FORMAT_EXECUTABLE',
                                             'clang-format')
    if verbose:
        print("Running clang-format on {} files...".format(len(files)))
        last_update = time.time()

    def print_path(p):
        if p.startswith(REPO_ROOT):
                p = os.path.relpath(p, REPO_ROOT)
        return p

    altered = []
    for i, path in enumerate(files):
        if verbose:
            now = time.time()
            if now - last_update > UPDATE_INTERVAL:
                last_update = now
                update_status('File {}/{} ({:.1f}%) - {}'.format(
                            i + 1, len(files), (i + 1) / len(files) * 100,
                            print_path(path)))
        # Note - couldn't find a way to get clang-format to return whether
        # or not a file was altered with '-i' - so checking ourselves
        # Checking mtime is not foolproof, but is faster than reading file
        # and comparing, and probably good enough
        mtime_orig = os.path.getmtime(path)
        check_call([clang_format_executable, '-i', path])
        mtime_new = os.path.getmtime(path)
        if mtime_new != mtime_orig:
            post_update_print("File altered: {}".format(print_path(path)))
            altered.append(path)
    post_update_print("Done - altered {} files".format(len(altered)))
    return altered


def get_parser():
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('paths', nargs='*',
        help='Paths to run clang-format on; defaults to all files in repo')
    parser.add_argument('-v', '--verbose', action='store_true',
        help='Enable more output (ie, progress messages)')
    parser.add_argument('-c', '--commit',
        help='Git commit / revision / branch; will first check out that commit,'
            " then query it for it's list of affected files, to use as the files"
            ' to run clang-format on; if PATHS are also manually given, they are'
            ' appended')
    return parser


def main(raw_args=None):
    parser = get_parser()
    args = parser.parse_args(raw_args)
    try:
        altered = run_clang_format(paths=args.paths, verbose=args.verbose,
            commit=args.commit)
    except Exception:
        import traceback
        traceback.print_exc()
        return 1
    if altered:
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
