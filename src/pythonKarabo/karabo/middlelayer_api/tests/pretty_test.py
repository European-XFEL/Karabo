from enum import Enum
from textwrap import dedent
from unittest import TestCase, main
from xml.etree.ElementTree import fromstring, tostring

from IPython.lib.pretty import pretty

from karabo.middlelayer import (
    Proxy, ProxyNode, Hash, Int32, SubProxy, VectorString)

class Node(SubProxy):
    strings = VectorString(key="strings")

    _allattrs = ["strings"]

class Words(Enum):
    a = 5
    b = 7


class Sample(Proxy):
    bin_desc = Int32(
        displayType="""bin|1:a,2:some really long description
                    for testing if line < break works properly,3:c""",
        key="bin_desc")
    bin_int = Int32(displayType="bin", key="bin_int")
    oct_int = Int32(displayType="oct", key="oct_int")
    hex_int = Int32(displayType="hex", key="hex_int")
    enum_int = Int32(enum=Words, key="enum_int")
    strings = VectorString(key="strings")
    node = ProxyNode(cls=Node, key="node")

    _allattrs = ["bin_desc", "bin_int", "oct_int", "hex_int", "enum_int",
                 "strings", "node"]


class Tests(TestCase):
    def assertPretty(self, challenge, response):
        self.assertEqual(pretty(challenge), dedent(response[1:]))

    def assertHtml(self, challenge, response):
        challenge = tostring(fromstring(challenge._repr_html_()),
                             encoding="unicode")
        response = "".join(line.strip().replace("$", "\n")
                           for line in response.split("\n"))
        self.assertEqual(challenge, response)

    def test_none(self):
        s = Sample()
        self.assertPretty(s, """

            bin_desc: None
            bin_int: None
            oct_int: None
            hex_int: None
            enum_int: None
            strings: None
            node: 
                strings: None""")

        self.assertHtml(s, """
            <table>
                <tr>
                    <td style="padding-left:1em">bin_desc</td>
                    <td>None</td>
                </tr><tr>
                    <td style="padding-left:1em">bin_int</td>
                    <td>None</td>
                </tr><tr>
                    <td style="padding-left:1em">oct_int</td>
                    <td>None</td>
                </tr><tr>
                    <td style="padding-left:1em">hex_int</td>
                    <td>None</td>
                </tr><tr>
                    <td style="padding-left:1em">enum_int</td>
                    <td>None</td>
                </tr><tr>
                    <td style="padding-left:1em">strings</td>
                    <td>None</td>
                </tr><tr>
                    <td style="padding-left:1em"><b>node</b></td>
                    <td />
                </tr><tr>
                    <td style="padding-left:2em">strings</td>
                    <td>None</td>
                </tr>
            </table>
        """)

    def test_empty(self):
        s = Sample()
        s._onChanged(Hash("bin_desc", 0, "bin_int", 0, "oct_int", 0,
                          "hex_int", 0, "enum_int", 5, "strings", [],
                          "node.strings", []))
        self.assertPretty(s, """

            bin_desc: {  }
            bin_int: 0b0
            oct_int: 0o0
            hex_int: 0x0
            enum_int: <Words.a>
            strings: []
            node: 
                strings: []""")

        self.assertHtml(s, """
            <table>
                <tr>
                    <td style="padding-left:1em">bin_desc</td>
                    <td />
                </tr><tr>
                    <td style="padding-left:1em">bin_int</td>
                    <td>0b0</td>
                </tr><tr>
                    <td style="padding-left:1em">oct_int</td>
                    <td>0o0</td>
                </tr><tr>
                    <td style="padding-left:1em">hex_int</td>
                    <td>0x0</td>
                </tr><tr>
                    <td style="padding-left:1em">enum_int</td>
                    <td><i>Words.a</i></td>
                </tr><tr>
                    <td style="padding-left:1em">strings</td>
                    <td />
                </tr><tr>
                    <td style="padding-left:1em"><b>node</b></td>
                    <td />
                </tr><tr>
                    <td style="padding-left:2em">strings</td>
                    <td />
                </tr>
            </table>
        """)

    def test_short(self):
        s = Sample()
        s._onChanged(Hash("bin_desc", 2, "bin_int", 5, "oct_int", 10,
                          "hex_int", 20, "enum_int", 5, "strings", ["a"],
                          "node.strings", ["b"]))
        self.assertPretty(s, """

            bin_desc: { a }
            bin_int: 0b101
            oct_int: 0o12
            hex_int: 0x14
            enum_int: <Words.a>
            strings: [a]
            node: 
                strings: [b]""")

        self.assertHtml(s, """
            <table>
                <tr>
                    <td style="padding-left:1em">bin_desc</td>
                    <td>a</td>
                </tr><tr>
                    <td style="padding-left:1em">bin_int</td>
                    <td>0b101</td>
                </tr><tr>
                    <td style="padding-left:1em">oct_int</td>
                    <td>0o12</td>
                </tr><tr>
                    <td style="padding-left:1em">hex_int</td>
                    <td>0x14</td>
                </tr><tr>
                    <td style="padding-left:1em">enum_int</td>
                    <td><i>Words.a</i></td>
                </tr><tr>
                    <td style="padding-left:1em">strings</td>
                    <td>a</td>
                </tr><tr>
                    <td style="padding-left:1em"><b>node</b></td>
                    <td />
                </tr><tr>
                    <td style="padding-left:2em">strings</td>
                    <td>b</td>
                </tr>
            </table>
        """)

    def test_long(self):
        s = Sample()
        s._onChanged(Hash("bin_desc", 6, "bin_int", -1000000000,
                          "oct_int", -2000000000,
                          "hex_int", -1500000000,
                          "enum_int", 5,
                          "strings", [
                            "this", """is a list of long strings which will
                            most likely exceed one line"""],
                          "node.strings", ["another", "list", "this", "time",
                                           "with", "many", "ent>ries", "also",
                                           "exceeding", "one", "line"]))
        self.assertPretty(s, """

            bin_desc:
                { a
                | some really long description
                                for testing if line < break works properly }
            bin_int: 0b-111011100110101100101000000000
            oct_int: 0o-16715312000
            hex_int: 0x-59682f00
            enum_int: <Words.a>
            strings:
                [this,
                 is a list of long strings which will
                                        most likely exceed one line]
            node: 
                strings:
                    [another,
                     list,
                     this,
                     time,
                     with,
                     many,
                     ent>ries,
                     also,
                     exceeding,
                     one,
                     line]""")

        self.assertHtml(s, """
            <table>
                <tr>
                    <td style="padding-left:1em">bin_desc</td>
                    <td>a<br />some really long des
                        cription$                    for test
                        ing if line &lt; break works properly</td>
                </tr><tr>
                    <td style="padding-left:1em">bin_int</td>
                    <td>0b-111011100110101100101000000000</td>
                </tr><tr>
                    <td style="padding-left:1em">oct_int</td>
                    <td>0o-16715312000</td>
                </tr><tr>
                    <td style="padding-left:1em">hex_int</td>
                    <td>0x-59682f00</td>
                </tr><tr>
                    <td style="padding-left:1em">enum_int</td>
                    <td><i>Words.a</i></td>
                </tr><tr>
                    <td style="padding-left:1em">strings</td>
                    <td>this<br />
                        is a list of long strings which wi
                        ll$                            most likely ex
                        ceed one line</td>
                </tr><tr>
                    <td style="padding-left:1em"><b>node</b></td>
                    <td />
                </tr><tr>
                    <td style="padding-left:2em">strings</td>
                    <td>another<br />list<br />this<br />time<br />
                        with<br />many<br />ent&gt;ries<br />also<br />
                        exceeding<br />one<br />line</td>
                </tr>
            </table>
        """)


if __name__ == "__main__":
    main()
