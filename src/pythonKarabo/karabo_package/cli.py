import ast
from IPython.external.qt_loaders import loaded_api
from IPython.lib.inputhook import inputhook_manager
import IPython
import asyncio
import asyncio.base_events


class RewriteYieldFrom(ast.NodeTransformer):
    def visit_YieldFrom(self, node):
        ret = ast.Call(ast.Name("@", ast.Load()), [node.value], [], None, None)
        ret.lineno = node.lineno
        ret.col_offset = node.col_offset
        ast.fix_missing_locations(ret)
        return ret


    def visit_FunctionDef(self, node):
        return node


if loaded_api() is not None:
    from PyQt4 import QtCore
    import quamash


    def yield_from(coro):
        loop = QtCore.QEventLoop()
        future = asyncio.async(coro)
        future.add_done_callback(lambda future: loop.quit())
        loop.exec()
        return future.result()


    def set_event_loop():
        loop = quamash.QEventLoop(QtCore.QCoreApplication.instance())
        asyncio.set_event_loop(loop)
        loop._QEventLoop__is_running = True

else:
    from karabo.eventloop import EventLoop


    class InputEventLoop(EventLoop):
        def __init__(self):
            super().__init__()
            self._clock_resolution = 0.1 # the resolution of inputhooks


        def hook(self):
            self.call_soon(lambda: None)
            while self._ready:
                self._run_once()
            return 0


    def set_event_loop():
        loop = InputEventLoop()
        asyncio.set_event_loop(loop)
        inputhook_manager.set_inputhook(loop.hook)
        loop._running = True


    def yield_from(coro):
        loop = asyncio.get_event_loop()
        loop._running = False
        try:
            return loop.run_until_complete(coro)
        finally:
            loop._running = True


def load_ipython_extension(ip, set_loop=True):
    ip.ast_transformers.append(RewriteYieldFrom())
    ip.user_global_ns["@"] = yield_from
    if set_loop:
        set_event_loop()


if __name__ == "__main__":
    ip = IPython.get_ipython()
    load_ipython_extension(ip)
