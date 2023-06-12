#############################################################################
# Author: __EMAIL__
#
# Created on __DATE__
# from template '__TEMPLATE_ID__' of Karabo __KARABO_VERSION__
#
# This file is intended to be used together with Karabo:
#
# http://www.karabo.eu
#
# IF YOU REQUIRE ANY LICENSING AND COPYRIGHT TERMS, PLEASE ADD THEM HERE.
# Karabo itself is licensed under the terms of the MPL 2.0 license.
#############################################################################
import __PACKAGE_NAME__


def test_property_code_guideline():
    """Test that all properties and slots follow common code style"""
    try:
        from karabo.middlelayer.testing import check_device_package_properties
    except (ImportError, ModuleNotFoundError):
        print("Checking device properties not possible with existing "
              "karabo version.")
        return
    keys = check_device_package_properties(__PACKAGE_NAME__)
    msg = ("The key naming does not comply with our coding guidelines. "
           f"Please have a look at (class: paths): {keys}")
    assert not keys, msg
