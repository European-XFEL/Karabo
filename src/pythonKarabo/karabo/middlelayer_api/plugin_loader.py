from asyncio import coroutine, get_event_loop
import pkg_resources

from .enums import Assignment, AccessLevel
from .hash import String
from .schema import Configurable


class PluginLoader(Configurable):
    pluginDirectory = String(
        displayedName="Plugin Directory",
        description="Directory to search for plugins",
        assignment=Assignment.OPTIONAL, defaultValue="plugins",
        displayType="directory", requiredAccessLevel=AccessLevel.EXPERT)

    pluginNamespace = String(
        displayedName="Plugin Namespace",
        description="Namespace to search for plugins",
        assignment=Assignment.OPTIONAL,
        defaultValue="karabo.middlelayer_device",
        requiredAccessLevel=AccessLevel.EXPERT)

    @coroutine
    def update(self):
        yield from get_event_loop().run_in_executor(
            None, pkg_resources.working_set.add_entry, self.pluginDirectory)
        return list(pkg_resources.iter_entry_points(self.pluginNamespace))
