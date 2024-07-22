# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
from qtpy.Qsci import QsciLexerPython
from qtpy.QtGui import QColor, QFont

from karabogui.fonts import get_qfont

# Collection of comment types as Enum.
COMMENTS = (
    QsciLexerPython.Comment, QsciLexerPython.CommentBlock,
    QsciLexerPython.TripleDoubleQuotedString,
    QsciLexerPython.TripleSingleQuotedString
)

# Syntax color map
COLOR_MAP = {
    QsciLexerPython.Default: QColor("#000000"),
    QsciLexerPython.Comment: QColor("#8C8C8C"),
    QsciLexerPython.Number: QColor("#1750EB"),
    QsciLexerPython.DoubleQuotedString: QColor("#067D17"),
    QsciLexerPython.DoubleQuotedFString: QColor("#067D17"),
    QsciLexerPython.SingleQuotedString: QColor("#067D17"),
    QsciLexerPython.SingleQuotedFString: QColor("#067D17"),
    QsciLexerPython.UnclosedString: QColor("#067D17"),
    QsciLexerPython.Keyword: QColor("#0033B3"),
    QsciLexerPython.TripleSingleQuotedString: QColor("#8C8C8C"),
    QsciLexerPython.TripleDoubleQuotedString: QColor("#8C8C8C"),
    QsciLexerPython.ClassName: QColor("#000000"),
    QsciLexerPython.FunctionMethodName: QColor("#00627A"),
    QsciLexerPython.Operator: QColor("#000000"),
    QsciLexerPython.Identifier: QColor("#000000"),
    QsciLexerPython.CommentBlock: QColor("#8C8C8C"),
    QsciLexerPython.HighlightedIdentifier: QColor("#94558D"),
    QsciLexerPython.Decorator: QColor("#9E880D"),
}


class PythonLexer(QsciLexerPython):
    """
    Syntax highlighter for Macro Editor
    """

    def __init__(self, parent=None):
        super().__init__(parent)

        # Set font
        font = get_qfont("Source Code Pro,10,-1,5,50,0,0,0,0,0")
        for item, color in COLOR_MAP.items():
            self.setColor(color, item)
            self.setFont(font, item)

        # Comment text in Italics
        comment_font = QFont(font)
        comment_font.setItalic(True)
        for comment in COMMENTS:
            self.setFont(comment_font, comment)

        # code fold
        self.setFoldComments(True)
        self.setFoldQuotes(True)
        self.setFoldCompact(True)

    def keywords(self, set_number):
        """
        Extend method of QsciLexerPython to create a keyword set for 'True',
        'False', 'self' and 'cls' to color them in the editor.
        """
        ret_value = super().keywords(set_number)
        # Other Python reserved keywords are defined as set number 1 in
        # QsciLexerPython
        if set_number == 2:
            ret_value = "True False self cls"
        return ret_value
