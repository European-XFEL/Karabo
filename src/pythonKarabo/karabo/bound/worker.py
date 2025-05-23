# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.

import threading
import traceback
from collections.abc import Callable
from typing import Self


class Worker(threading.Thread):
    def __init__(self,
                 callback: Callable[[], None] | None = None,
                 single_shot: bool = False,
                 timeout: int | float = 1000,
                 daemon: bool = True) -> None:
        """Constructs the Worker thread, that is by default a daemon thread.

        Please note that daemon threads may not release resources
        properly when stopped abruptly.

        Note: `timeout` is in milliseconds.
        """
        super().__init__(daemon=daemon)
        self.callback: Callable[[], None] | None = callback
        self.single_shot: bool = single_shot
        self.running: bool = False
        self.aborted: bool = False
        self.suspended: bool = False
        self.cv: threading.Condition = threading.Condition()
        if not timeout > 0:
            raise RuntimeError("Timeout needs to large zero.")
        self.timeout: int | float = timeout

    def set(self, callback: Callable[[], None],
            timeout: int | float = 1000) -> None:
        self.callback = callback
        if not timeout > 0:
            raise RuntimeError("Timeout needs to large zero.")
        self.timeout = timeout

    def setSingleShot(self, single_shot: bool) -> None:
        self.single_shot = single_shot

    def setTimeout(self, timeout: int | float = 1000) -> None:
        if not timeout > 0:
            raise RuntimeError("Timeout needs to large zero.")
        self.timeout = timeout

    def is_running(self) -> bool:
        return self.running

    def run(self) -> None:
        self.running = True
        self.aborted = False
        self.suspended = False
        try:
            assert self.callback is not None, "Callback cannot be None"
            while not self.aborted:
                if not self.running:
                    break

                if self.suspended:
                    with self.cv:
                        while self.suspended:
                            self.cv.wait()
                        if self.aborted or not self.running:
                            break
                    continue

                with self.cv:
                    self.cv.wait(self.timeout / 1000.0)

                if self.suspended:
                    continue

                if self.running and self.callback:
                    self.callback()
                    if self.single_shot:
                        break
        except Exception:
            traceback.print_exc()
        finally:
            self.running = False

    def start(self) -> Self:
        if not self.running:
            self.suspended = False
            super().start()

        if self.suspended:
            with self.cv:
                self.suspended = False
                self.cv.notify()

        return self

    def stop(self) -> Self:
        if self.running:
            with self.cv:
                self.running = False
                self.suspended = False
                self.cv.notify()
        return self

    def abort(self) -> Self:
        self.aborted = True
        self.running = False
        if self.suspended:
            with self.cv:
                self.suspended = False
                self.cv.notify()
        return self

    def pause(self) -> None:
        if not self.suspended:
            with self.cv:
                self.suspended = True
                self.cv.notify()
