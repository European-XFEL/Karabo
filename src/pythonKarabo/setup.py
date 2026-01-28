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
# flake8: noqa
import os

from setuptools import find_packages, setup

SUBMODULE = os.environ.get("BUILD_KARABO_SUBMODULE", "").upper()

overrides = {}

if SUBMODULE == "NATIVE":
    overrides["packages"] = find_packages(include=[
        "karabo", "karabo.common*", "karabo.native*", "karabo.testing*"
    ])
    overrides["package_data"] = {
        "karabo.common.scenemodel.tests": [
            "data/*.svg", "data/inkscape/*.svg", "data/legacy/*.svg",
            "data/legacy/icon_data/*.svg"
        ],
        "karabo.testing": ["resources/*.*"],
    }

elif SUBMODULE == "MDL":
    overrides["packages"] = find_packages(include=[
        "karabo", "karabo.common*", "karabo.native*", "karabo.testing*",
        "karabo.interactive*", "karabo.middlelayer*",
        "karabo.middlelayer_devices*",
        "karabo.packaging*",
    ])
    overrides["package_data"] = {
        "karabo.common.scenemodel.tests": [
            "data/*.svg", "data/inkscape/*.svg", "data/legacy/*.svg",
            "data/legacy/icon_data/*.svg"
        ],
        "karabo.middlelayer.tests": ["*.xml"],
        "karabo.testing": ["resources/*.*"],
    }
    overrides["entry_points"] = {
        "console_scripts": [
            "karabo=karabo.interactive.karabo:main",
            "karabo-middlelayerserver=karabo.middlelayer.device_server:MiddleLayerDeviceServer.main",
            "ikarabo=karabo.interactive.ikarabo:main",
        ],
        "karabo.middlelayer_device": [
            "PropertyTest=karabo.middlelayer_devices.property_test:PropertyTest",
        ],
    }


if __name__ == "__main__":
    setup(**overrides)
