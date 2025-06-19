import logging
import xml.etree.ElementTree as ET
from datetime import UTC, datetime

from .models import Scene

logger = logging.getLogger(__file__)

DATE_FORMAT = "%Y-%m-%d %H:%M:%S"


def datetime_now() -> datetime:
    """Return a datetime object in utc without microseconds"""
    return datetime.now(UTC).replace(microsecond=0)


def datetime_str_now() -> str:
    """Return a datetime string for now for project db format"""
    return datetime_to_str(datetime.now(UTC))


def datetime_to_str(timestamp: datetime | None) -> str:
    """Return a str in project db representation from a datetime"""
    if timestamp is None:
        return ""
    return timestamp.replace(tzinfo=UTC).strftime(DATE_FORMAT)


def datetime_from_str(timestamp: str) -> datetime:
    """Return a datetime in project db representation from a string"""
    return datetime.strptime(timestamp, DATE_FORMAT).replace(tzinfo=UTC)


def date_utc_to_local(dt: datetime) -> str:
    """Convert given `dt` datetime object to the local time string

    Note that is is a different time string as for the others, as it adds
    the UTC offset.
    """
    local_ts = dt.replace(tzinfo=UTC).astimezone()
    return datetime.strftime(local_ts, DATE_FORMAT)


def get_trashed(value: str | None) -> bool:
    """The trashed boolean in XML attributes is stored as string"""
    return False if value is None else value.lower() == "true"


def get_scene_links(scene: Scene) -> list[str]:
    scene_svg = scene.svg_data.strip()
    # Svg Data empty
    if not scene_svg:
        return []
    try:
        root = ET.fromstring(scene_svg)
    except ET.ParseError as e:
        logger.error(
            f"Invalid SVG XML for scene '{scene.name}'({scene.uuid}) "
            f"while processing its scene links: {e}")
        return []

    ns = {
        "svg": "http://www.w3.org/2000/svg",
        "krb": "http://karabo.eu/scene"
    }

    # Find all scene links - svg:rect elements with krb:class="SceneLink"
    # attributes - and add the UUIDs of their target scenes to a list.
    targets_uuids: list[str] = []
    for rect in root.findall(".//svg:rect", ns):
        krb_class = rect.attrib.get(f"{{{ns['krb']}}}class")
        if krb_class == "SceneLink":
            target = rect.attrib.get(f"{{{ns['krb']}}}target")
            target_parts = target.split(":")
            if len(target_parts) < 2:
                logger.error(
                    f"Invalid scene link target, '{target}', found for "
                    f"scene '{scene.name}'. Skipping it.")
                continue
            # A scene can have ":" in its name. Take the UUID after the
            # last ":".
            target_uuid = target_parts[-1].strip()
            if len(target_uuid) < 8:
                logger.error(
                    f"Invalid scene link target UUID, '{target_uuid}', "
                    f"found for scene '{scene.name}'. Skipping it.")
                continue
            targets_uuids.append(target_uuid)
    return targets_uuids
