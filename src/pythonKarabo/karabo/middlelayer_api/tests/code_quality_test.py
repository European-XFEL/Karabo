"""
Test the middle layer code against PEP8 and pyflakes

There are two exception lists which list the number of expected
warnings per file. Update as needed.
"""
import os
from unittest import TestCase, main
import sys

from mccabe import get_code_complexity
from pycodestyle import Checker
from pyflakes.api import checkPath

from karabo import middlelayer

pep8_exceptions = {
}

flakes_exceptions = {
    "states.py": 1,
}


class Tests(TestCase):
    @classmethod
    def setUpClass(cls):
        mods = (obj.__module__ for obj in middlelayer.__dict__.values()
                if hasattr(obj, "__module__")
                and obj.__module__.startswith("karabo"))
        cls.modules = {sys.modules[m].__file__ for m in mods}

    def test_pep8(self):
        for mod in self.modules:
            base = os.path.basename(mod)
            with self.subTest(module=base):
                checker = Checker(mod)
                errs = checker.check_all()
                self.assertEqual(errs, pep8_exceptions.get(base, 0))

    def test_flakes(self):
        for mod in self.modules:
            base = os.path.basename(mod)
            with self.subTest(module=base):
                errs = checkPath(mod)
                self.assertEqual(errs, flakes_exceptions.get(base, 0))

    def test_mmcabe(self):
        for mod in self.modules:
            base = os.path.basename(mod)
            with self.subTest(module=base):
                with open(mod, "r", encoding="utf-8") as fin:
                    code = fin.read()
                complexity = get_code_complexity(code, filename=base)
                self.assertLess(complexity, 6)


if __name__ == "__main__":
    main()
