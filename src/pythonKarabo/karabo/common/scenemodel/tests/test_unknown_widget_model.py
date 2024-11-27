from xml.etree import ElementTree

from karabo.common.scenemodel.const import NS_SVG
from karabo.common.scenemodel.model import (
    UnknownWidgetDataModel, UnknownXMLDataModel, __unknown_widget_data_reader,
    __unknown_widget_data_writer)
from karabo.common.scenemodel.shapes import LineModel

arrow = ElementTree.fromstring("""
<svg:g xmlns:krb="http://karabo.eu/scene"
xmlns:svg="http://www.w3.org/2000/svg" krb:class="ArrowPolygonModel" x="80"
y="60" width="70" height="40">
    <svg:line stroke="#000000" stroke-opacity="1.0" stroke-linecap="butt"
    stroke-dashoffset="0.0" stroke-width="1.0" stroke-dasharray=""
    stroke-style="1" stroke-linejoin="miter" stroke-miterlimit="4.0"
    fill="none" x1="80" y1="60" x2="150" y2="100" />
    <svg:polygon points="150,100 140,97 143,92 " stroke="#000000"
    stroke-opacity="1.0" stroke-linecap="butt" stroke-dashoffset="0.0"
    stroke-width="1.0" stroke-dasharray="" stroke-style="1"
    stroke-linejoin="miter" stroke-miterlimit="4.0" fill="#000000"
    fill-opacity="1.0" />
            </svg:g>
""")

parent = ElementTree.fromstring("""
<svg:svg xmlns:krb="http://karabo.eu/scene"
 xmlns:svg="http://www.w3.org/2000/svg"
 krb:version="2" krb:uuid="2ea69b7b-7669-4266-afd1-b428184f6cb3" height="500"
 width="700" x="10" y="20">
 </svg:svg>""")


def test_unknown_widget_reader_writer():
    """Reading and writing element with sub-element """
    # Read the element as model
    model = __unknown_widget_data_reader(arrow)
    assert isinstance(model, UnknownWidgetDataModel)

    children = getattr(model, "children", [])
    assert len(children) == 2
    assert isinstance(children[0], LineModel)
    assert isinstance(children[1], UnknownXMLDataModel)

    # Write the model back to element.
    new_element = __unknown_widget_data_writer(model, parent=parent)
    assert len(new_element) == 2
    line, polygon = new_element

    assert line.tag == NS_SVG + "line"
    assert polygon.tag == NS_SVG + "polygon"
    assert line.get("x1") == "80"
    assert line.get("y1") == "60"
    assert line.get("x2") == "150"
    assert line.get("y2") == "100"

    assert polygon.get("points") == "150,100 140,97 143,92 "
