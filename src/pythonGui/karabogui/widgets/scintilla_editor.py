from qtpy.Qsci import QsciAPIs, QsciScintilla
from qtpy.QtCore import Qt, Signal, Slot
from qtpy.QtGui import QColor, QKeySequence
from qtpy.QtWidgets import QShortcut, QVBoxLayout, QWidget

from karabogui.widgets.find_toolbar import FindToolBar
from karabogui.widgets.scintilla_api import get_symbols
from karabogui.widgets.scintilla_lexer import PythonLexer

AUTO_COMPLETION_THRESHOLD = 3
LINE_LENGTH = 79
LINE_HIGHLIGHT = QColor("#FFFFCC")
MARGIN_BACKGROUND = QColor("#E0E0E0")
MARGIN_FOREGROUND = QColor("#A0A0A0")


class CodeBook(QWidget):
    """Python Code editor class based on Scintilla"""

    codeChanged = Signal()

    def __init__(self, code=None, parent=None):
        super().__init__(parent)
        layout = QVBoxLayout(self)
        layout.setSpacing(0)
        layout.setContentsMargins(0, 0, 0, 0)

        code_editor = CodeEditor(parent=self)
        if code is not None:
            code_editor.setText(code)

        code_editor.textChanged.connect(self.codeChanged)

        find_toolbar = FindToolBar(parent=self)
        find_toolbar.findRequested.connect(self._findNext)
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

    @Slot()
    def showFindToolbar(self):
        self.find_toolbar.setVisible(True)
        selected_text = self.code_editor.selectedText()
        if selected_text:
            self.find_toolbar.find_line_edit.setText(selected_text)
        self.find_toolbar.find_line_edit.setFocus()
        self.find_toolbar.set_replace_widgets_visibility(False)

    @Slot()
    def showReplaceToolbar(self):
        self.showFindToolbar()
        self.find_toolbar.set_replace_widgets_visibility(True)

    @Slot(str, bool, bool)
    def _findNext(self, text, match_case, find_backward):
        self.code_editor.find_match(text, match_case, find_backward)

    @Slot(str, str, bool, bool)
    def _replace(self, text, new_text, match_case, replace_all):
        if replace_all:
            self.code_editor.replace_all(text, new_text, match_case)
        else:
            self.code_editor.replace_text(text, new_text, match_case)

    def increaseFontSize(self):
        self.code_editor.zoomIn()

    def decreaseFontSize(self):
        self.code_editor.zoomOut()

    def setReadOnly(self, value):
        """ Set the editor read only or not. """
        self.code_editor.setReadOnly(value)

    def moveCursorToLine(self, line_number, offset):
        self.code_editor.setCursorPosition(line_number, offset)

    def getEditorCode(self):
        """ Get the current code from the editor """
        return self.code_editor.text()


class CodeEditor(QsciScintilla):
    """
    Python Code editor class.
    """

    def __init__(self, parent=None, use_api=True):
        super().__init__(parent)

        # Indentation
        self.setTabWidth(4)
        self.setAutoIndent(True)
        self.setIndentationsUseTabs(True)
        self.setIndentationGuides(True)
        self.setBackspaceUnindents(True)
        self.setIndentationGuidesBackgroundColor(MARGIN_BACKGROUND)
        self.setIndentationGuidesForegroundColor(MARGIN_BACKGROUND)

        # Auto completion
        self.setAutoCompletionSource(QsciScintilla.AcsAll)
        self.setAutoCompletionReplaceWord(True)
        self.setAutoCompletionCaseSensitivity(False)
        self.autoCompleteFromDocument()
        self.setAutoCompletionThreshold(AUTO_COMPLETION_THRESHOLD)

        # Highlight the current line
        self.setCaretLineVisible(True)
        self.setCaretLineBackgroundColor(LINE_HIGHLIGHT)

        # lexer
        lexer = PythonLexer(parent=self)

        # Show line numbers
        font = lexer.font(PythonLexer.Default)
        self.setMarginsFont(font)
        self.setMarginWidth(0, "0000")
        self.setMarginLineNumbers(0, True)
        self.setMarginsBackgroundColor(MARGIN_BACKGROUND)
        self.setMarginsForegroundColor(MARGIN_FOREGROUND)

        # Brace matching: highlight the brace when the cursor is on left or
        # on right
        self.setBraceMatching(QsciScintilla.SloppyBraceMatch)

        # Code folding
        self.setFolding(QsciScintilla.PlainFoldStyle)
        self.setFoldMarginColors(MARGIN_BACKGROUND, MARGIN_BACKGROUND)

        # A vertical line to indicate the line-length limit
        self.setEdgeMode(QsciScintilla.EdgeLine)
        self.setEdgeColumn(LINE_LENGTH)
        self.setEdgeColor(QColor("gray"))

        if use_api:
            self._apis = QsciAPIs(lexer)
            for symbol in get_symbols():
                self._apis.add(symbol)
            self._apis.prepare()

        self.setLexer(lexer)

    def find_match(self, text, match_case=False, find_backward=False):
        """
        Select the first match from the current cursor position.
        Return True if found else False.
        """
        regular_expression = False
        word_only = False
        wrap = True
        find_forward = not find_backward
        line, col = self.getCursorPosition()
        if find_backward:
            # Move cursor behind by one, so that backward search doesn't get
            # stuck on the just single hit.
            col -= 1
        return self.findFirst(text, regular_expression, match_case, word_only,
                              wrap, find_forward, line, col)

    def replace_text(self, text, new_text, match_case=False):
        """
        Replace the text with new_text.
        """
        if self.find_match(text, match_case):
            self.replace(new_text)

    def replace_all(self, text, new_text, match_case=False):
        found = self.find_match(text, match_case)
        if found:
            self.replace(new_text)
            while self.findNext():
                self.replace(new_text)
