from qtpy.Qsci import QsciAPIs, QsciScintilla
from qtpy.QtCore import Signal
from qtpy.QtGui import QColor

from karabogui.widgets.scintilla_api import get_symbols
from karabogui.widgets.scintilla_lexer import PythonLexer

AUTO_COMPLETION_THRESHOLD = 3
LINE_LENGTH = 79
LINE_HIGHLIGHT = QColor("#FFFFCC")
MARGIN_BACKGROUND = QColor("#E0E0E0")
MARGIN_FOREGROUND = QColor("#A0A0A0")


class CodeBook(QsciScintilla):
    """Python Code editor class based on Scintilla"""

    codeChanged = Signal()

    def __init__(self, code=None, use_api=True, parent=None):
        super().__init__(parent)
        if code is not None:
            self.setText(code)
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
        self.textChanged.connect(self.codeChanged)

    def getEditorCode(self):
        """Return the text of the code editor"""
        return self.text()

    def increaseFontSize(self):
        self.zoomIn()

    def decreaseFontSize(self):
        self.zoomOut()
