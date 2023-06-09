/*
 * $Id: ioProfiler.hh 5260 2012-02-26 22:13:16Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Created on February 25, 2012, 21:04 AM
 *
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

#ifndef KARABO_IO_IOPROFILER_HH
#define KARABO_IO_IOPROFILER_HH

#include <iostream>
#include <karabo/util/Profiler.hh>


#define KARABO_PROFILER(p) karabo::util::Profiler p(#p);
#define KARABO_PROFILER_START(p, name) p.start(name);
#define KARABO_PROFILER_STOP(p) p.stop();
#define KARABO_PROFILER_REPORT(p, name) \
    std::cout << name << ": " << HighResolutionTimer::time2string(p.getTime(name)) << std::endl;


#ifdef KARABO_USE_PROFILER_TABLE1
#define KARABO_PROFILER_TABLE1 KARABO_PROFILER(table1)
#define KARABO_PROFILER_START_TABLE1(name) KARABO_PROFILER_START(table1, name)
#define KARABO_PROFILER_STOP_TABLE1 KARABO_PROFILER_STOP(table1)
#define KARABO_PROFILER_REPORT_TABLE1(name) KARABO_PROFILER_REPORT(table1, name)
#else
#define KARABO_PROFILER_TABLE1
#define KARABO_PROFILER_START_TABLE1(name)
#define KARABO_PROFILER_STOP_TABLE1
#define KARABO_PROFILER_REPORT_TABLE1(name)
#endif


#ifdef KARABO_USE_PROFILER_SCALAR1
#define KARABO_PROFILER_SCALAR1 KARABO_PROFILER(scalar1)
#define KARABO_PROFILER_START_SCALAR1(name) KARABO_PROFILER_START(scalar1, name)
#define KARABO_PROFILER_STOP_SCALAR1 KARABO_PROFILER_STOP(scalar1)
#define KARABO_PROFILER_REPORT_SCALAR1(name) KARABO_PROFILER_REPORT(scalar1, name)
#else
#define KARABO_PROFILER_SCALAR1
#define KARABO_PROFILER_START_SCALAR1(name)
#define KARABO_PROFILER_STOP_SCALAR1
#define KARABO_PROFILER_REPORT_SCALAR1(name)
#endif


#ifdef KARABO_USE_PROFILER_SCALARFILTERBUFFER1
#define KARABO_PROFILER_SCALARFILTERBUFFER1 KARABO_PROFILER(scalarFilterBuffer1)
#define KARABO_PROFILER_START_SCALARFILTERBUFFER1(name) KARABO_PROFILER_START(scalarFilterBuffer1, name)
#define KARABO_PROFILER_STOP_SCALARFILTERBUFFER1 KARABO_PROFILER_STOP(scalarFilterBuffer1)
#define KARABO_PROFILER_REPORT_SCALARFILTERBUFFER1(name) KARABO_PROFILER_REPORT(scalarFilterBuffer1, name)
#else
#define KARABO_PROFILER_SCALARFILTERBUFFER1
#define KARABO_PROFILER_START_SCALARFILTERBUFFER1(name)
#define KARABO_PROFILER_STOP_SCALARFILTERBUFFER1
#define KARABO_PROFILER_REPORT_SCALARFILTERBUFFER1(name)
#endif


#endif /* KARABO_IO_IOPROFILER_HH */
