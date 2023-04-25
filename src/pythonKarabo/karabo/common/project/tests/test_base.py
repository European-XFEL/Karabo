# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
import uuid
from unittest import TestCase

from traits.api import pop_exception_handler, push_exception_handler

from karabo.common.project.api import BaseProjectObjectModel


class TestBase(TestCase):
    def setUp(self):
        push_exception_handler(lambda *args: None, reraise_exceptions=True)

    def tearDown(self):
        pop_exception_handler()

    def test_uuid(self):
        model = BaseProjectObjectModel()

        # check that this doesn't assert
        uuid.UUID(model.uuid)

        with self.assertRaises(ValueError):
            model.uuid = "foo"

    def test_modified_flag(self):
        model = BaseProjectObjectModel(initialized=True)

        model.reset_uuid()
        assert model.modified

        model.modified = False
        assert not model.modified
