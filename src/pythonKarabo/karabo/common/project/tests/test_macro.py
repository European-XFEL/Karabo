import uuid

from karabo.common.project.api import MacroModel


def test_instance_id_property():
    UUID = str(uuid.uuid4())
    macro = MacroModel(simple_name='foo', uuid=UUID)

    assert macro.instance_id == 'Macro-foo-{}'.format(UUID)
    macro.simple_name = 'bar'
    assert macro.instance_id == 'Macro-bar-{}'.format(UUID)
