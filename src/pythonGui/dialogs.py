from PyQt4 import uic
from PyQt4.QtCore import Qt, pyqtSlot
from PyQt4.QtGui import (QDialog, QIcon, QPixmap, QColorDialog, QValidator,
                         QPalette, QFrame, QFontDialog)
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


    def __init__(self, pen, brush=None):
        QDialog.__init__(self)
        uic.loadUi(path.join(path.dirname(__file__), 'pendialog.ui'), self)

        self.pen = pen
        self.brush = brush
        self.set_color()
        self.opacity.setValue(pen.color().alpha())
        self.width.setValue(pen.widthF())
        if pen.style() == Qt.NoPen:
            self.width.setValue(0)
        getattr(self, self.linecaps[pen.capStyle()] + 'Cap').setChecked(True)
        getattr(self, self.linejoins[pen.joinStyle()] + 'Join').setChecked(True)
        self.miterlimit.setValue(pen.miterLimit())
        self.dashes.setText(' '.join(unicode(s)
                            for s in pen.dashPattern()))
        self.dashes.setValidator(Validator(self.dashes))

        if brush is None:
            self.actionBrush.setChecked(True)
            self.actionBrush.setChecked(False)
        else:
            if brush.style() == Qt.SolidPattern:
                self.fillopacity.setValue(brush.color().alpha())
            else:
                self.fillopacity.setValue(0)

    @pyqtSlot()
    def on_color_clicked(self):
        self.pen.setColor(QColorDialog.getColor(self.pen.color()))
        self.set_color()


    @pyqtSlot()
    def on_fillcolor_clicked(self):
        self.brush.setColor(QColorDialog.getColor(self.brush.color()))
        self.set_color()


    def set_color(self):
        p = QPixmap(32, 16)
        p.fill(self.pen.color())
        self.color.setIcon(QIcon(p))
        if self.brush is not None:
            p.fill(self.brush.color())
            self.fillcolor.setIcon(QIcon(p))


    def exec_(self):
        QDialog.exec_(self)
        c = self.pen.color()
        c.setAlpha(self.opacity.value())
        self.pen.setColor(c)
        for k, v in self.linecaps.iteritems():
            if getattr(self, v + 'Cap').isChecked():
                self.pen.setCapStyle(k)
        for k, v in self.linejoins.iteritems():
            if getattr(self, v + 'Join').isChecked():
                self.pen.setJoinStyle(k)
        self.pen.setMiterLimit(self.miterlimit.value())
        self.pen.setDashPattern([float(s) for s in self.dashes.text().split()])
        if self.width.value() == 0:
            self.pen.setStyle(Qt.NoPen)
        else:
            self.pen.setWidth(self.width.value())
        if self.brush is not None:
            if self.fillopacity.value() == 0:
                self.brush.setStyle(Qt.NoBrush)
            else:
                self.brush.setStyle(Qt.SolidPattern)
                c = self.brush.color()
                c.setAlpha(self.fillopacity.value())
                self.brush.setColor(c)


class TextDialog(QDialog):
    def __init__(self, label):
        QDialog.__init__(self)
        uic.loadUi(path.join(path.dirname(__file__), 'textdialog.ui'), self)

        self.label = label
        self.palette = QPalette(label.palette())
        self.text.setText(label.text())
        self.set_color()
        self.background.setChecked(self.label.autoFillBackground())
        self.framewidth.setValue(self.label.frameWidth())


    @pyqtSlot()
    def on_textcolor_clicked(self):
        self.palette.setColor(QPalette.Foreground,
            QColorDialog.getColor(self.palette.color(QPalette.Foreground)))
        self.set_color()


    @pyqtSlot()
    def on_backcolor_clicked(self):
        self.palette.setColor(QPalette.Background,
            QColorDialog.getColor(self.palette.color(QPalette.Background)))
        self.set_color()


    @pyqtSlot()
    def on_font_clicked(self):
        font, ok = QFontDialog.getFont(self.label.font(), self)
        if ok:
            self.label.setFont(font)


    def set_color(self):
        p = QPixmap(32, 16)
        p.fill(self.palette.windowText().color())
        self.textcolor.setIcon(QIcon(p))
        p.fill(self.palette.window().color())
        self.backcolor.setIcon(QIcon(p))


    def exec_(self):
        QDialog.exec_(self)
        self.label.setPalette(self.palette)
        self.label.setText(self.text.text())
        self.label.setFrameShape(QFrame.NoFrame if self.framewidth.value == 0
                                 else QFrame.Box)
        self.label.setLineWidth(self.framewidth.value())
        self.label.setAutoFillBackground(self.background.isChecked())
