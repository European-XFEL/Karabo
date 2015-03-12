import karabo
karabo.api_version = 2

import gc
from unittest import TestCase, main
import sys
import time
import weakref

from karabo.macro import Macro
from karabo import Integer, Slot


class Remote(Macro):
    counter = Integer(defaultValue=-1)

    @Slot()
    def count(self):
        for i in range(30):
            self.counter = i
            time.sleep(0.1)


class Tests(TestCase):
    def test_delete(self):
        remote = Remote()
        remote.count()
        self.assertEqual(remote.counter, 29)
        r = weakref.ref(remote)
        del remote
        gc.collect()
        self.assertIsNone(r())

    def test_main(self):
        save = sys.argv
        try:
            sys.argv = ["", "count", "counter=7"]
            Remote.main()
        finally:
            sys.argv = save


if __name__ == "__main__":
    main()
