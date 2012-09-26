/*
 * $Id: String.cc 5321 2012-03-01 13:49:34Z heisenb $
 *
 * File:   String.cc
 * Author: <your.email@xfel.eu>
 *
 * Created on August 19, 2010, 8:14 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

/**
 * ###   PLEASE NOTE   ###
 * (1) Fill in the Author field above with your email address
 * (2) Rename "packageName" to your real package's name (in camel case)
 * (3) Remove this whole comment block
 */

#include "String.hh"

using namespace std;

namespace karabo {
  namespace util {

    template<>
    std::string String::toString(const char* in, int width, int precision, char fillChar) {
      if (width > -1) {
        return std::string(in, width);
      } else {
        return std::string(in);
      }
    }
  }
}
