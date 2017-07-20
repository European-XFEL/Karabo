"""
Test the middle layer code against PEP8 and pyflakes

There are two exception lists which list the number of expected
warnings per file. Update as needed.
"""
import os
from unittest import TestCase, main
import sys

from pycodestyle import Checker
from pyflakes.api import checkPath

from karabo import middlelayer

pep8_exceptions = {
}

flakes_exceptions = {
    "states.py": 1,
}


class Tests(TestCase):
    def find_modules(self):
        mods = (obj.__module__ for obj in middlelayer.__dict__.values()
                if hasattr(obj, "__module__")
                and obj.__module__.startswith("karabo"))
        mods = {sys.modules[m].__file__ for m in mods}
        return mods

    def test_pep8(self):
        mods = self.find_modules()
        for mod in mods:
            base = os.path.basename(mod)
            with self.subTest(module=base):
                checker = Checker(mod)
                errs = checker.check_all()
                self.assertEqual(errs, pep8_exceptions.get(base, 0))

    def test_flakes(self):
        mods = self.find_modules()
        for mod in mods:
            base = os.path.basename(mod)
            with self.subTest(module=base):
                errs = checkPath(mod)
                self.assertEqual(errs, flakes_exceptions.get(base, 0))


if __name__ == "__main__":
    main()
