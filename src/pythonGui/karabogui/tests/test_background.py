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
import time

from qtpy.QtCore import QObject, QThreadPool, Slot

from karabogui.background import background

TASK_TIME = 0.1


def test_task_background(gui_app):
    """Test that we can run tasks in the background"""

    class Dialog(QObject):
        running = False
        result = None
        finished = False

        @Slot(object)
        def callback(self, future):
            self.result = future.result()
            self.finished = True

        def background_task(self):
            self.running = True
            time.sleep(TASK_TIME)
            return "Karabo"

        def long_task(self):
            self.running = True
            while self.running:
                time.sleep(TASK_TIME)
            return "Karabo"

    dialog = Dialog()
    background(dialog.background_task, callback=dialog.callback)
    QThreadPool.globalInstance().waitForDone(20)
    assert dialog.running
    assert not dialog.finished
    QThreadPool.globalInstance().waitForDone()
    gui_app.processEvents()
    assert dialog.result == "Karabo"
    assert dialog.finished

    dialog = Dialog()
    background(dialog.long_task, callback=dialog.callback)
    QThreadPool.globalInstance().waitForDone(20)
    assert dialog.running
    assert not dialog.finished
    dialog.running = False
    QThreadPool.globalInstance().waitForDone()
    gui_app.processEvents()
    assert dialog.result == "Karabo"


def test_run_background_functor(gui_app):
    """Test that we can cancel a runnable"""
    result = None

    @Slot(object)
    def callback(future):
        nonlocal result
        result = future.result()

    class Functor:
        running = False

        def __call__(self, *args, **kwargs):
            self.running = True
            while self.running:
                time.sleep(TASK_TIME)
            return "Karabo"

        def stop(self):
            self.running = False

    functor = Functor()
    background(functor, callback=callback)
    QThreadPool.globalInstance().waitForDone(20)
    gui_app.processEvents()
    assert functor.running
    QThreadPool.globalInstance().waitForDone(int(TASK_TIME * 1000))
    gui_app.processEvents()
    assert functor.running
    QThreadPool.globalInstance().waitForDone(int(TASK_TIME * 1000))
    gui_app.processEvents()
    assert functor.running
    functor.stop()
    QThreadPool.globalInstance().waitForDone()
    gui_app.processEvents()
    assert not functor.running
    assert result == "Karabo"


def test_run_background_exception(gui_app):
    """Test that we can redirect an exception to the main thread"""

    exception = None

    @Slot(object)
    def callback(future):
        nonlocal exception
        try:
            assert future.exception() is not None
            future.result()
        except RuntimeError as e:
            exception = e

    class Functor:
        running = False

        def __call__(self, *args, **kwargs):
            self.running = True
            time.sleep(TASK_TIME)
            raise RuntimeError("Device not installed")

    functor = Functor()
    background(functor, callback=callback)
    QThreadPool.globalInstance().waitForDone()
    gui_app.processEvents()
    assert exception is not None
    assert isinstance(exception, RuntimeError)
    assert str(exception) == "Device not installed"
