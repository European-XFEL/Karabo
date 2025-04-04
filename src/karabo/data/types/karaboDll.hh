/*
 * $Id$
 *
 *   * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 24, 2011, 11:13 AM
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

#ifndef KARABO_DATA_TYPES_KARABO_DLL_HH
#define KARABO_DATA_TYPES_KARABO_DLL_HH

// clang-format off

/********************************************
 *         Disable Specific Warnings         *
 ********************************************/



/********************************************
 *       Declare a parameter as unused      *
 ********************************************/

#if defined(_WIN32)
#define ALWAYS_UNUSED(x) x;
#else
#define ALWAYS_UNUSED(x)
#endif


/********************************************
 *            Windows DLL stuff             *
 ********************************************/

#if defined(_WIN32)
#if defined(__DLL__)
#define KARABO_DECLSPEC __declspec(dllexport)
#define KARABO_TEMPLATE_DLL
#else
#define KARABO_DECLSPEC __declspec(dllimport)
#define KARABO_TEMPLATE_DLL extern
#endif
#elif defined(unix)      || defined(__unix)      || defined(__unix__) \
 || defined(linux)       || defined(__linux)     || defined(__linux__) \
 || defined(sun)         || defined(__sun) \
 || defined(BSD)         || defined(__OpenBSD__) || defined(__NetBSD__) \
 || defined(__FreeBSD__) || defined __DragonFly__ \
 || defined(sgi)         || defined(__sgi) \
 || defined(__MACOSX__)  || defined(__APPLE__) \
 || defined(__CYGWIN__)
#define KARABO_DECLSPEC
#define KARABO_TEMPLATE_DLL
#endif

#ifdef __GNUC__
#define KARABO_DEPRECATED __attribute__((deprecated))
#elif defined(_MSC_VER)
#define KARABO_DEPRECATED __declspec(deprecated)
#else
#pragma message("WARNING: You need to implement KARABO_DEPRECATED for this compiler")
#define KARABO_DEPRECATED
#endif

// clang-format on

#endif
