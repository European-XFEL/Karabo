from PyQt4.QtCore import QPointF
from PyQt4.QtGui import QPainterPath


import re


class Parser(object):
    re = re.compile("([MmZzLlHhVvCcSsQqTtAa])|"
        "(-?(([0-9]*[.][0-9]*)|[0-9]+)([eE][+-]?[0-9]*)?)")

    def next(self):
        if self.iter is None:
            self.token = ''
            raise StopIteration
        ret = float(self.number) if self.number is not None else self.token
        try:
            self.token, self.number, _, _, _ = self.iter.next().groups()
        except StopIteration:
            self.iter = None
        return ret


    def __init__(self, s):
        self.iter = self.re.finditer(s)


    def __iter__(self):
        while self.token is None:
            yield self.next()


    def parse(self):
        self.path = QPainterPath()
        self.pos = QPointF(0, 0)
        try:
            self.number = self.token = None
            self.next()
            while True:
                token = self.next()
                getattr(self, token.lower())(token.islower())
                self.lasttoken = token
        except StopIteration:
            return self.path


    def points(self, relative):
        for x, y in zip(self, self):
            if relative:
                yield self.path.currentPosition() + QPointF(x, y)
            else:
                yield QPointF(x, y)


    def m(self, relative):
        p = self.points(relative).next()
        self.path.moveTo(p)
        self.l(relative)


    def z(self, relative):
        self.path.closeSubpath()


    def l(self, relative):
        for p in self.points(relative):
            self.path.lineTo(p)


    def h(self, relative):
        for x in self:
            pos = self.path.currentPosition()
            if relative:
                pos += QPointF(x, 0)
            else:
                pos.setX(x)
            self.lineTo(pos)


    def v(self, relative):
        for y in self:
            pos = self.path.currentPosition()
            if relative:
                pos += QPointF(0, y)
            else:
                pos.setY(y)
            self.lineTo(pos)


    def c(self, relative):
        p = self.points(relative)
        for a, b, c in zip(p, p, p):
            self.path.cubicTo(a, b, c)
        self.lastcontrol = b


    def s(self, relative):
        p = self.points(relative)
        if self.lasttoken not in 'CcSs':
            self.lastcontrol = self.pos
        a = 2 * self.pos - self.lastcontrol
        for b, c in zip(p, p):
            self.path.cubicTo(a, b, c)
            a = 2 * c - b
        self.lastcontrol = b


    def q(self, relative):
        p = self.points(relative)
        for a, b in zip(p, p):
            self.path.quadTo(a, b)
        self.lastcontrol = a


    def t(self, relative):
        if self.lasttoken not in 'QqTt':
            self.lastcontrol = self.pos
        a = 2 * self.pos - self.lastcontrol
        for b in self.points(relative):
            self.path.cubicTo(a, b)
            a = 2 * c - b
        self.lastcontrol = a


    def a(self, relative):
        for rx, ry, phi, fa, fs, x, y in zip(*(self,) * 7):
            pp = (self.path.currentPosition() - QPointF(x, y)) / 2
            cp = (-1) ** (fa == fs) * sqrt(
                 ((rx * ry) ** 2 - (rx * pp.y()) ** 2 - (ry * pp.x()) ** 2) /
                  ((rx * pp.y()) ** 2 + (ry * pp.x()) ** 2)) * (
                 QPointF(rx * pp.y() / ry, -ry * pp.x() / rx))
            c = cp + (self.path.currentPosition() + QPointF(x, y)) / 2


if __name__ == '__main__':
    p = Parser('c 100, 100, 0, 0 10, 10')
    p.parse()
