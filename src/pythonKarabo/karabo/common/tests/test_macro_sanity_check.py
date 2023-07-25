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
from unittest import TestCase

from karabo.common.sanity_check import validate_macro

ASYNC_UPDATE_MACRO = """
from karabo.middlelayer import Macro, Slot, String
class Macro(Macro):
    name = String()
    @Slot()
    async def update(self):
        print("Hello {}!".format(self.name))
"""

UPDATE_MACRO = """
from karabo.middlelayer import Macro, Slot, String
class Macro(Macro):
    name = String()
    @Slot()
    def update(self):
        print("Hello {}!".format(self.name))
"""

VALID_MACRO = """
from karabo.middlelayer import Macro, Slot, String
class Macro(Macro):
    name = String()
    @Slot()
    def execute(self):
        try:
            print("Hello {}!".format(self.name))
        except Exception:
            # this is bad practice! but in python 3.8 is not a problem
            # the CancelledError will not be caught by this macro.
            pass

"""

TIME_CONFLICT_MACRO = """
import time

from karabo.middlelayer import Macro, Slot, String
class Macro(Macro):
    task = None
    name = String()
    @Slot()
    def execute(self):
        if self.task is not None:
            self.task.cancel()
        print("Hello {}!".format(self.name))
"""

CANCEL_MACRO = """
from karabo.middlelayer import Macro, Slot, String
class Macro(Macro):
    name = String()
    @Slot()
    def cancel(self):
        print("Hello {}!".format(self.name))
"""

ASYNC_CANCEL_MACRO = """
from karabo.middlelayer import Macro, Slot, String
class Macro(Macro):
    name = String()
    @Slot()
    async def cancel(self):
        print("Hello {}!".format(self.name))
"""

REGISTER_MACRO = """
from karabo.middlelayer import Macro
class Macro(Macro):
    def register(self):
        pass
"""

ASYNC_REGISTER_MACRO = """
from karabo.middlelayer import Macro
class Macro(Macro):
    name = String()
    async def register(self):
        pass
"""

UNSTOPPABLE_MACRO = """
from karabo.middlelayer import Macro
class Macro(Macro):
    name = String()
    async def run(self):
        try:
            print(f"{self.name}")
        except:
            # this macro will catch the CancelledError and never stop.
            pass
"""


UNSTOPPABLE_MACRO_BASE_EXC = """
from karabo.middlelayer import Macro
class Macro(Macro):
    name = String()
    async def run(self):
        try:
            print(f"{self.name}")
        except BaseException:
            # this macro will catch the CancelledError and never stop.
            pass
"""

STOPPABLE_MACRO_CANCELLED_EXC = """
from asyncio import CancelledError
from karabo.middlelayer import Macro
class Macro(Macro):
    name = String()
    async def run(self):
        try:
            print(f"{self.name}")
        except CancelledError as e:
            print("do something")
            raise e
"""

SUB_IMPORT_MACRO = """
from karabo.middlelayer import Slot, String
from karabo.middlelayer.device_client import DeviceClientBase

class Macro(DeviceClientBase):
    name = String()
    @Slot()
    def execute(self):
        try:
            print("Hello {}!".format(self.name))
        except Exception:
            # this is bad practice! but in python 3.8 is not a problem
            # the CancelledError will not be caught by this macro.
            pass
"""


class Tests(TestCase):
    def test_fromtime(self):
        fromtime = "from time import sleep\n"
        res = validate_macro(fromtime)
        self.assertEqual(len(res), 1)

        fromtime = "\n\nfrom time import time, sleep\n"
        res = validate_macro(fromtime)
        self.assertEqual(len(res), 1)

        fromtime = "import numpy; from time import sleep\n"
        res = validate_macro(fromtime)
        self.assertEqual(len(res), 1)

    def test_importime(self):
        time = "import time\n" "time.sleep(1)\n"
        res = validate_macro(time)
        self.assertEqual(len(res), 1)

        time = "import time\n" "#time.sleep(1)\n"
        res = validate_macro(time)
        self.assertSequenceEqual([], res)

    def test_time_conflict(self):
        res = validate_macro(TIME_CONFLICT_MACRO)
        self.assertSequenceEqual([], res)

    def test_importtimeas(self):
        timeas = "import time as t\n" "t.sleep(5)\n"
        res = validate_macro(timeas)
        self.assertEqual(len(res), 1)

        timeas = "import whatever; import time as bla; bla.sleep(5)"
        res = validate_macro(timeas)
        self.assertEqual(len(res), 1)

        timeas = "import whatever\n" "import time as bla\n" "bla.sleep(5)"
        res = validate_macro(timeas)
        self.assertEqual(len(res), 1)

        timeas = (
            "from karabo.middlelayer import Macro, Slot, String\n"
            "import time as k\n"
            "k.time()\n"
        )
        res = validate_macro(timeas)
        self.assertSequenceEqual([], res)

    def test_comment(self):
        fromtime = "from time import time #, sleep\n"
        res = validate_macro(fromtime)
        self.assertSequenceEqual([], res)

        fromtime = "# from time import sleep\n"
        res = validate_macro(fromtime)
        self.assertSequenceEqual([], res)

        fromtime = "import numpy; # from time import sleep\n"
        res = validate_macro(fromtime)
        self.assertSequenceEqual([], res)

        time = "#time.sleep(1)\n"
        res = validate_macro(time)
        self.assertSequenceEqual([], res)

        time = "# time.sleep(1)\n"
        res = validate_macro(time)
        self.assertSequenceEqual([], res)

    def test_no_occurence(self):
        res = validate_macro(VALID_MACRO)
        self.assertSequenceEqual([], res)

    def test_update_function(self):
        res = validate_macro(ASYNC_UPDATE_MACRO)
        self.assertEqual(len(res), 1)
        res = validate_macro(UPDATE_MACRO)
        self.assertEqual(len(res), 1)

    def test_cancel_function(self):
        res = validate_macro(CANCEL_MACRO)
        self.assertEqual(len(res), 1)
        res = validate_macro(ASYNC_CANCEL_MACRO)
        self.assertEqual(len(res), 1)

    def test_register_function(self):
        res = validate_macro(REGISTER_MACRO)
        self.assertEqual(len(res), 1)
        res = validate_macro(ASYNC_REGISTER_MACRO)
        self.assertEqual(len(res), 1)

    def test_unstoppable_macro(self):
        res = validate_macro(UNSTOPPABLE_MACRO)
        self.assertEqual(len(res), 1, "\n".join(res))
        res = validate_macro(UNSTOPPABLE_MACRO_BASE_EXC)
        self.assertEqual(len(res), 1, "\n".join(res))
        # this code is dangerous.
        # But we will allow it for the moment
        res = validate_macro(STOPPABLE_MACRO_CANCELLED_EXC)
        self.assertEqual(len(res), 0, "\n".join(res))

    def test_sub_imports(self):
        res = validate_macro(SUB_IMPORT_MACRO)
        self.assertEqual(len(res), 1)
