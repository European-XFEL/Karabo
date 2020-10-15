"""
Test the middle layer code against PEP8

There are two exception lists which list the number of expected
warnings per file. Update as needed.
"""
import os
from unittest import TestCase, main
import sys

from pycodestyle import Checker

from karabo import middlelayer

pep8_exceptions = {
}


class Tests(TestCase):
    @classmethod
    def setUpClass(cls):
        mods = (obj.__module__ for obj in middlelayer.__dict__.values()
                if hasattr(obj, "__module__") and
                obj.__module__.startswith("karabo"))
        cls.modules = {sys.modules[m].__file__ for m in mods}

    def test_pep8(self):
        for mod in self.modules:
            base = os.path.basename(mod)
            with self.subTest(module=base):
                checker = Checker(mod)
                errs = checker.check_all()
                self.assertEqual(errs, pep8_exceptions.get(base, 0))


if __name__ == "__main__":
    main()
