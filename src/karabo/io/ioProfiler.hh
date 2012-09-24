/*
 * $Id: ioProfiler.hh 5260 2012-02-26 22:13:16Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 * 
 * Created on February 25, 2012, 21:04 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef EXFEL_IO_IOPROFILER_HH
#define	EXFEL_IO_IOPROFILER_HH

#include <karabo/util/Profiler.hh>
#include <iostream>

#ifdef DEBUG_IO_HDF5_TABLE
#define tracer if(0); else std::cerr
#else 
#define tracer if(1); else std::cerr
#endif



#define EXFEL_PROFILER(p) exfel::util::Profiler p(#p);
#define EXFEL_PROFILER_START(p,name) p.start(name);
#define EXFEL_PROFILER_STOP(p) p.stop();
#define EXFEL_PROFILER_REPORT(p, name) std::cout << name << ": " << HighResolutionTimer::time2string (p.getTime(name)) << std::endl;


#ifdef EXFEL_USE_PROFILER_TABLE1
#define EXFEL_PROFILER_TABLE1 EXFEL_PROFILER(table1)
#define EXFEL_PROFILER_START_TABLE1(name) EXFEL_PROFILER_START(table1, name)
#define EXFEL_PROFILER_STOP_TABLE1 EXFEL_PROFILER_STOP(table1)
#define EXFEL_PROFILER_REPORT_TABLE1(name) EXFEL_PROFILER_REPORT(table1, name)
#else
#define EXFEL_PROFILER_TABLE1
#define EXFEL_PROFILER_START_TABLE1(name)
#define EXFEL_PROFILER_STOP_TABLE1 
#define EXFEL_PROFILER_REPORT_TABLE1(name) 
#endif


#ifdef EXFEL_USE_PROFILER_SCALAR1
#define EXFEL_PROFILER_SCALAR1 EXFEL_PROFILER(scalar1)
#define EXFEL_PROFILER_START_SCALAR1(name) EXFEL_PROFILER_START(scalar1, name)
#define EXFEL_PROFILER_STOP_SCALAR1 EXFEL_PROFILER_STOP(scalar1)
#define EXFEL_PROFILER_REPORT_SCALAR1(name) EXFEL_PROFILER_REPORT(scalar1, name)
#else
#define EXFEL_PROFILER_SCALAR1
#define EXFEL_PROFILER_START_SCALAR1(name)
#define EXFEL_PROFILER_STOP_SCALAR1 
#define EXFEL_PROFILER_REPORT_SCALAR1(name) 
#endif


#ifdef EXFEL_USE_PROFILER_SCALARFILTERBUFFER1
#define EXFEL_PROFILER_SCALARFILTERBUFFER1 EXFEL_PROFILER(scalarFilterBuffer1)
#define EXFEL_PROFILER_START_SCALARFILTERBUFFER1(name) EXFEL_PROFILER_START(scalarFilterBuffer1, name)
#define EXFEL_PROFILER_STOP_SCALARFILTERBUFFER1 EXFEL_PROFILER_STOP(scalarFilterBuffer1)
#define EXFEL_PROFILER_REPORT_SCALARFILTERBUFFER1(name) EXFEL_PROFILER_REPORT(scalarFilterBuffer1, name)
#else
#define EXFEL_PROFILER_SCALARFILTERBUFFER1
#define EXFEL_PROFILER_START_SCALARFILTERBUFFER1(name)
#define EXFEL_PROFILER_STOP_SCALARFILTERBUFFER1 
#define EXFEL_PROFILER_REPORT_SCALARFILTERBUFFER1(name) 
#endif




#endif	/* EXFEL_IO_IOPROFILER_HH */

