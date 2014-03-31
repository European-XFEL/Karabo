from xml.etree import ElementTree


class Parser(ElementTree.XMLParser):
    def _fixtext(self, text):
        """Don't encode text into ASCII, that breaks distinction between
        STRING and VECTOR_CHAR in karabo"""
        return text


def parse(source):
    return ElementTree.parse(source,
                             parser=Parser(target=ElementTree.TreeBuilder()))