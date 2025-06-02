import datetime

ISO8601_FORMAT = "%Y-%m-%d %H:%M:%S"


def utc_to_local(timestamp: datetime.datetime | None) -> str:
    """Format a datetime object to a local str representation

    XXX: This is borrowed from config_db and should move to common.
    """
    if timestamp is None:
        return ""
    return timestamp.replace(
        tzinfo=datetime.UTC).astimezone().strftime(ISO8601_FORMAT)
