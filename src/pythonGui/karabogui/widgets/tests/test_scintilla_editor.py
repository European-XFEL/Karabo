from karabogui.widgets.scintilla_editor import CodeBook


def test_scinitilla_editor(gui_app):
    editor = CodeBook(code="from karabo.native import Hash")

    # Basic default features
    assert editor.tabWidth() == 4
    assert editor.autoIndent()
    assert editor.indentationGuides()
    assert editor.getEditorCode() is not None
