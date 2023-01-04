from karabogui.widgets.scintilla_editor import CodeBook


def test_scinitilla_editor(gui_app):
    editor = CodeBook()

    # Basic default features
    assert editor.tabWidth() == 4
    assert editor.autoIndent()
    assert editor.indentationGuides()
