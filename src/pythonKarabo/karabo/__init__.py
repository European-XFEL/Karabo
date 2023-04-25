# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
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
