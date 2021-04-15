from unittest import TestCase

from karabo.common.macro_sanity_check import validate_macro

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
        print("Hello {}!".format(self.name))
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
        time = ("import time\n"
                "time.sleep(1)\n")
        res = validate_macro(time)
        self.assertEqual(len(res), 1)

        time = ("import time\n"
                "#time.sleep(1)\n")
        res = validate_macro(time)
        self.assertSequenceEqual([], res)

    def test_time_conflict(self):
        res = validate_macro(TIME_CONFLICT_MACRO)
        self.assertSequenceEqual([], res)

    def test_importtimeas(self):
        timeas = ("import time as t\n"
                  "t.sleep(5)\n")
        res = validate_macro(timeas)
        self.assertEqual(len(res), 1)

        timeas = ("import whatever; import time as bla; bla.sleep(5)")
        res = validate_macro(timeas)
        self.assertEqual(len(res), 1)

        timeas = ("import whatever\n"
                  "import time as bla\n"
                  "bla.sleep(5)")
        res = validate_macro(timeas)
        self.assertEqual(len(res), 1)

        timeas = ("from karabo.middlelayer import Macro, Slot, String\n"
                  "import time as k\n"
                  "k.time()\n")
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
