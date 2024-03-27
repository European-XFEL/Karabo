# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
import pytest

from karabogui.events import (
    KaraboEvent, broadcast_event, register_for_broadcasts,
    unregister_from_broadcasts)
from karabogui.singletons.mediator import Mediator
from karabogui.testing import singletons
from karabogui.util import process_qt_events


def test_basic_mediator(gui_app):
    instanceId = ""

    def _show_device(data):
        nonlocal instanceId
        instanceId = data.get("instanceId", "")

    event_map = {
        KaraboEvent.ShowDevice: _show_device,
    }

    mediator = Mediator()
    assert len(mediator._listeners.keys()) == 0

    with singletons(mediator=mediator):
        register_for_broadcasts(event_map)
        assert KaraboEvent.ShowDevice in mediator._listeners.keys()
        assert len(mediator._listeners.keys()) == 1

        assert instanceId == ""
        broadcast_event(KaraboEvent.ShowDevice,
                        data={"instanceId": "Marty"})
        process_qt_events(timeout=10)
        assert instanceId == "Marty"

        # Unregister once
        unregister_from_broadcasts(event_map)
        assert len(mediator._listeners.keys()) == 0

        # Unregister twice, no harm
        unregister_from_broadcasts(event_map)
        assert len(mediator._listeners.keys()) == 0


if __name__ == "__main__":
    pytest.main()
