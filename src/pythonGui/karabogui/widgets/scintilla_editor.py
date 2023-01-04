from jedi import Script
from qtpy.Qsci import QsciAPIs, QsciScintilla
from qtpy.QtCore import Signal, Slot
from qtpy.QtGui import QColor

from karabogui.widgets.scintilla_lexer import PythonLexer

AUTO_COMPLETION_THRESHOLD = 3
LINE_HIGHLIGHT = QColor("#FFFFCC")
MARGIN_BACKGROUND = QColor("#E0E0E0")
MARGIN_FOREGROUND = QColor("#A0A0A0")


class CodeBook(QsciScintilla):
    """
    Python Code editor class.
    """

    codeChanged = Signal()

    def __init__(self, code=None, parent=None):
        super().__init__(parent)

        if code is not None:
            self.setText(code)
        # Indentation
        self.setTabWidth(4)
        self.setAutoIndent(True)
        self.setIndentationsUseTabs(True)
        self.setIndentationGuides(True)
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

        # A vertical line at 79th column as indicate the line-length limit
        self.setEdgeMode(QsciScintilla.EdgeLine)
        self.setEdgeColumn(79)
        self.setEdgeColor(QColor("gray"))

        self.textChanged.connect(self._update_suggestions)
        self._apis = QsciAPIs(lexer)
        self.setLexer(lexer)

    @Slot()
    def _update_suggestions(self):
        """
        Update the auto-suggestion list using 'jedi.Script'. This suggests the
        apis from the installed libraries effectively.
        """
        line, col = self.getCursorPosition()
        if col < AUTO_COMPLETION_THRESHOLD:
            return
        text = self.text()
        script = Script(text)
        completions = script.complete(line+1, col)
        self._apis.clear()
        for item in completions:
            self._apis.add(item.name)
        self._apis.prepare()

    def increaseFontSize(self):
        self.zoomIn()

    def decreaseFontSize(self):
        self.zoomOut()

    #  TODO : Find and Replace functionality
