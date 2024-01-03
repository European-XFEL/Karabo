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


def test_import_sub_modules():
    """Check the device package for forbidden subimports"""
    from karabo.common.api import has_sub_imports
    from karabo.middlelayer.testing import get_ast_objects
    ignore = ["karabo.middlelayer.testing"]
    ast_objects = get_ast_objects(__PACKAGE_NAME__)
    for ast_obj in ast_objects:
        assert not len(has_sub_imports(ast_obj, "karabo.middlelayer", ignore))
