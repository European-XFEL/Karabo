import os
import shutil
import subprocess
from threading import Thread
from time import sleep, time

from karabo.bound import DeviceClient

from .utils import create_instanceId


class ServerContext:
    PROCESS_TIME: int = 1
    WAIT_TIME: int = 10

    def __init__(self, serverId: str, args: list[str] = None,
                 api: str = "bound",
                 client: DeviceClient | None = None,
                 verbose: bool = True):
        # We take care that we can delete logs later
        assert "/" not in serverId, "Test servers can't have `/` ..."
        if args is None:
            args = []
        self.process: subprocess.Popen | None = None
        self.args: list[str] = [f"serverId={serverId}"] + args
        if api == "mdl":
            api = "middlelayer"
        elif api == "bound":
            api = "python"
        self.serverId: str = serverId
        self.api: str = api
        if client is None:
            client = DeviceClient(create_instanceId())
        self.client = client
        self.verbose: bool = verbose

    def is_alive(self) -> bool:
        if self.process is None:
            return False
        return self.process.poll() is None

    def __enter__(self) -> 'ServerContext':
        process = subprocess.Popen(
            [f"karabo-{self.api}server", *self.args],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE)
        if process is not None:

            def print_stdout():
                while True:
                    line = process.stdout.readline()
                    if not line:
                        break
                    print(line.decode("ascii"), end="")

            def print_stderr():
                while True:
                    line = process.stderr.readline()
                    if not line:
                        break
                    print(line.decode("ascii"), end="")

            if self.verbose:
                Thread(target=print_stdout, daemon=True).start()
                Thread(target=print_stderr, daemon=True).start()

            self.process = process
            sleep(self.PROCESS_TIME)
        alive = self.is_alive()
        if not alive:
            raise RuntimeError(f"Could not start {self.serverId}")
        self.waitOnline(self.WAIT_TIME)
        return self

    def remote(self):
        """Convenience handle to retrieve device client"""
        return self.client

    def __exit__(self, exc_type, exc_val, exc_tb):
        if self.process is not None:
            try:
                self.process.terminate()
                self.process.wait(10)
            except Exception:
                # Ensure the process has fully terminated
                self.process.kill()

        # Always check for logs
        env = os.getenv("KARABO")
        log_path = f"{env}/var/log/{self.serverId}"
        if os.path.exists(log_path):
            shutil.rmtree(log_path)

    def waitOnline(self, timeout=10):
        start = time()
        while True:
            online, _ = self.client.exists(self.serverId)
            if online:
                break
            now = time()
            if now - start > timeout:
                raise TimeoutError(
                    f"Timeout for server {self.serverId}")
            sleep(0.5)
