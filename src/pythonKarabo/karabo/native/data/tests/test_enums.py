from ..enums import AccessLevel, Assignment, MetricPrefix, Unit


def test_access_level():
    """Test the ordering of the access level"""
    assert AccessLevel.EXPERT > AccessLevel.OPERATOR
    assert AccessLevel.OPERATOR < AccessLevel.EXPERT
    assert AccessLevel.OPERATOR > AccessLevel.OBSERVER
    assert AccessLevel.OBSERVER < AccessLevel.OPERATOR
    assert AccessLevel.EXPERT == 2
    assert AccessLevel.EXPERT.value == 2
    assert AccessLevel.EXPERT != 5


def test_assigment():
    """Test the assignment comparison"""
    assert Assignment.OPTIONAL < 1
    assert Assignment.OPTIONAL == 0
    assert Assignment.MANDATORY > 0
    assert Assignment.INTERNAL != 0


def test_unit_and_prefix():
    assert Unit.METER == "m"
    assert Unit.AMPERE == "A"
    assert Unit.AMPERE.value == "A"
    assert MetricPrefix.MICRO == "u"
    assert MetricPrefix.MILLI == "m"
