import uuid

from karabo.common.project.api import MacroModel


def test_instance_id_property():
    UUID = str(uuid.uuid4())
    macro = MacroModel(simple_name='foo', uuid=UUID, revision=2)

    assert macro.instance_id == 'Macro-foo-{}-2'.format(UUID)
    macro.simple_name = 'bar'
    assert macro.instance_id == 'Macro-bar-{}-2'.format(UUID)
    macro.revision = 4
    assert macro.instance_id == 'Macro-bar-{}-4'.format(UUID)
