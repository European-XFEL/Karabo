#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on May 11, 2018
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt5.QtCore import pyqtSlot, QRect, QSize, Qt
from PyQt5.QtGui import QColor, QFontMetrics, QPainter, QTextFormat
from PyQt5.QtWidgets import QPlainTextEdit, QTextEdit, QWidget

LINE_WIDGET_BACKGROUND = QColor(224, 224, 224)
LINE_WIDGET_COLOR = QColor(160, 160, 160)
HIGHLIGHT_COLOR = QColor(Qt.yellow).lighter(180)


class LineNumberWidget(QWidget):
    def __init__(self, parent):
        super(LineNumberWidget, self).__init__(parent)
        self.editor = parent

    def sizeHint(self):
        return QSize(self.editor.numberWidgetArea(), 0)

    def paintEvent(self, event):
        self.editor.numberPaintEvent(event)


class CodeEditor(QPlainTextEdit):
    """This widget is an adaption of the 'official' Qt CodeEditor

    The code has been extended with block and line caching to prevent
    unnecessary repainting.
    """
    def __init__(self, parent=None):
        super(CodeEditor, self).__init__(parent)
        self.number_widget = LineNumberWidget(parent=self)
        self.cache_blocks = -1
        self.cache_lines = -1
        self.blockCountChanged.connect(self.updateMargins)
        self.updateRequest.connect(self.updateLineNumberWidget)
        self.cursorPositionChanged.connect(self.highlightCurrentLine)

        self.updateMargins()
        self.highlightCurrentLine()

    # -----------------------------------
    # Qt slots

    @pyqtSlot()
    def updateMargins(self):
        self.setViewportMargins(self.numberWidgetArea(), 0, 0, 0)

    @pyqtSlot(QRect, int)
    def updateLineNumberWidget(self, rect, dy):
        if dy:
            self.number_widget.scroll(0, dy)
        # NOTE: Cache comparison is done due to cursor blinking, e.g.
        # sending an update request. Since we don't want to repaint everytime,
        # we cache our blocks and lines
        elif (self.cache_blocks != self.blockCount()
              or self.cache_lines != self.textCursor().block().lineCount()):
            # Execute the paintEvent
            self.number_widget.update(0, rect.y(),
                                      self.numberWidgetArea(),
                                      rect.height())
            self.cache_blocks = self.blockCount()
            self.cache_lines = self.textCursor().block().lineCount()
        if rect.contains(self.viewport().rect()):
            self.updateMargins()

    @pyqtSlot()
    def highlightCurrentLine(self):
        extra_selections = []

        if not self.isReadOnly():
            selection = QTextEdit.ExtraSelection()
            selection.format.setBackground(HIGHLIGHT_COLOR)
            selection.format.setProperty(QTextFormat.FullWidthSelection, True)
            selection.cursor = self.textCursor()
            selection.cursor.clearSelection()
            extra_selections.append(selection)
        self.setExtraSelections(extra_selections)

    # -----------------------------------
    # Events

    def resizeEvent(self, event):
        super(CodeEditor, self).resizeEvent(event)
        cr = self.contentsRect()
        # resize the block number area
        self.number_widget.setGeometry(
            QRect(cr.left(), cr.top(), self.numberWidgetArea(), cr.height()))

    def numberPaintEvent(self, event):
        """Paint Event execute from the LineNumber widget

        This method draws the block numbers.
        """
        painter = QPainter(self.number_widget)
        painter.fillRect(event.rect(), LINE_WIDGET_BACKGROUND)

        block = self.firstVisibleBlock()
        block_number = block.blockNumber()
        top = self.blockBoundingGeometry(block).translated(
            self.contentOffset()).top()
        bottom = top + self.blockBoundingRect(block).height()

        height = QFontMetrics(self.font()).height()
        while block.isValid() and (top <= event.rect().bottom()):
            if block.isVisible() and (bottom >= event.rect().top()):
                number = '{}'.format(block_number + 1)
                painter.setPen(LINE_WIDGET_COLOR)
                painter.drawText(0, top, self.numberWidgetArea(),
                                 height, Qt.AlignRight, number)

            block = block.next()
            top = bottom
            bottom = top + self.blockBoundingRect(block).height()
            block_number += 1

    # -----------------------------------
    # Public interface

    def numberWidgetArea(self):
        """Calculate the required space for the code line numbers
        """
        digits = 1
        count = max(1, self.blockCount())
        while count >= 10:
            count /= 10
            digits += 1
        # The font widget is around 6 pixels
        space = 3 + self.fontMetrics().width('9') * digits
        return space
