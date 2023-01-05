from karabogui.widgets.scintilla_api import create_symbols
from karabogui.widgets.scintilla_editor import CodeBook


def test_scintilla_editor(gui_app):
    symbols = create_symbols()
    assert len(symbols) > 150

    editor = CodeBook(code="from karabo.native import Hash", use_api=False)
    # Basic default features
    assert editor.tabWidth() == 4
    assert editor.autoIndent()
    assert editor.indentationGuides()
    assert editor.getEditorCode() is not None
