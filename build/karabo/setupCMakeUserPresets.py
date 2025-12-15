#!/usr/bin/env python3
# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.

import argparse
import json
import os
import psutil   # Python installation bundled with the Framework has psutil
import shutil
import subprocess
import sys
from datetime import datetime
from pathlib import Path
from typing import Dict, List, Union


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


def cmake_user_presets(build_type: str,
                       skip_build_unit_tests: bool,
                       skip_build_integration_tests: bool,
                       build_long_tests: bool) -> Dict[str,
                                                       Union[str, Dict, List]]:

    arch = os.uname().machine
    lsb_release_info = _get_lsb_release_info()
    distro_id = lsb_release_info["LSB_RELEASE_DIST"]
    distro_release_major = lsb_release_info["LSB_RELEASE_VERSION"]

    physical_cores = psutil.cpu_count(logical=False) or 1
    total_memory_gb = psutil.virtual_memory().total/(1024**3)
    parallel_level = physical_cores
    if total_memory_gb / physical_cores < 2.0:
        # From experience, when the ratio of physical memory per building job
        # goes below 2 GB/job, the performance of the building system can be
        # compromised.
        parallel_level = physical_cores // 2

    cmake_prefix_path = (
        f"${{sourceDir}}/extern/{distro_id}-{distro_release_major}-{arch}")
    cmake_install_dir = (
        f"${{sourceDir}}/package/{build_type}/{distro_id}/"
        f"{distro_release_major}/{arch}/karabo")
    cmake_toolchain_file = (
        f"${{sourceDir}}/extern/{distro_id}-{distro_release_major}-{arch}/"
        "/conan_toolchain/conan_toolchain.cmake")

    if (build_type.upper() == "DEBUG" or
            build_type.upper() == "CODECOVERAGE"):
        cmake_build_type = "Debug"
    else:
        cmake_build_type = "Release"

    build_unit_testing = "0" if skip_build_unit_tests else "1"
    build_integration_testing = "0" if skip_build_integration_tests else "1"
    build_long_run_testing = "1" if build_long_tests else "0"
    gen_code_coverage = "1" if build_type.upper() == "CODECOVERAGE" else "0"

    # NOTE: Please keep in sync with the arguments passed to cmake configure
    # step in auto_build_all.sh:
    cmake_user_presets = {
        "version": 3,  # CLion as of 2025.2.5 can't deal with newer versions
        "cmakeMinimumRequired": {
            "major": 3,
            "minor": 15,
            "patch": 0
        },
        "configurePresets": [
            {
                "name": f"{cmake_build_type}",
                "binaryDir": "${sourceDir}/build_debug",
                "cacheVariables": {
                    "CMAKE_BUILD_TYPE": f"{cmake_build_type}",
                    "CMAKE_PREFIX_PATH": f"{cmake_prefix_path}",
                    "BUILD_UNIT_TESTING": f"{build_unit_testing}",
                    "BUILD_INTEGRATION_TESTING": (
                        f"{build_integration_testing}"),
                    "BUILD_LONG_RUN_TESTING": f"{build_long_run_testing}",
                    "GEN_CODE_COVERAGE": f"{gen_code_coverage}",
                    "CMAKE_MAP_IMPORTED_CONFIG_DEBUG": "Release",
                    "CMAKE_EXPORT_COMPILE_COMMANDS": "1",
                },
                "cmakeExecutable": (
                    "${sourceDir}/extern/"
                    f"{distro_id}-{distro_release_major}-{arch}/bin/cmake"),
                "description": (
                    "Debug build of the Karabo C++ Framework with unit and "
                    "integration tests"),
                "displayName": "Debug Build",
                "environment": {
                    "CMAKE_BUILD_PARALLEL_LEVEL": f"{parallel_level}"
                },
                "installDir": f"{cmake_install_dir}",
                "toolchainFile": f"{cmake_toolchain_file}"
            }
        ],
    }

    # Synchronizes with the behavior of auto_build_all.sh to use Ninja as the
    # generator if Ninja is available.
    if shutil.which("ninja") is not None:
        cmake_user_presets["configurePresets"][0]["generator"] = "Ninja"

    return cmake_user_presets


def update_cmake_user_presets(
        user_presets: Dict[str, Union[str, Dict, List]]) -> int:
    file_dir = f"{git_root()}"
    file_path = Path(f"{file_dir}/CMakeUserPresets.json")

    if file_path.exists():
        # Back-up the current settings file.
        now_stamp = datetime.now().isoformat()
        now_stamp = now_stamp.replace(":", "_").replace(".", "_")
        backup_path = f"{file_dir}/CMakeUserPresets.{now_stamp}.json"
        shutil.copy(file_path, backup_path)
        print("")
        print(f"{file_path} back-up file created at:")
        print(f"    '{backup_path}'")

    with open(file_path, "w") as presets_file:
        json.dump(user_presets, presets_file, indent=4)

    print("")
    print(f"File '{file_path}' created")
    print("")
    print("")

    return os.EX_OK


def setupCMakeUserPresets(build_type: str,
                          skip_build_unit_tests: bool,
                          skip_build_integration_tests: bool,
                          build_long_tests: bool) -> int:
    """
    Updates (or initializes) the "CMakeUserPresets.json" file for the
    configuration of the Framework project with the same settings used by the
    "auto_build_all.sh" script.

    Any existing "CMakeUserPresets.json" file for the Karabo Framework project
    will be backed up with the name "CMakeUserPresets.<create_timestamp>.json".
    "<create_timestamp>" is the timestamp of the creation of the backup file.

    NOTE: The current external dependency management mechanism requires
    "auto_build_all.sh" to be executed if there is any change in the set of
    external dependencies - starting a build from an IDE won't update any
    external dependency.
    """
    os_type = os.uname().sysname
    if os_type.upper() == "LINUX":
        settings = cmake_user_presets(build_type, skip_build_unit_tests,
                                      skip_build_integration_tests,
                                      build_long_tests)
    else:
        print("")
        print("This script must be run on a Linux system.")
        print(f"Your system was identified as '{os_type}'.")
        print("Setup aborted.")
        print("")
        return os.EX_SOFTWARE

    return update_cmake_user_presets(settings)


if __name__ == "__main__":

    ap = argparse.ArgumentParser()
    ap.add_argument("build_type",
                    choices=["Debug", "Release", "CodeCoverage"],
                    nargs="?", default="Debug",
                    help="Build type (default is 'Debug')")
    ap.add_argument("--skipBuildUnitTests", action="store_true",
                    help="Skip building of C++ unit tests? "
                    "(default is to build)")
    ap.add_argument("--skipBuildIntegrationTests", action="store_true",
                    help="Skip building of C++ integration tests? "
                    "(default is to build)")
    ap.add_argument("--buildLongTests", action="store_true",
                    help="Build C++ long-running tests? (default is to skip)")
    cmd_args = ap.parse_args()

    no_test_build = (cmd_args.skipBuildUnitTests and
                     cmd_args.skipBuildIntegrationTests and
                     not cmd_args.buildLongTests)
    if cmd_args.build_type == "CodeCoverage" and no_test_build:
        print("")
        print("ERROR: CodeCoverage requires building some type of test.")
        print("")
        sys.exit(os.EX_CONFIG)

    ret_code = setupCMakeUserPresets(cmd_args.build_type,
                                     cmd_args.skipBuildUnitTests,
                                     cmd_args.skipBuildIntegrationTests,
                                     cmd_args.buildLongTests)

    sys.exit(ret_code)
