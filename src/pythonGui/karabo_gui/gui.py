#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 3, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from karabo_gui.singletons.api import get_manager, get_panel_wrangler


def init_gui(app, splash):
    """ Initialize the GUI.

    Imports are being done inside this function to avoid delaying the display
    of the splash screen. We want the user to know that something is happening
    soon after they launch the GUI.
    """
    # XXX: gui_registry_loader is only imported for its side-effects
    import karabo_gui.gui_registry_loader  # noqa
    import karabo_gui.icons as icons
    import numpy

    numpy.set_printoptions(suppress=True, threshold=10)
    icons.init()

    app.setStyleSheet("QPushButton { text-align: left; padding: 5px; }")

    # Initialize the Manager singleton
    get_manager()
    # Initialize the PanelWrangler and attach the splash screen
    get_panel_wrangler().use_splash_screen(splash)
