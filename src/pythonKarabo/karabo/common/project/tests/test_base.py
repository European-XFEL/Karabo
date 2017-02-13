import uuid

from nose.tools import assert_raises
from traits.api import push_exception_handler, pop_exception_handler

from karabo.common.project.api import BaseProjectObjectModel


def setUp():
    push_exception_handler(lambda *args: None, reraise_exceptions=True)


def tearDown():
    pop_exception_handler()


def test_uuid():
    model = BaseProjectObjectModel()

    # check that this doesn't assert
    uuid.UUID(model.uuid)

    with assert_raises(ValueError):
        model.uuid = 'foo'


def test_modified_flag():
    model = BaseProjectObjectModel(initialized=True)

    model.reset_uuid()
    assert model.modified

    model.modified = False
    assert not model.modified
