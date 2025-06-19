# This file is intended to be used together with Karabo:
#
# http://www.karabo.eu
#
# IF YOU REQUIRE ANY LICENSING AND COPYRIGHT TERMS, PLEASE ADD THEM HERE.
# Karabo itself is licensed under the terms of the MPL 2.0 license.
import pytest

from karabo.middlelayer.testing import KaraboTestLoopPolicy


@pytest.fixture(scope="session")
def event_loop_policy():
    return KaraboTestLoopPolicy()
