from karabo.common.project.api import (
    MacroModel, ProjectModel, walk_traits_object
)


def test_project_modified_obj_check():
    modified = []

    def find_modified(obj):
        if obj.modified:
            modified.append(obj)

    macro = MacroModel(code="print('hello world')")
    proj = ProjectModel(macros=[macro])

    walk_traits_object(proj, find_modified)
    assert len(modified) == 0

    macro.code = "print('I was modified!')"
    walk_traits_object(proj, find_modified)
    assert modified == [macro]
