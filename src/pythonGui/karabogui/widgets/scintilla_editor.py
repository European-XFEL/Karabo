from jedi import Script
from qtpy.Qsci import QsciAPIs, QsciScintilla
from qtpy.QtCore import QTimer, Signal, Slot
from qtpy.QtGui import QColor

from karabogui.background import background
from karabogui.widgets.scintilla_lexer import PythonLexer

AUTO_COMPLETION_THRESHOLD = 3
LINE_LENGTH = 79
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

        # A vertical line to indicate the line-length limit
        self.setEdgeMode(QsciScintilla.EdgeLine)
        self.setEdgeColumn(LINE_LENGTH)
        self.setEdgeColor(QColor("gray"))

        self._apis = QsciAPIs(lexer)
        self.setLexer(lexer)

        self.timer = QTimer(self)
        self.timer.timeout.connect(self._update_suggestions)
        self.timer.setInterval(500)
        self.timer.setSingleShot(True)
        self.textChanged.connect(self.timer.start)
        self.textChanged.connect(self.codeChanged)

    @Slot()
    def _update_suggestions(self):
        """
        Update the auto-suggestion list using 'jedi.Script'. This suggests the
        apis from the installed libraries effectively.
        """
        line, col = self.getCursorPosition()
        if col < AUTO_COMPLETION_THRESHOLD:
            return

        background(self._prepare_apis, line, col)

    def _prepare_apis(self, line, col):
        """Prepare the auto completion with new input"""
        text = self.text()
        script = Script(text)
        completions = script.complete(line + 1, col)
        self._apis.clear()
        for item in completions:
            self._apis.add(item.name)
        self._apis.prepare()

    def getEditorCode(self):
        """Return the text of the code editor"""
        return self.text()

    def increaseFontSize(self):
        self.zoomIn()

    def decreaseFontSize(self):
        self.zoomOut()
