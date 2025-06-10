from datetime import UTC, datetime

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
