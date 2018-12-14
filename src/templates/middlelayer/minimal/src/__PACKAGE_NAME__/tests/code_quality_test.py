import os
from unittest import TestCase, main
import sys

from pycodestyle import Checker
from pyflakes.api import checkPath

import __PACKAGE_NAME__

pep8_exceptions = {
}

flakes_exceptions = {
}


class Tests(TestCase):
    @classmethod
    def setUpClass(cls):
        mods = (obj.__module__ for obj in __PACKAGE_NAME__.__dict__.values()
                if hasattr(obj, "__module__")
                and obj.__module__.startswith("__PACKAGE_NAME__"))
        cls.modules = {sys.modules[m].__file__ for m in mods}
        print("Testing code quality for modules {}".format(cls.modules))

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


if __name__ == "__main__":
    main()
