#!/usr/bin/env python3

#
#  NOTE
#
#  This is just a convenience script developed for internal use while two
#  external dependency systems exist for the C++ components of the Framework:
#  the legacy system based on binary packages under the "extern/resources"
#  directory and the newer one, based on a Conda Environment.
#
#  Due to its temporary nature, this script is provided as-is and no formal
#  maintenance and evolution processes should be expected.
#

import argparse
import json
import os
import shutil
import subprocess
import sys
from datetime import datetime
from pathlib import Path
from typing import Dict, Union


def git_root() -> str:
    res = subprocess.run(["git", "rev-parse", "--show-toplevel"],
                         capture_output=True)
    return res.stdout.decode("utf-8").strip()


def cmake_settings(build_type: str,
                   build_unit_tests: bool,
                   build_integration_tests: bool,
                   build_long_tests: bool) -> Dict[str, Union[str, Dict]]:

    arch = os.uname().machine
    res = subprocess.run(["lsb_release", "-is"], capture_output=True)
    distro_id = res.stdout.decode("utf-8").strip()
    res = subprocess.run(["lsb_release", "-rs"], capture_output=True)
    distro_release = res.stdout.decode("utf-8").strip()
    distro_release_major = distro_release.split(sep=".")[0]
    base_dir = git_root()
    cmake_prefix_path = (
        f"{base_dir}/extern/{distro_id}-{distro_release_major}-{arch}")
    cmake_install_prefix = (
        f"{base_dir}/package/{build_type}/{distro_id}/"
        f"{distro_release_major}/{arch}/karabo")

    if (build_type.upper() == "DEBUG" or
            build_type.upper() == "CODECOVERAGE"):
        cmake_build_type = "Debug"
    else:
        cmake_build_type = "Release"

    cmake_build_directory = f"{base_dir}/build_{cmake_build_type.lower()}"

    build_unit_testing = "1" if build_unit_tests else "0"
    build_integration_testing = "1" if build_integration_tests else "0"
    build_long_run_testing = "1" if build_long_tests else "0"
    gen_code_coverage = "1" if build_type.upper() == "CODECOVERAGE" else "0"

    cmake_config_settings = {
        "CMAKE_BUILD_TYPE": cmake_build_type,
        "CMAKE_INSTALL_PREFIX": cmake_install_prefix,
        "CMAKE_PREFIX_PATH": cmake_prefix_path,
        "BUILD_UNIT_TESTING": build_unit_testing,
        "BUILD_INTEGRATION_TESTING": build_integration_testing,
        "BUILD_LONG_RUN_TESTING": build_long_run_testing,
        "GEN_CODE_COVERAGE": gen_code_coverage
    }

    return {
        "cmake.buildDirectory": cmake_build_directory,
        "cmake.cmakePath": f"{cmake_prefix_path}/bin/cmake",
        "cmake.configureSettings": cmake_config_settings
    }


def update_cmake_settings(settings: Dict[str, str]) -> int:
    file_dir = f"{git_root()}/.vscode"
    settings_path = Path(f"{file_dir}/settings.json")
    data_to_save = {}
    file_operation = ""

    if settings_path.exists():
        # Initializes data_to_save with the current settings.
        # If the current settings cannot be read, just reports the error and
        # leave.
        with open(settings_path, "r") as settings_file:
            try:
                data_to_save = json.load(settings_file)
            except json.JSONDecodeError as je:
                print("")
                print(f"ERROR: invalid settings file found: '{settings_path}'")
                print(f"       {je}")
                print("")
                print("Setup aborted.")
                print("")
                return os.EX_OSFILE
        # Back-up the current settings file.
        now_stamp = datetime.now().isoformat()
        now_stamp = now_stamp.replace(":", "_").replace(".", "_")
        backup_path = f"{file_dir}/settings.bak.{now_stamp}.json"
        shutil.copy(settings_path, backup_path)
        print("")
        print("Settings back-up file created at:")
        print(f"    '{backup_path}'")
        file_operation = "updated"
    else:
        dir_path = Path(file_dir)
        if not dir_path.exists():
            dir_path.mkdir(parents=True, exist_ok=True)
        file_operation = "created"

    data_to_save["cmake.configureSettings"] = (
        settings["cmake.configureSettings"])
    data_to_save["cmake.buildDirectory"] = settings["cmake.buildDirectory"]
    data_to_save["cmake.cmakePath"] = settings["cmake.cmakePath"]

    with open(settings_path, "w") as settings_file:
        json.dump(data_to_save, settings_file, indent=4)

    print("")
    print(f"File '{settings_path}' {file_operation} with the settings:")
    print(json.dumps(settings, indent=4))
    print("")
    print("Please choose to 'Clean Reconfigure All Projects' before building "
          "from inside\n'Visual Studio Code'!")
    print("")

    return os.EX_OK


def setupVSCodeCMake(build_type: str,
                     build_unit_tests: bool,
                     build_integration_tests: bool,
                     build_long_tests: bool) -> int:
    """
    Updates (or initializes) the "settings.json" file of the VSCode Workspace
    for the Karabo Framework with the same CMake settings used by the
    "auto_build_all.sh" script.

    Any existing "settings.json" file for the Karabo Framework workspace will
    be backed up with the name "settings.bak.<create_timestamp>.json".
    "<create_timestamp>" is the timestamp of the creation of the backup file.

    NOTE: using the same CMake settings as the "auto_build_all.sh" implies
    switching to the current external dependency management mechanism of the
    Framework, instead of using the new mechanism which gets the external
    dependencies from a Conda Environment. The new mechanism allows builds
    started from VSCode to automatically update the external dependencies when
    there is any modification in the set of installed packages of the Conda
    environment. The current external dependency mechanism requires
    "auto_build_all.sh" to be executed if there is any change in the set of
    external dependecies - starting a build from VSCode won't update any
    external dependency.
    """
    os_type = os.uname().sysname
    if os_type.upper() == "LINUX":
        settings = cmake_settings(build_type, build_unit_tests,
                                  build_integration_tests, build_long_tests)
    else:
        print("")
        print("This script must be run on a Linux system.")
        print(f"Your system was identified as '{os_type}'.")
        print("Setup aborted.")
        print("")
        return os.EX_SOFTWARE

    return update_cmake_settings(settings)


if __name__ == "__main__":

    ap = argparse.ArgumentParser()
    ap.add_argument("build_type",
                    choices=["Debug", "Release", "CodeCoverage"],
                    nargs="?", default="Debug",
                    help="Build type (default is 'Debug')")
    ap.add_argument("--buildUnitTests", action="store_true",
                    help="Build C++ unit tests?")
    ap.add_argument("--buildIntegrationTests", action="store_true",
                    help="Build C++ integration tests?")
    ap.add_argument("--buildLongTests", action="store_true",
                    help="Build C++ long-running tests?")
    cmd_args = ap.parse_args()

    if (cmd_args.build_type == "CodeCoverage" and
            not (cmd_args.buildUnitTests or cmd_args.buildIntegrationTests or
                 cmd_args.buildLongTests)):
        print("")
        print("ERROR: CodeCoverage requires building some type of test.")
        print("")
        sys.exit(os.EX_CONFIG)

    ret_code = setupVSCodeCMake(cmd_args.build_type,
                                cmd_args.buildUnitTests,
                                cmd_args.buildIntegrationTests,
                                cmd_args.buildLongTests)

    sys.exit(ret_code)
