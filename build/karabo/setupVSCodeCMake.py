#!/usr/bin/env python3

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


def _get_lsb_release_info() -> Dict[str, str]:
    resp: Dict[str, str] = {}
    if os.path.exists("/etc/os-release"):
        # /etc/os-release contains distribution and version info for both
        # Debian and RHEL based Linux distributions. Use it as the primary
        # source for distribution and version info.
        with open("/etc/os-release") as f:
            for line in f:
                if line.upper().startswith("NAME="):
                    dist_name = line[5:].replace('"', '').split()[0].strip()
                    resp["LSB_RELEASE_DIST"] = dist_name
                elif line.upper().startswith("VERSION_ID="):
                    version = line[11:].replace('"', '').split(".")[0].strip()
                    resp["LSB_RELEASE_VERSION"] = version
                if len(resp) == 2:
                    # All the keys of interest have been found.
                    break
    else:
        # Uses lsb_release command as a second option
        res = subprocess.run(["lsb_release", "-is"], capture_output=True)
        resp["LSB_RELEASE_DIST"] = res.stdout.decode("utf-8").strip()
        res = subprocess.run(["lsb_release", "-rs"], capture_output=True)
        resp["LSB_RELEASE_VERSION"] = res.stdout.decode(
            "utf-8").strip().split(sep=".")[0]
    return resp


def cmake_settings(build_type: str,
                   build_unit_tests: bool,
                   build_integration_tests: bool,
                   build_long_tests: bool) -> Dict[str, Union[str, Dict]]:

    arch = os.uname().machine
    lsb_release_info = _get_lsb_release_info()
    distro_id = lsb_release_info["LSB_RELEASE_DIST"]
    distro_release_major = lsb_release_info["LSB_RELEASE_VERSION"]
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
        "cmake.sourceDirectory": f"{base_dir}/src",
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
    data_to_save["cmake.sourceDirectory"] = settings["cmake.sourceDirectory"]
    data_to_save["cmake.buildDirectory"] = settings["cmake.buildDirectory"]
    data_to_save["cmake.cmakePath"] = settings["cmake.cmakePath"]

    # Be sure the directory that will host the settings file exists
    os.makedirs(file_dir, exist_ok=True)

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

    NOTE: The current external dependency management mechanism requires
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
    ap.add_argument("--buildUnitTests", action="store_false",
                    help="Build C++ unit tests?")
    ap.add_argument("--buildIntegrationTests", action="store_false",
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
