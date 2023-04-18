#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on May 11, 2018
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#############################################################################

try:
    from qtconsole.pygments_highlighter import PygmentsHighlighter
except ImportError:
    from IPython.qt.console.pygments_highlighter import PygmentsHighlighter

from pygments.style import Style
from pygments.token import (
    Comment, Keyword, Literal, Name, Number, Operator, String, Token)
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
MAX_FONT_SIZE = 35
MIN_FONT_SIZE = 9


class CustomStyle(Style):
    default_style = "#000000"
    styles = {
        Comment: 'italic #8C8C8C',
        Keyword: '#0033B3',
        Keyword.Type: '#000000',
        Literal: '#000000',
        Name: '#000000',
        Name.Builtin: '#000080',
        Name.Builtin.Pseudo: '#94558D',
        Name.Class: '#000000',
        Name.Decorator: '#9E880D',
        Name.Exception: '#0033B3',
        Name.Function: '#00627A',
        Name.Function.Magic: '#B200B2',
        Name.Namespace: '#000000',
        Name.Variable.Magic: '#B200B2',
        Number: '#1750EB',
        String: '#067D17',
        String.Doc: 'italic #8C8C8C',
        String.Escape: '#0037A6',
        Operator.Word: '#0033B3',
        Token.Text: '#000000',
    }


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
        code_editor.resultFound.connect(self.updateResultText)
        code_editor.textChanged.connect(self.codeChanged)

        find_toolbar = FindToolBar(parent=self)

        find_toolbar.findRequested.connect(self._findAndHighlight)
        find_toolbar.highlightRequested.connect(self._highlight)
        find_toolbar.aboutToClose.connect(self._clearHighlight)
        find_toolbar.replaceRequested.connect(self._replace)
        find_toolbar.setVisible(False)

        find = QShortcut(QKeySequence(Qt.CTRL + Qt.Key_F), self)
        find.activated.connect(self.showFindToolbar)

        replace = QShortcut(QKeySequence(Qt.CTRL + Qt.Key_R), self)
        replace.activated.connect(self.showReplaceToolbar)

        self.find_toolbar = find_toolbar
        self.code_editor = code_editor
        layout.addWidget(self.find_toolbar)
        layout.addWidget(self.code_editor)
        syntax_highlighter = PygmentsHighlighter(self.code_editor.document())
        syntax_highlighter.set_style(CustomStyle)

        increase = QShortcut(QKeySequence(Qt.CTRL + Qt.Key_Plus), self)
        increase.activated.connect(self.code_editor.increase_font_size)

        decrease = QShortcut(QKeySequence(Qt.CTRL + Qt.Key_Minus), self)
        decrease.activated.connect(self.code_editor.decrease_font_size)

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
        match_case = self.find_toolbar.match_case.isChecked()
        if selected_text:
            self.find_toolbar.find_line_edit.setText(selected_text)
            self.code_editor.highlight(selected_text, match_case=match_case)
        self.find_toolbar.find_line_edit.setFocus()
        self.find_toolbar.set_replace_widgets_visibility(False)

    @Slot()
    def showReplaceToolbar(self):
        self.showFindToolbar()
        self.find_toolbar.set_replace_widgets_visibility(True)

    @Slot(str, str, bool, bool)
    def _replace(self, text, new_text, match_case, replace_all):
        if replace_all:
            self.code_editor.replace_all(text, new_text, match_case)
        else:
            self.code_editor.replace(text, new_text, match_case)

    def eventFilter(self, obj, event):
        if event.type() == QEvent.KeyPress:
            if event.key() == Qt.Key_Tab:
                cursor = self.code_editor.textCursor()
                if cursor.hasSelection():
                    self.code_editor.indent_blocks()
                else:
                    self.code_editor.indent_line()
                return True

            if event.key() == Qt.Key_Backtab:
                cursor = self.code_editor.textCursor()
                if cursor.hasSelection():
                    self.code_editor.deindent_blocks()
                else:
                    self.code_editor.deindent_line()
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

    @Slot()
    def increaseFontSize(self):
        self.code_editor.increase_font_size()

    @Slot()
    def decreaseFontSize(self):
        self.code_editor.decrease_font_size()


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

    def indent_line(self):
        """ Indent the cursor line by adding spaces at the beginning"""
        start_pos = self._get_text_start_position()
        remainder = start_pos % 4
        number_of_spaces = 4 - remainder

        cursor = self.textCursor()
        cursor.beginEditBlock()
        cursor.movePosition(cursor.StartOfLine)
        cursor.insertText(" " * number_of_spaces)
        cursor.endEditBlock()

    def indent_blocks(self):
        """ Indent the selected lines."""
        cursor = self.textCursor()
        cursor.beginEditBlock()
        selection_start = cursor.selectionStart()
        selection_end = cursor.selectionEnd()

        cursor.setPosition(selection_end)
        end_block = cursor.blockNumber()
        cursor.setPosition(selection_start)
        start_block = cursor.blockNumber()

        if end_block == start_block:
            cursor.endEditBlock()
            self.indent_line()
            return

        for i in range(end_block - start_block + 1):
            cursor.movePosition(cursor.StartOfLine)
            cursor.insertText(" " * 4)
            cursor.movePosition(cursor.NextBlock)
        cursor.endEditBlock()

    def deindent_line(self):
        cursor = self.textCursor()
        cursor.beginEditBlock()
        start_line = self._get_text_start_position()
        remainder = start_line % 4
        space_count = remainder if remainder else 4
        for i in range(space_count):
            cursor.movePosition(cursor.StartOfLine)
            cursor.movePosition(cursor.Right, cursor.KeepAnchor)
            if cursor.selectedText() == " ":
                cursor.removeSelectedText()
            else:
                break
        cursor.endEditBlock()

    def deindent_blocks(self):
        cursor = self.textCursor()
        cursor.beginEditBlock()
        selection_start = cursor.selectionStart()
        selection_end = cursor.selectionEnd()

        cursor.setPosition(selection_end)
        end_block = cursor.blockNumber()
        cursor.setPosition(selection_start)
        start_block = cursor.blockNumber()

        if end_block == start_block:
            cursor.endEditBlock()
            self.deindent_line()
            return

        for i in range(end_block - start_block + 1):
            for j in range(4):
                cursor.movePosition(cursor.StartOfLine)
                cursor.movePosition(cursor.Right, cursor.KeepAnchor)
                if cursor.selectedText() == " ":
                    cursor.removeSelectedText()
                else:
                    break
            cursor.movePosition(cursor.NextBlock)
        cursor.endEditBlock()

    def _get_text_start_position(self):
        """
        Get the starting position of the text  in the line.
        This is to get the number of space to add in order to match the next
        indentation (multiples of four).
        """
        cursor = self.textCursor()
        cursor.clearSelection()
        cursor.movePosition(cursor.StartOfLine)
        cursor.movePosition(cursor.NextWord)
        pos = cursor.position()
        cursor.movePosition(cursor.PreviousWord, cursor.KeepAnchor)
        if not cursor.selectedText().strip():
            cursor.setPosition(pos)
        return cursor.columnNumber()

    def replace(self, text, new_text, match_case):
        """ Replace the 'text' with new_text. It also selects the next
        occurrence of 'text'. """
        cursor = self.textCursor()
        cursor.beginEditBlock()
        selected_text = cursor.selectedText()
        text_is_selected = (selected_text == text if match_case else
                            selected_text.lower() == text.lower())
        if text_is_selected:
            cursor.insertText(new_text)
        self._highlighted_word = None
        self.findAndHighlight(text, match_case, find_backward=False)
        cursor.endEditBlock()

    def replace_all(self, text, new_text, match_case):
        """ Recursively replace a text with a new text."""
        cursor = self.textCursor()
        cursor.beginEditBlock()
        flags = QTextDocument.FindFlags()
        if match_case:
            flags = flags | QTextDocument.FindCaseSensitively

        self.moveCursor(QTextCursor.Start)
        if self.find(text, flags):
            self.textCursor().insertText(new_text)
            self.replace_all(text, new_text, match_case)
        else:
            self.clearHighlight()
        cursor.endEditBlock()

    def increase_font_size(self):
        font = self.font()
        font_size = font.pointSize() + 2
        if font_size > MAX_FONT_SIZE:
            return
        font.setPointSize(font_size)
        self.setFont(font)
        self.update()
        self.number_widget.setFont(font)
        self.number_widget.update()

    def decrease_font_size(self):
        font = self.font()
        font_size = font.pointSize() - 2
        if font_size < MIN_FONT_SIZE:
            return
        font.setPointSize(font_size)
        self.setFont(font)
        self.number_widget.setFont(font)
        self.update()
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
