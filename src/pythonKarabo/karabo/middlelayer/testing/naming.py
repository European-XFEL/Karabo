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
import inspect
import re
import sys

from karabo.middlelayer import Configurable, Device

PROPERTY_REGEX = re.compile(r"^[a-z][a-zA-Z0-9]+$")


def check_device_package_properties(package):
    """Test that all properties and slots follow common code style

    :param package: the package containing `Configurable` classes

    :returns: Dictionary with classes as keys and properties paths as values
    """
    base_paths = Device.getClassSchema().hash.paths()
    keys = {}

    def _verify_object_keys(configurable, existing=[]):
        schema = configurable.getClassSchema()
        for key in schema.hash.paths():
            if existing and key in existing:
                continue
            if not PROPERTY_REGEX.match(key):
                properties = keys.setdefault(name, set())
                properties.add(key)

    for name, obj in inspect.getmembers(sys.modules[package.__name__]):
        if not inspect.isclass(obj):
            continue
        if issubclass(obj, Device):
            # Device can have base properties that have `__KEY__`
            _verify_object_keys(obj, existing=base_paths)
        elif issubclass(obj, Configurable):
            _verify_object_keys(obj)

    return keys
