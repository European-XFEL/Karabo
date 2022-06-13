#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on May 11, 2018
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

try:
    from qtconsole.pygments_highlighter import PygmentsHighlighter
except ImportError:
    from IPython.qt.console.pygments_highlighter import PygmentsHighlighter

from qtpy.QtCore import (
    QEvent, QRect, QRegularExpression, QSize, Qt, Signal, Slot)
from qtpy.QtGui import (
    QBrush, QColor, QFontMetrics, QKeySequence, QPainter, QTextCharFormat,
    QTextCursor, QTextDocument, QTextFormat)
from qtpy.QtWidgets import (
    QPlainTextEdit, QShortcut, QTextEdit, QVBoxLayout, QWidget)

from karabogui.widgets.find_toolbar import FindToolBar

LINE_WIDGET_BACKGROUND = QColor(224, 224, 224)
LINE_WIDGET_COLOR = QColor(160, 160, 160)
HIGHLIGHT_COLOR = QColor(Qt.yellow).lighter(180)
TEXT_HIGHLIGHT_COLOR = QColor(Qt.yellow)
DEFAULT_BACKGROUND_COLOR = QBrush(QColor(Qt.white))


class CodeBook(QWidget):

    codeChanged = Signal()

    def __init__(self, code=None, parent=None):
        super().__init__(parent)
        layout = QVBoxLayout(self)
        layout.setSpacing(0)
        layout.setContentsMargins(0, 0, 0, 0)

        code_editor = CodeEditor(parent=self)
        if code is not None:
            code_editor.setPlainText(code)
        code_editor.installEventFilter(self)
        PygmentsHighlighter(code_editor.document())
        code_editor.resultFound.connect(self.updateResultText)
        code_editor.textChanged.connect(self.codeChanged)

        find_toolbar = FindToolBar(parent=self)

        find_toolbar.findRequested.connect(self._findAndHighlight)
        find_toolbar.highlightRequested.connect(self._highlight)
        find_toolbar.aboutToClose.connect(self._clearHighlight)
        find_toolbar.setVisible(False)

        find = QShortcut(QKeySequence(Qt.CTRL + Qt.Key_F), self)
        find.activated.connect(self.showFindToolbar)

        self.find_toolbar = find_toolbar
        self.code_editor = code_editor
        layout.addWidget(self.find_toolbar)
        layout.addWidget(self.code_editor)

    @Slot(str, bool, bool)
    def _findAndHighlight(self, text, match_case, find_backward):
        self.code_editor.findAndHighlight(text, match_case, find_backward)

    @Slot(str, bool)
    def _highlight(self, text, match_case):
        self.code_editor.highlight(text, match_case)

    @Slot()
    def _clearHighlight(self):
        self.code_editor.clearHighlight()

    @Slot(int)
    def updateResultText(self, count):
        self.find_toolbar.setResultText(count)

    @Slot()
    def showFindToolbar(self):
        self.find_toolbar.setVisible(True)
        selected_text = self.code_editor.textCursor().selectedText()
        self.find_toolbar.find_line_edit.setText(selected_text)
        match_case = self.find_toolbar.match_case.isChecked()
        if selected_text:
            self.code_editor.highlight(selected_text, match_case=match_case)
        self.find_toolbar.find_line_edit.setFocus()

    def eventFilter(self, obj, event):
        if event.type() == QEvent.KeyPress:
            if event.key() == Qt.Key_Tab:
                self.code_editor.textCursor().insertText(" " * 4)
                return True
        return False

    def moveCursorToLine(self, line_number, offset):
        cursor = self.code_editor.textCursor()
        cursor.movePosition(cursor.Start)
        cursor.movePosition(cursor.Down, n=line_number - 1)
        cursor.movePosition(cursor.Right, n=offset)
        self.code_editor.setTextCursor(cursor)

    def setReadOnly(self, value):
        """ Set the editor read only or not. """
        self.code_editor.setReadOnly(value)

    def getEditorCode(self):
        """ Get the current code from the editor """
        return self.code_editor.toPlainText()


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

    resultFound = Signal(int)

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

        self.setStyleSheet("font-family: monospace")
        self.setLineWrapMode(QPlainTextEdit.NoWrap)
        self._highlighted_word = None

    @Slot(str, bool, bool)
    def findAndHighlight(self, text, match_case, find_backward):
        """
        Look for the given text in the Macro editor and highlight all the
        occurrences.
        Case-sensitive if match_case is 'True'.
        """
        is_new_word = self._highlighted_word != text
        flags = QTextDocument.FindFlags()
        if match_case:
            flags = flags | QTextDocument.FindCaseSensitively
        if find_backward:
            flags = flags | QTextDocument.FindBackward
        found = self.find(text, flags)
        if is_new_word:
            self.clearHighlight()
            if found:
                self.highlight(text, match_case)
        if not found:
            location = QTextCursor.End if find_backward else QTextCursor.Start
            self.moveCursor(location)
            self.find(text, flags)

    @Slot(str, bool)
    def highlight(self, text, match_case):
        """ Highlight all the occurrence of the given text"""
        self.clearHighlight()
        if text == "":
            return
        self._highlighted_word = text
        cursor = self.textCursor()
        text_format = QTextCharFormat()
        text_format.setBackground(QBrush(TEXT_HIGHLIGHT_COLOR))

        options = QRegularExpression().patternOptions()

        if not match_case:
            options = options | QRegularExpression.CaseInsensitiveOption
        pattern = QRegularExpression(text, options)

        matches = pattern.globalMatch(self.toPlainText())

        hit_count = 0
        while matches.hasNext():
            match = matches.next()
            if match.capturedLength() != len(pattern.pattern()):
                # Avoid matching regular expression.
                continue
            cursor.setPosition(match.capturedStart(), QTextCursor.MoveAnchor)
            cursor.setPosition(match.capturedEnd(), QTextCursor.KeepAnchor)
            cursor.mergeCharFormat(text_format)
            hit_count += 1
        self.resultFound.emit(hit_count)

    @Slot()
    def clearHighlight(self):
        text_cursor = self.textCursor()
        text_format = QTextCharFormat()
        text_format.setBackground(DEFAULT_BACKGROUND_COLOR)
        text_cursor.setPosition(0, QTextCursor.MoveAnchor)
        text_cursor.setPosition(len(self.toPlainText()),
                                QTextCursor.KeepAnchor)
        text_cursor.setCharFormat(text_format)
        self._highlighted_word = None
        self.resultFound.emit(0)

    # -----------------------------------
    # Qt slots

    @Slot()
    def updateMargins(self):
        self.setViewportMargins(self.numberWidgetArea(), 0, 0, 0)

    @Slot(QRect, int)
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

    @Slot()
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
