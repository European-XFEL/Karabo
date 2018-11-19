from unittest import TestCase

from ..macro_sanity_check import macro_sleep_check


class Tests(TestCase):

    def test_fromtime(self):
        fromtime = "from time import sleep\n"
        res = macro_sleep_check(fromtime)
        self.assertSequenceEqual([1], res)

        fromtime = "\n\nfrom time import time, sleep\n"
        res = macro_sleep_check(fromtime)
        self.assertSequenceEqual([3], res)

        fromtime = "import numpy; from time import sleep\n"
        res = macro_sleep_check(fromtime)
        self.assertSequenceEqual([1], res)

    def test_uppercase(self):
        fromtime = "exec('FROM TIME IMPORT SLEEP'.lower())"
        res = macro_sleep_check(fromtime)
        self.assertSequenceEqual([1], res)

    def test_importime(self):
        time = ("import time\n"
                "time.sleep(1)\n")
        res = macro_sleep_check(time)
        self.assertSequenceEqual([2], res)

        time = ("import time\n"
                "#time.sleep(1)\n")
        res = macro_sleep_check(time)
        self.assertSequenceEqual([], res)

    def test_comment(self):
        fromtime = "from time import time #, sleep\n"
        res = macro_sleep_check(fromtime)
        self.assertSequenceEqual([], res)

        fromtime = "# from time import sleep\n"
        res = macro_sleep_check(fromtime)
        self.assertSequenceEqual([], res)

        fromtime = "import numpy; # from time import sleep\n"
        res = macro_sleep_check(fromtime)
        self.assertSequenceEqual([], res)

        time = "#time.sleep(1)\n"
        res = macro_sleep_check(time)
        self.assertSequenceEqual([], res)

        time = "# time.sleep(1)\n"
        res = macro_sleep_check(time)
        self.assertSequenceEqual([], res)

    def test_no_occurence(self):
        no_occurences = ("from karabo.middlelayer import sleep\n"
                         "class NoOccurence:\n"
                         "    def execute(self):\n"
                         "        print(hello)")
        res = macro_sleep_check(no_occurences)
        self.assertSequenceEqual([], res)
