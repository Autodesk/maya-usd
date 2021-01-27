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
import subprocess
import sys
import time


THIS_FILE = os.path.normpath(os.path.abspath(inspect.getsourcefile(lambda: None)))
THIS_DIR = os.path.dirname(THIS_FILE)
# THIS_DIR = REPO_ROOT/tests/bin
REPO_ROOT = os.path.dirname(os.path.dirname(THIS_DIR))

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


def is_git_ignored(path):
    result = subprocess.call(['git', 'check-ignore', '-q', path], cwd=REPO_ROOT)
    return not bool(result)


def run_clang_format(paths=(), verbose=False):
    if not paths:
        paths = [REPO_ROOT]
    
    files = []
    folders = []

    include_path = os.path.join(REPO_ROOT, '.clang-format-include')
    include_regex = regex_from_file(include_path)

    ignore_path = os.path.join(REPO_ROOT, '.clang-format-ignore')
    ignore_regex = regex_from_file(ignore_path, glob=True)

    # tried to parse .gitignore with regex_from_file, but it has
    # too much special git syntax. Instead just using `git ls-files`
    # as a filter...
    git_files = subprocess.check_output(['git', 'ls-files'], cwd=REPO_ROOT)
    git_files = ['./' + x for x in git_files.splitlines()]

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
        if match_path not in git_files:
            return False
        if not include_regex.search(match_path):
            return False
        if ignore_regex.search(match_path):
            return False
        return True

    # parse the initial fed-in paths, which may be files or folders
    for path in paths:
        # Get the stat, so we only do one filesystem call, instead of
        # two for os.path.isfile() + os.path.isdir()
        try:
            st_mode = os.stat(path).st_mode
            if stat.S_ISDIR(st_mode):
                folders.append(path)
            elif stat.S_ISREG(st_mode):
                # We apply filters even to fed-in paths... this is to aid
                # in use of globs on command line
                if passes_filter(path):
                    files.append(path)
        except Exception:
            print("Given path did not exist: {}".format(path))
            raise

    if folders:
        if verbose:
            print("Finding files...")
            last_update = time.time()
            num_checked = 0

        for folder in folders:
            if is_git_ignored(folder):
                continue
            for dirpath, dirnames, filenames in os.walk(folder):
                if is_git_ignored(dirpath):
                    continue
                for filename in filenames:
                    path = os.path.join(dirpath, filename)
                    if verbose:
                        num_checked += 1
                        now = time.time()
                        if now - last_update > UPDATE_INTERVAL:
                            last_update = now
                            update_status('Checked {} - Found {} - {}'.format(
                                num_checked, len(files), print_path(path)))
                    if passes_filter(path):
                        files.append(path)
        if verbose:
            post_update_print("...done finding files. Found: {}"
                .format(len(files)))

    clang_format_executable = os.environ.get('CLANG_FORMAT_EXECUTABLE',
                                             'clang-format')
    if verbose:
        print("Running clang-format on {} files...".format(len(files)))
        last_update = time.time()

    def print_path(p):
        if p.startswith(REPO_ROOT):
                p = os.path.relpath(p, REPO_ROOT)
        return p

    num_altered = 0
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
        subprocess.check_call([clang_format_executable, '-i', path])
        mtime_new = os.path.getmtime(path)
        if mtime_new != mtime_orig:
            post_update_print("File altered: {}".format(print_path(path)))
            num_altered += 1
    post_update_print("Done - altered {} files".format(num_altered))


def get_parser():
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('paths', nargs='*',
        help='Paths to run clang-format on; defaults to all files in repo')
    parser.add_argument('-v', '--verbose', action='store_true',
        help='Enable more output (ie, progress messages)')
    return parser


def main(raw_args=None):
    parser = get_parser()
    args = parser.parse_args(raw_args)
    try:
        run_clang_format(paths=args.paths, verbose=args.verbose)
    except Exception:
        import traceback
        traceback.print_exc()
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
