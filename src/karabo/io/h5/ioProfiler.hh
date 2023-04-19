/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Created on February 25, 2012, 21:04 AM
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */

#ifndef KARABO_IO_H5_IOPROFILER_HH
#define KARABO_IO_H5_IOPROFILER_HH

#include <iostream>
#include <karabo/util/TimeProfiler.hh>

#ifdef KARABO_IO_TRACE
#define KRB_IO_DEBUG_TRACE \
    if (0)                 \
        ;                  \
    else std::cerr
#else
#define KRB_IO_DEBUG_TRACE \
    if (1)                 \
        ;                  \
    else std::cerr
#endif


#define KARABO_PROFILER(p) karabo::util::TimeProfiler p(#p);
#define KARABO_PROFILER_START(p, name) p.start(name);
#define KARABO_PROFILER_STOP(p) p.stop();
#define KARABO_PROFILER_REPORT(p, name) std::clog << name << ": " << p.getPeriod(name).getDuration() << std::endl;

// format(p.getTime(name), "%s.%l") << std::endl;


#define KARABO_USE_PROFILER_TABLE1

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

#define KARABO_USE_PROFILER_SCALAR1

#ifdef KARABO_USE_PROFILER_SCALAR1
#define KARABO_PROFILER_SCALAR1 KARABO_PROFILER(scalar1);
#define KARABO_PROFILER_START_SCALAR1(name) KARABO_PROFILER_START(scalar1, name);
#define KARABO_PROFILER_STOP_SCALAR1 KARABO_PROFILER_STOP(scalar1);
#define KARABO_PROFILER_REPORT_SCALAR1(name) KARABO_PROFILER_REPORT(scalar1, name);
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
