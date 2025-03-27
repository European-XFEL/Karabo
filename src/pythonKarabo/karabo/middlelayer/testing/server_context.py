import os
import shutil
import subprocess
from asyncio import (
    create_subprocess_exec, ensure_future, get_event_loop, sleep, wait_for)
from contextlib import suppress
from typing import Any


class AsyncServerContext:
    STARTUP_TIME: int = 1
    WAIT_TIME: int = 10

    def __init__(self, serverId: str, args: list[str],
                 api: str = "middlelayer", verbose: bool = True):
        assert "/" not in serverId, "Test servers can't have `/` ..."
        if args is None:
            args = []
        self.process: subprocess.Popen[Any] = None
        self.args: list[str] = [f"serverId={serverId}"] + args
        if api == "mdl":
            api = "middlelayer"
        elif api == "bound":
            api = "python"
        self.serverId: str = serverId
        self.api: str = api
        self.verbose: bool = verbose

    async def is_alive(self) -> bool:
        if self.process is None:
            return False
        with suppress(TimeoutError):
            await wait_for(self.process.wait(), 0.5)
        return self.process.returncode is None

    async def __aenter__(self) -> "AsyncServerContext":
        process = await create_subprocess_exec(
            f"karabo-{self.api}server", *self.args,
            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        self.process = process
        if process is not None:

            async def print_stdout():
                while not process.stdout.at_eof():
                    line = await process.stdout.readline()
                    print(line.decode("ascii"), end="")

            async def print_stderr():
                while not process.stderr.at_eof():
                    line = await process.stderr.readline()
                    print(line.decode("ascii"), end="")

            if self.verbose:
                ensure_future(print_stdout())
                ensure_future(print_stderr())

            await sleep(self.STARTUP_TIME)
        alive = await self.is_alive()
        if not alive:
            raise RuntimeError(f"Could not start {self.serverId}")
        await self.waitOnline(self.WAIT_TIME)
        return self

    async def __aexit__(self, exc_type, exc_value, traceback) -> None:
        if self.process is not None:
            self.process.terminate()
            try:
                await wait_for(self.process.wait(), timeout=10)
            except TimeoutError:
                self.process.kill()

        # Always check for logs
        env = os.getenv("KARABO")
        log_path = f"{env}/var/log/{self.serverId}"
        if os.path.exists(log_path):
            shutil.rmtree(log_path)

    async def waitOnline(self, timeout=10):
        loop = get_event_loop()
        instance = loop.instance()
        start = loop.time()
        while True:
            try:
                await wait_for(instance.call(
                    self.serverId, "slotPing", self.serverId, 1, True),
                    timeout=0.2)
                break
            except TimeoutError:
                now = loop.time()
                if now - start > timeout:
                    raise TimeoutError(
                        f"Timeout for server {self.serverId}")
                await sleep(0.5)
