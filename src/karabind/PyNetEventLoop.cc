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

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <karabo/net/EventLoop.hh>

#include "HandlerWrap.hh"
#include "Wrapper.hh"

namespace py = pybind11;
using namespace karabo::net;
using namespace karabind;


void exportPyNetEventLoop(py::module_& m) {
    py::class_<EventLoop> el(m, "EventLoop", "EventLoop is a singleton class wrapping Boost ASIO functionality.");

    el.def_static(
          "work",
          []() {
              py::gil_scoped_release release;
              EventLoop::work();
          },
          "Start the event loop and block until EventLoop::stop() is called.\n\n"
          "The system signals SIGINT and SIGTERM will be caught and trigger the\n"
          "following actions:\n"
          " - a signal handler set via setSignalHandler is called,\n"
          " - and the event loop is stopped.");

    el.def_static("run", []() {
        py::gil_scoped_release release;
        EventLoop::run();
    });

    el.def_static("stop", &EventLoop::stop);

    el.def_static("addThread", &EventLoop::addThread, py::arg("nThreads") = 1);

    el.def_static("removeThread", &EventLoop::removeThread, py::arg("nThreads") = 1);

    el.def_static("getNumberOfThreads", &EventLoop::getNumberOfThreads);

    el.def_static(
          "post",
          [](const py::object& callable, const py::object& delay) {
              // Wrap with GIL since callable's reference count will increase:
              HandlerWrap<> wrapped(callable, "EventLoop.post");
              unsigned int delayMillisec = 0;
              if (delay != py::none()) {
                  const double delaySeconds = delay.cast<double>(); // needs GIL
                  delayMillisec = static_cast<unsigned int>(delaySeconds * 1.e3);
              }
              // No GIL when entering pure C++:
              py::gil_scoped_release release;
              EventLoop::post(std::move(wrapped), delayMillisec);
          },
          py::arg("callable"), py::arg("delay") = py::none(),
          "Post 'callable' for execution in background. Needs EventLoop to run/work.\n"
          "If 'delay' is not None, execution is postponed by given seconds.");

    el.def_static(
          "setSignalHandler",
          [](const py::object& handler) {
              HandlerWrap<int> wrapped(handler, "EventLoop.setSignalHandler");
              // No GIL when entering pure C++:
              py::gil_scoped_release release;
              EventLoop::setSignalHandler(wrapped);
          },
          py::arg("handler"),
          "Set handler to be called if a system signal is caught in 'work',\n"
          "'handler' has to take the signal value as argument.");
}
