from ..models import Scene
from ..utils import (
    datetime_from_str, datetime_now, datetime_to_str, get_scene_links)

SCENE_UUID = "73d31361-b019-4d0c-9fd5-aa0acd536bb8"
SCENE_NAME = "das_Bild"

SCENE_HEADER = (
    f'<xml item_type="scene" uuid="{SCENE_UUID}" '
    f'simple_name="{SCENE_NAME}">'
    '<svg:svg xmlns:svg="http://www.w3.org/2000/svg" '
    'xmlns:krb="http://karabo.eu/scene" height="768" width="1024" '
    'krb:uuid="73d31361-b019-4d0c-9fd5-aa0acd536bb8" krb:version="2">')

SCENE_FOOTER = '</svg:svg></xml>'


def test_date_tools_round_trip():
    """Test the datetime tools with a roundtrip"""
    dt = datetime_now()
    ts = datetime_to_str(dt)
    assert datetime_to_str(dt) == ts
    assert datetime_from_str(ts) == dt


def test_get_scene_links():
    """Test the extraction of UUIDs of target scenes of scene links
    in a scene"""

    # Invalid scene links must be ignored
    svg = (
        f'{SCENE_HEADER}'
        '<svg:rect krb:class="SceneLink" '
        f'krb:target="scene_tgt:091:" />'  # Missing the UUID
        '<svg:rect krb:class="SceneLink" '
        f'krb:target="{SCENE_UUID}" />'  # Missing the target name
        f'{SCENE_FOOTER}')
    links = get_scene_links(
        Scene(uuid=SCENE_UUID, name=f"{SCENE_NAME}", svg_data=svg))
    assert len(links) == 0, f"Expected to find no scene link, got {len(links)}"

    svg = (
        f'{SCENE_HEADER}'
        '<svg:rect krb:class="Scene__Link" '  # Wrong  krb:class
        f'krb:target="scene_tgt:{SCENE_UUID}" />'
        f'{SCENE_FOOTER}')
    links = get_scene_links(
        Scene(uuid=SCENE_UUID, name=f"{SCENE_NAME}", svg_data=svg))
    assert len(links) == 0, f"Expected to find no scene link, got {len(links)}"

    # Multiple scene links must be returned
    svg = (
        f'{SCENE_HEADER}'
        '<svg:rect krb:class="SceneLink" '
        f'krb:target="scene_tgt:{SCENE_UUID}" />'
        '<svg:rect krb:class="SceneLink" '
        f'krb:target="scene_tgt:{SCENE_UUID}" />'
        f'{SCENE_FOOTER}')
    links = get_scene_links(
        Scene(uuid=SCENE_UUID, name=f"{SCENE_NAME}", svg_data=svg))
    assert len(links) == 2, f"Expected to find 2 scene links, got {len(links)}"
    assert links[0] == SCENE_UUID
    assert links[1] == SCENE_UUID
