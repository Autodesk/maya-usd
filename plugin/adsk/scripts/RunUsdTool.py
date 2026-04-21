import os.path
import os
import sys
import subprocess
from typing import List


def run_usd_tool():
    '''Setup the dynamic libraries paths and run the specified USD tool command'''

    if len(sys.argv) < 2:
        print('Usage: RunUsdTool.py <tool_name> [args...]')
        sys.exit(1)

    tool_name = sys.argv[1]

    # Find where the current script is located to find other files relative to it
    script_folder = os.path.dirname(__file__)

    # Find the Maya module file (.mod) to get the libraries paths
    mod_files = find_mod_files(os.path.join(script_folder, '..', '..', '..'))
    pretty_mod_files = "\n".join(['    ' + mod_file for mod_file in mod_files])
    print(f'Found Maya module files:\n{pretty_mod_files}')

    # Find all the dynamic libraries folders from the module files
    lib_folders = []
    for mod_file in mod_files:
        lib_folders.extend(find_mod_file_libraries_folders(mod_file))
    pretty_lib_folders = "\n".join(['    ' + lib_folder for lib_folder in lib_folders])
    print(f'Found dynamic libraries folders:\n{pretty_lib_folders}')

    # Update the OS environment variable to include the dynamic libraries paths
    update_os_dylib_env_var(lib_folders)

    # Run the specified tool executable with the remaining arguments
    cmd = tool_name
    args = [cmd] + sys.argv[2:]
    subprocess.run(args)


def find_mod_files(folder) -> List[str]:
    '''Find Maya plugin module file to find dynamic libraries paths'''
    mod_files = []
    for current_folder, sub_folders, sub_files in os.walk(folder):
        for sub_file in sub_files:
            if not sub_file.endswith('.mod'):
                continue
            mod_files.append(os.path.normpath(os.path.join(current_folder, sub_file)))
    return mod_files


def find_mod_file_libraries_folders(mod_file_name) -> List[str]:
    '''Find plugin installation paths and add the corresponding dynamic libraries'''
    found_lib_folders = []
    mod_file_folder = os.path.dirname(mod_file_name)

    if not os.path.exists(mod_file_name):
        print('Cannot find the MayaUSD module file.')
        return found_lib_folders

    for line in open(mod_file_name).readlines():
        if not line.startswith('+'):
            continue
        current_folder = extract_plugin_folder(line)
        
        # Construct the path to the USD dynamic libraries directory
        # Update the environment variable to include the libraries path
        for lib_folder in ['lib', 'lib64']:
            dylib_folder = os.path.normpath(os.path.join(mod_file_folder, current_folder, lib_folder))
            if os.path.exists(dylib_folder):
                found_lib_folders.append(dylib_folder)

    return found_lib_folders


def update_os_dylib_env_var(lib_folders):
    '''Update the OS environment variable to include the dynamic libraries paths'''
    env_var = get_os_dylib_env_var()
    for lib_folder in lib_folders:
        if os.path.exists(lib_folder):
            if env_var in os.environ:
                os.environ[env_var] = lib_folder + os.pathsep + os.environ[env_var]
            else:
                os.environ[env_var] = lib_folder
    print(f'Updated {env_var} to: {os.environ.get(env_var)}\n\n')


def get_os_dylib_env_var():
    '''Determine which environment variable to modify'''
    if sys.platform.startswith("win"):
        return 'PATH'
    elif sys.platform.startswith("linux"):
        return 'LD_LIBRARY_PATH'
    elif sys.platform == "darwin":
        return 'DYLD_LIBRARY_PATH'
    else:
        return 'PATH'


def extract_plugin_folder(line):
    '''Extract the Maya plugin folder from the plugin declaration'''
    return line.strip().split(' ')[-1]


run_usd_tool()
