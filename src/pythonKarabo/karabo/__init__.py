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
"""\
Useful commands in Karabo:
==========================

connectDevice(deviceId)
  connect to a remote device, returns a proxy to it

getDevices()
  get a list of running devices

instantiate(serverId, classId, deviceId, **kwargs)
  instantiate a remote device

shutdown(deviceId)
  shut down a remote device
"""

try:
    from karabo._version import version
    from karabo.common.packaging import utils
    __version__ = utils.extract_full_version(version)
except ImportError:
    # Don't cause a failure when running setup.py
    __version__ = ''
