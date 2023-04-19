/*
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */
#include "testRunner.hh"

#include "boost/thread.hpp"
#include "karabo/net/EventLoop.hh"

int main(int argc, char* argv[]) {
    boost::thread eventLoopThread = boost::thread(karabo::net::EventLoop::work);
    return run_test(argc, argv);
}
