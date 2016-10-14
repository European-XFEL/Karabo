from karabo.common.project.api import MacroModel


def test_instance_id_property():
    macro = MacroModel(title='foo', project_name='proj')

    assert macro.instance_id == 'Macro-proj-foo'
    macro.title = 'bar'
    assert macro.instance_id == 'Macro-proj-bar'
    macro.project_name = 'jorp'
    assert macro.instance_id == 'Macro-jorp-bar'
