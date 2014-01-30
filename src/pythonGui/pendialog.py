from PyQt4 import uic
from PyQt4.QtCore import Qt, pyqtSlot
from PyQt4.QtGui import QDialog, QIcon, QPixmap, QColorDialog, QValidator
from os import path

class Validator(QValidator):
    def validate(self, input, pos):
        try:
            if len([float(s) for s in input.split()]):
                return QValidator.Intermediate, input, pos
            else:
                return QValidator.Acceptable, input, pos
        except:
            return QValidator.Invalid, input, pos


class PenDialog(QDialog):
    linecaps = {Qt.FlatCap: "butt", Qt.SquareCap: "square",
                Qt.RoundCap: "round"}
    linejoins = {Qt.SvgMiterJoin: "miter", Qt.MiterJoin: "miter",
                 Qt.BevelJoin: "bevel", Qt.RoundJoin: "round"}


    def __init__(self, pen):
        QDialog.__init__(self)
        uic.loadUi(path.join(path.dirname(__file__), 'pendialog.ui'), self)

        self.pen = pen
        self.set_color()
        self.opacity.setValue(pen.color().alpha())
        self.width.setValue(pen.widthF())
        getattr(self, self.linecaps[pen.capStyle()] + 'Cap').setChecked(True)
        getattr(self, self.linejoins[pen.joinStyle()] + 'Join').setChecked(True)
        self.miterlimit.setValue(pen.miterLimit())
        self.dashes.setText(' '.join(unicode(s)
                            for s in pen.dashPattern()))
        self.dashes.setValidator(Validator(self.dashes))
        self.pen = pen


    @pyqtSlot()
    def on_color_clicked(self):
        self.pen.setColor(QColorDialog.getColor(self.pen.color()))
        self.set_color()


    def set_color(self):
        p = QPixmap(32, 16)
        p.fill(self.pen.color())
        self.color.setIcon(QIcon(p))


    def exec_(self):
        QDialog.exec_(self)
        c = self.pen.color()
        c.setAlpha(self.opacity.value())
        self.pen.setColor(c)
        self.pen.setWidth(self.width.value())
        for k, v in self.linecaps.iteritems():
            if getattr(self, v + 'Cap').isChecked():
                self.pen.setCapStyle(k)
        for k, v in self.linejoins.iteritems():
            if getattr(self, v + 'Join').isChecked():
                self.pen.setJoinStyle(k)
        self.pen.setMiterLimit(self.miterlimit.value())
        self.pen.setDashPattern([float(s) for s in self.dashes.text().split()])
