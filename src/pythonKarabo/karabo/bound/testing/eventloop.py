from collections.abc import Iterator
from threading import Thread

import pytest

from karabo.bound import EventLoop


@pytest.fixture(scope="module")
def eventLoop() -> Iterator[EventLoop]:
    thread: Thread = Thread(target=EventLoop.work, daemon=True)
    thread.start()
    try:
        yield EventLoop
    finally:
        EventLoop.stop()
        thread.join(timeout=10)
        if thread.is_alive():
            raise RuntimeError("Thread still alive after joining ...")
