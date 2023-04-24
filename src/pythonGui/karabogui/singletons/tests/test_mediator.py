# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from unittest import main

from karabogui.events import (
    KaraboEvent, broadcast_event, register_for_broadcasts,
    unregister_from_broadcasts)
from karabogui.singletons.mediator import Mediator
from karabogui.testing import GuiTestCase, singletons


class TestMediator(GuiTestCase):
    def test_basic_mediator(self):
        instanceId = ""

        def _show_device(data):
            nonlocal instanceId
            instanceId = data.get("instanceId", "")

        event_map = {
            KaraboEvent.ShowDevice: _show_device,
        }

        mediator = Mediator()
        self.assertEqual(len(mediator._listeners.keys()), 0)

        with singletons(mediator=mediator):
            register_for_broadcasts(event_map)
            self.assertIn(KaraboEvent.ShowDevice, mediator._listeners.keys())
            self.assertEqual(len(mediator._listeners.keys()), 1)

            self.assertEqual(instanceId, "")
            broadcast_event(KaraboEvent.ShowDevice,
                            data={"instanceId": "Marty"})
            self.process_qt_events()
            self.assertEqual(instanceId, "Marty")

            # Unregister once
            unregister_from_broadcasts(event_map)
            self.assertEqual(len(mediator._listeners.keys()), 0)

            # Unregister twice, no harm
            unregister_from_broadcasts(event_map)
            self.assertEqual(len(mediator._listeners.keys()), 0)


if __name__ == "__main__":
    main()
