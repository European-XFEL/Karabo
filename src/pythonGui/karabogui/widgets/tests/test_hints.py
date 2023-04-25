# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from karabogui.widgets.hints import ElidingLabel


def test_elided_label(gui_app):
    label = ElidingLabel()
    label.setMaximumWidth(100)
    label.setText("Karabo")
    assert not label.isElided()
    label.setText("".join(15 * ["Karabo"]))
    assert label.isElided()
    label.setText("ThisIsKarabo")
    assert not label.isElided()
