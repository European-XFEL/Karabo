from karabogui.widgets.scintilla_api import create_symbols
from karabogui.widgets.scintilla_editor import CodeBook, CodeEditor

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

    assert code_editor.find_match("Number")
    assert code_editor.selectedText() == "Number"
    assert_mouse_position(3, 6)

    assert not code_editor.find_match("number", match_case=True)

    assert code_editor.find_match("number", match_case=False)
    assert_mouse_position(4, 6)

    assert code_editor.find_match("number", match_case=False)
    assert_mouse_position(5, 6)

    # Loop over the code.
    assert code_editor.find_match("number", match_case=False)
    assert_mouse_position(3, 6)

    # Search backward
    assert code_editor.find_match("number", match_case=False,
                                  find_backward=True)
    assert_mouse_position(5, 6)

    # Non-existing text
    assert not code_editor.find_match("foo")


def test_replace(gui_app):
    code_editor = CodeEditor(use_api=False)
    code_editor.setText(MULTILINE_CODE)

    def assert_mouse_position(line, index):
        assert code_editor.getCursorPosition() == (line, index)

    assert_mouse_position(0, 0)

    # Replace first occurrence
    code_editor.replace_text("number", "Line")
    assert_mouse_position(3, 4)
    assert code_editor.selectedText() == "Line"
    expected = MULTILINE_CODE.replace("Number", "Line", 1)
    assert code_editor.text() == expected

    # Replace all
    code_editor.replace_all("Number", "Line")
    assert code_editor.text() == MULTILINE_CODE.replace("Number", "Line")

    # Case-sensitive replace. Should not change the 'lines' in 2nd line
    code_editor.replace_all("Line", "Number", match_case=True)
    assert code_editor.text() == MULTILINE_CODE
