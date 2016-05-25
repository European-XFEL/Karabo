import unittest
from unittest.mock import call, Mock

from PyQt4.QtCore import QPoint, QRect
from PyQt4.QtGui import QApplication, QWidget

from karabo_gui.layouts import BoxLayout, FixedLayout, ProxyWidget


class Tests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.app = QApplication.instance()
        if cls.app is None:
            cls.app = QApplication([])

    def test_listlike(self):
        bl = BoxLayout(0)
        self.assertEqual(len(bl), 0)
        w = QWidget()
        bl.addWidget(w)
        self.assertEqual(len(bl), 1)
        self.assertIs(bl[0], w)
        self.assertEqual(list(bl), [w])

        sl = BoxLayout(1)
        bl.addItem(sl)
        self.assertEqual(list(bl), [w, sl])
        self.assertEqual(list(bl.iterWidgets()), [w])

        del bl[0]

        self.assertEqual(list(bl), [sl])
        self.assertEqual(list(bl.iterWidgets()), [])
        sl.addWidget(w)
        self.assertEqual(list(bl.iterWidgets()), [w])

    def test_shapes(self):
        bl = BoxLayout(0)
        bls = Mock()
        bl.shapes.append(bls)
        sl = BoxLayout(0)
        sls = Mock()
        sl.shapes.append(sls)
        bl.addItem(sl)
        painter = Mock()

        bl.draw(painter)
        self.assertEqual(bls.method_calls, [call.draw(painter)])
        self.assertEqual(sls.method_calls, [call.draw(painter)])
        bls.reset_mock()
        sls.reset_mock()

        bl.setGeometry(QRect(33, 77, 12, 13))
        bl.setGeometry(QRect(44, 99, 13, 14))
        self.assertEqual(bls.method_calls, [call.translate(QPoint(11, 22))])
        self.assertEqual(sls.method_calls, [call.translate(QPoint(11, 22))])
        bls.reset_mock()
        sls.reset_mock()

    def test_fixed(self):
        parent = QWidget()
        fl = FixedLayout()
        parent.setLayout(fl)

        bl = BoxLayout(0)
        bl.set_geometry(QRect(10, 20, 30, 40))
        fl[0] = bl

        w = ProxyWidget(parent)
        w.set_geometry(QRect(11, 22, 33, 44))
        fl[1] = w

        nested = FixedLayout()
        nested.set_geometry(QRect(100, 200, 300, 400))
        nw = ProxyWidget(parent)
        nw.set_geometry(QRect(111, 222, 30, 40))
        nested[0] = nw
        nestnest = FixedLayout()
        nestnest.set_geometry(QRect(66, 77, 88, 99))
        nested[1] = nestnest
        nnw = ProxyWidget(parent)
        nnw.set_geometry(QRect(70, 80, 10, 20))
        nestnest[0] = nnw
        fl[2] = nested

        fl.activate()
        self.assertEqual(bl.geometry(), QRect(10, 20, 30, 40))
        self.assertEqual(w.geometry(), QRect(11, 22, 33, 44))
        self.assertEqual(nested.geometry(), QRect(100, 200, 300, 400))
        self.assertEqual(nw.geometry(), QRect(111, 222, 30, 40))
        self.assertEqual(nestnest.geometry(), QRect(66, 77, 88, 99))
        self.assertEqual(nnw.geometry(), QRect(70, 80, 10, 20))

        bl.translate(QPoint(5, 6))
        w.translate(QPoint(7, 8))
        nested.translate(QPoint(10, 11))
        fl.activate()
        self.assertEqual(bl.geometry(), QRect(15, 26, 30, 40))
        self.assertEqual(w.geometry(), QRect(18, 30, 33, 44))
        self.assertEqual(nested.geometry(), QRect(110, 211, 300, 400))
        self.assertEqual(nw.geometry(), QRect(121, 233, 30, 40))
        self.assertEqual(nestnest.geometry(), QRect(76, 88, 88, 99))
        self.assertEqual(nnw.geometry(), QRect(80, 91, 10, 20))


if __name__ == '__main__':
    unittest.main()
