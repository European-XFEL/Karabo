import os
import os.path as op
from unittest import TestCase

from mccabe import get_code_complexity
from pycodestyle import Checker
from pyflakes.api import checkPath

import karabogui

GUI_MODULES = [op.join(r, name)
               for r, d, fs in os.walk(op.dirname(karabogui.__file__))
               for name in fs if name.endswith('.py') and name != 'api.py']
PEP8_EXCEPTIONS = {
    'pathparser.py': 1,
}
FLAKES_EXCEPTIONS = {
    'globals.py': 2,
    'test_sparkline.py': 1,
}
MAX_COMPLEXITY = 6


class TestCodeQuality(TestCase):
    def test_pep8(self):
        for mod in GUI_MODULES:
            base = os.path.basename(mod)
            with self.subTest(module=base):
                checker = Checker(mod)
                errs = checker.check_all()
                msg = 'PEP8 failed for {}'.format(mod)
                assert errs == PEP8_EXCEPTIONS.get(base, 0), msg

    def test_flakes(self):
        for mod in GUI_MODULES:
            base = os.path.basename(mod)
            with self.subTest(module=base):
                errs = checkPath(mod)
                msg = 'pyflakes failed for {}'.format(mod)
                assert errs == FLAKES_EXCEPTIONS.get(base, 0), msg

    def test_mmcabe(self):
        for mod in GUI_MODULES:
            base = os.path.basename(mod)
            with self.subTest(module=base):
                with open(mod, "r", encoding="utf-8") as fin:
                    code = fin.read()
                complexity = get_code_complexity(code, filename=base)
                msg = 'mccabe failed for {}'.format(mod)
                assert complexity < MAX_COMPLEXITY, msg
