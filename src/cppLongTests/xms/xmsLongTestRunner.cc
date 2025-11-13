/*
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <gtest/gtest.h>

#include <thread>

#include "karabo/net/EventLoop.hh"


/*
 * @brief GoogleTest entry point
 */
int main(int argc, char** argv) {
    std::jthread eventLoopThread = std::jthread([](std::stop_token stoken) { karabo::net::EventLoop::work(); });
    ::testing::InitGoogleTest(&argc, argv);
    auto ret = RUN_ALL_TESTS();
    karabo::net::EventLoop::stop();
    return ret;
}
