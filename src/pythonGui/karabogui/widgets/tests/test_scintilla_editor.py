from karabogui.widgets.scintilla_api import create_symbols
from karabogui.widgets.scintilla_editor import (
    HIGHLIGHT_INDICATOR, CodeBook, CodeEditor)

MULTILINE_CODE = """
This is a dummy code
containing multiple lines
Number 1
Number 2
Number 3
"""


def test_scintilla_editor(gui_app):
    symbols = create_symbols()
    assert len(symbols) > 150

    code_book = CodeBook(code="from karabo.native import Hash")

    editor = code_book.code_editor
    # Basic default features
    assert editor.tabWidth() == 4
    assert editor.autoIndent()
    assert editor.indentationGuides()
    assert not editor.indentationsUseTabs()
    assert code_book.getEditorCode() is not None


def test_find_match(gui_app):
    code_editor = CodeEditor(use_api=False)
    code_editor.setText(MULTILINE_CODE)

    def assert_mouse_position(line, index):
        assert code_editor.getCursorPosition() == (line, index)

    assert_mouse_position(0, 0)

    assert code_editor.find_match("Number", False, False)
    assert code_editor.selectedText() == "Number"
    assert_mouse_position(3, 6)

    assert not code_editor.find_match("number", match_case=True,
                                      find_backward=False)

    assert code_editor.find_match("number", match_case=False,
                                  find_backward=False)
    assert_mouse_position(4, 6)

    assert code_editor.find_match("number", match_case=False,
                                  find_backward=False)
    assert_mouse_position(5, 6)

    # Loop over the code.
    assert code_editor.find_match("number", match_case=False,
                                  find_backward=False)
    assert_mouse_position(3, 6)

    # Search backward
    assert code_editor.find_match("number", match_case=False,
                                  find_backward=True)
    assert_mouse_position(5, 6)

    # Non-existing text
    assert not code_editor.find_match("foo", match_case=False,
                                      find_backward=True)


def test_replace(gui_app):
    code_editor = CodeEditor(use_api=False)
    code_editor.setText(MULTILINE_CODE)

    def assert_mouse_position(line, index):
        assert code_editor.getCursorPosition() == (line, index)

    assert_mouse_position(0, 0)

    # Replace first occurrence
    code_editor.replace_text("number", "Line", match_case=False)
    assert_mouse_position(3, 4)
    assert code_editor.selectedText() == "Line"
    expected = MULTILINE_CODE.replace("Number", "Line", 1)
    assert code_editor.text() == expected

    # Replace all
    code_editor.replace_all("Number", "Line", match_case=False)
    expected = MULTILINE_CODE.replace("Number", "Line")
    assert code_editor.text() == expected

    # Case-sensitive replace. Should not change the 'lines' in 2nd line
    code_editor.replace_all("Line", "Number", match_case=True)
    assert code_editor.text() == MULTILINE_CODE

    # Undo should revert all the replaces together.
    code_editor.undo()
    assert code_editor.text() == expected


def test_highlight(gui_app):
    """ Test for highlight and clear highlight"""
    code_editor = CodeEditor(use_api=False)
    code_editor.setText(MULTILINE_CODE)

    def has_highlight(pos):
        """Check if the given position has highlight"""
        # This scintilla api returns 1 if has highlight else 0.
        return bool(code_editor.SendScintilla(
            code_editor.SCI_INDICATORVALUEAT, HIGHLIGHT_INDICATOR, pos))

    code_editor.highlight("Number", match_case=False)
    expected = [
        {'line_start': 3, 'index_start': 0, 'line_end': 3, 'index_end': 6},
        {'line_start': 4, 'index_start': 0, 'line_end': 4, 'index_end': 6},
        {'line_start': 5, 'index_start': 0, 'line_end': 5, 'index_end': 6}]

    assert code_editor._highlights == expected

    # Highlight position as key line number and value (index_start, index_end)
    highlights = {3: (0, 6), 4: (0, 6), 5: (0, 6)}
    # Test only the expected positions has highlight in the entire code.
    for pos in range(len(code_editor.text())):
        line, index = code_editor.lineIndexFromPosition(pos)
        highlight_expected = False
        if line in highlights:
            highlight_expected = index in range(*highlights[line])
        assert has_highlight(pos) == highlight_expected

    # Clear highlight
    code_editor.clearHighlight()
    assert code_editor._highlights == []

    # No letter should have highlight
    for pos in range(len(code_editor.text())):
        assert not has_highlight(pos)


def test_highlight_from_find_toolbar(gui_app):
    """
    Typing/clearing the search text in the Find Toolbar should highlight it
    and also update the label in the toolbar.
    """
    code_book = CodeBook(code=MULTILINE_CODE)
    code_editor = code_book.code_editor
    find_toolbar = code_book.find_toolbar
    find_toolbar.find_line_edit.setText("is")
    assert len(code_editor._highlights) == 2
    assert find_toolbar.result_label.text() == "2 Results"
    find_toolbar.find_line_edit.clear()
    assert len(code_editor._highlights) == 0
    assert find_toolbar.result_label.text() == "0 Results"
