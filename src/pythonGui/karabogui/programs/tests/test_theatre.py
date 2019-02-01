from ..theatre import DEVSCENE_PROG


def test_device_link_regex():
    match = DEVSCENE_PROG.match('SCOPE_GROUP_COMPONENT-2/TYPE/MEMBER|scene')
    assert (match and all(match.groups()))
    match = DEVSCENE_PROG.match('random-string')
    assert not (match and all(match.groups()))
