# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.

from karabo.bound import Hash, Logger


def test_logging_config_rules():
    # Python API: 'Logger.logXXX(message, category)
    # We are chatty in this test
    # But the idea is to only see OKs and never ERROR
    # There is no ASSERT unfortunately, so this test needs visual inspection
    config = Hash("level", "DEBUG")
    Logger.configure(config)
    Logger.logDebug("ERROR")

    Logger.useConsole()
    Logger.logDebug("OK")
    Logger.logDebug("OK", "a1")
    Logger.logDebug("OK", "a1.a2")
    Logger.logInfo("OK")
    Logger.logInfo("OK", "a1")
    Logger.logInfo("OK", "a1.a2")

    Logger.reset()
    Logger.logDebug("ERROR")
    Logger.logDebug("ERROR", "a1")
    Logger.logDebug("ERROR", "a1.a2")
    Logger.logInfo("ERROR")
    Logger.logInfo("ERROR", "a1")
    Logger.logInfo("ERROR", "a1.a2")

    Logger.useConsole("a1")
    Logger.logDebug("ERROR")
    Logger.logDebug("OK", "a1")
    Logger.logDebug("OK", "a1.a2")
    Logger.logInfo("ERROR")
    Logger.logInfo("OK", "a1")
    Logger.logInfo("OK", "a1.a2")

    Logger.setLevel("INFO")
    Logger.logDebug("ERROR")
    Logger.logDebug("ERROR", "a1")
    Logger.logDebug("ERROR", "a1.a2")
    Logger.logInfo("ERROR")
    Logger.logInfo("OK", "a1")
    Logger.logInfo("OK", "a1.a2")

    Logger.setLevel("WARN")
    Logger.logDebug("ERROR")
    Logger.logDebug("ERROR", "a1")
    Logger.logDebug("ERROR", "a1.a2")
    Logger.logInfo("ERROR")
    Logger.logInfo("ERROR", "a1")
    Logger.logInfo("ERROR", "a1.a2")


def test_logging_config_more():
    Logger.reset()
    config = Hash("level", "INFO")
    Logger.configure(config)
    Logger.useConsole()
    Logger.useFile("a1", False)  # do not inherit appenders from parents
    Logger.logDebug("ERROR")
    Logger.logDebug("ERROR", "a1")
    Logger.logDebug("ERROR", "a1.a2")
    Logger.logInfo("CONSOLE-OK")
    Logger.logInfo("FILE-OK", "a1")
    Logger.logInfo("FILE-OK", "a1.a2")


def test_logging_last_message():
    # Reset (remove all appenders) and clear the cache
    Logger.reset()

    content = Logger.getCachedContent(10)
    assert len(content) == 0

    # Setup the Logger
    maxmsgs = 20
    config = Hash("level", "INFO", "cache.maxNumMessages", maxmsgs)
    Logger.configure(config)
    Logger.useCache()

    # calling Logger.getCachedContent before logging will get an empty vector
    content = Logger.getCachedContent(10)
    assert len(content) == 0

    # log something
    for i in range(100):
        Logger.logDebug(f"This should not be logged - {i}", "VERBOSE_STUFF")
        Logger.logInfo(f"line - {i}", "INFORMATIVE_STUFF")

    # get the last 10 entries
    content = Logger.getCachedContent(10)
    assert len(content) == 10

    index = 90
    for entry in content:
        assert entry.has("timestamp") is True
        assert entry.get("category") == "INFORMATIVE_STUFF"
        assert entry.get("type") == "INFO"
        expected = f'line - {index}'
        index += 1
        assert expected == entry.get("message")

    # One can request more than the max config["cache.maxNumMessages"]
    # but will not get more than that
    content = Logger.getCachedContent(200)
    assert maxmsgs == len(content)
    index = 100 - maxmsgs
    for entry in content:
        # check that the timestamp is in, but do not check
        assert entry.has("timestamp") is True
        assert "INFORMATIVE_STUFF" == entry.get("category")
        assert "INFO" == entry.get("type")
        expected = f'line - {index}'
        assert expected == entry.get("message")
        index += 1
