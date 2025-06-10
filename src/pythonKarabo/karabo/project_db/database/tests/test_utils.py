from ..utils import datetime_from_str, datetime_now, datetime_to_str


def test_date_tools_round_trip():
    """Test the datetime tools with a roundtrip"""
    dt = datetime_now()
    ts = datetime_to_str(dt)
    assert datetime_to_str(dt) == ts
    assert datetime_from_str(ts) == dt
