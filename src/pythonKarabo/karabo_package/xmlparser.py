
from xml.etree import ElementTree


class Parser(ElementTree.XMLParser):
    def _fixtext(self, text):
        """Don't encode text into ASCII, that breaks distinction between
        STRING and VECTOR_CHAR in karabo"""
        return text


class TreeBuilder(ElementTree.TreeBuilder):
    def _flush(self):
        super(TreeBuilder, self)._flush()
        if self._last is not None and self._last.text is None:
            self._last.text = ''


def parse(source):
    return ElementTree.parse(source, parser=Parser(target=TreeBuilder()))
