/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   FromNumpy.cc
 * Author: <burkhard.heisen@xsmail.com>
 *
 * Created on May 23, 2014 6:34 PM
 *
 */

#include "FromNumpy.hh"

#include <boost/python.hpp>

#include "Wrapper.hh"

namespace karathon {


    FromNumpy::FromNumpy() {
        // #define _KARABO_HELPER_MACRO(fromType, refType) (fromType, karabo::util::Types::refType)
#define _KARABO_HELPER_MACRO(fromType, refType) \
    { fromType, karabo::util::Types::refType }

        if (sizeof(unsigned long) == sizeof(unsigned int)) { // 32 bit CPU
            _typeInfoMap = {_KARABO_HELPER_MACRO(NPY_BOOL, BOOL),        _KARABO_HELPER_MACRO(NPY_BYTE, INT8),
                            _KARABO_HELPER_MACRO(NPY_UBYTE, UINT8),      _KARABO_HELPER_MACRO(NPY_SHORT, INT16),
                            _KARABO_HELPER_MACRO(NPY_USHORT, UINT16),    _KARABO_HELPER_MACRO(NPY_INT, INT32),
                            _KARABO_HELPER_MACRO(NPY_UINT, UINT32),      _KARABO_HELPER_MACRO(NPY_LONG, INT32),
                            _KARABO_HELPER_MACRO(NPY_ULONG, UINT32),     _KARABO_HELPER_MACRO(NPY_LONGLONG, INT64),
                            _KARABO_HELPER_MACRO(NPY_ULONGLONG, UINT64), _KARABO_HELPER_MACRO(NPY_FLOAT, FLOAT),
                            _KARABO_HELPER_MACRO(NPY_DOUBLE, DOUBLE)};
        } else { // 64 bit CPU
            _typeInfoMap = {_KARABO_HELPER_MACRO(NPY_BOOL, BOOL),        _KARABO_HELPER_MACRO(NPY_BYTE, INT8),
                            _KARABO_HELPER_MACRO(NPY_UBYTE, UINT8),      _KARABO_HELPER_MACRO(NPY_SHORT, INT16),
                            _KARABO_HELPER_MACRO(NPY_USHORT, UINT16),    _KARABO_HELPER_MACRO(NPY_INT, INT32),
                            _KARABO_HELPER_MACRO(NPY_UINT, UINT32),      _KARABO_HELPER_MACRO(NPY_LONG, INT64),
                            _KARABO_HELPER_MACRO(NPY_ULONG, UINT64),     _KARABO_HELPER_MACRO(NPY_LONGLONG, INT64),
                            _KARABO_HELPER_MACRO(NPY_ULONGLONG, UINT64), _KARABO_HELPER_MACRO(NPY_FLOAT, FLOAT),
                            _KARABO_HELPER_MACRO(NPY_DOUBLE, DOUBLE)};
        }
#undef _KARABO_HELPER_MACRO
    }
} // namespace karathon
