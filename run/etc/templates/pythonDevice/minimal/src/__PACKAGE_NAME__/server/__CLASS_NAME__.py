__author__ = "__EMAIL__"
__date__ = "__DATE__"
__copyright__ = """\
Copyright (c) 2010-2015 European XFEL GmbH Hamburg.
All rights reserved.
"""

from karabo.bound import runSingleDeviceServer


def main():
    # Note: One or more device class names can be passed in.
    runSingleDeviceServer('__CLASS_NAME__', serverId='__PACKAGE_NAME__-server')
