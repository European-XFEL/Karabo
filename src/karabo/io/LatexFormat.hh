/*
 * $Id: LatexFormat.hh 4644 2011-11-04 16:04:36Z heisenb@DESY.DE $
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on September 10, 2010, 10:31 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef EXFEL_IO_LATEXFORMAT_HH
#define	EXFEL_IO_LATEXFORMAT_HH

#include <karabo/util/Factory.hh>
#include <boost/foreach.hpp>

#include "Format.hh"

namespace exfel {

  namespace io {

    /**
     * The LatexFormat class.
     */
    class LatexFormat : public Format<exfel::util::Schema> {
    public:
      
      EXFEL_CLASSINFO(LatexFormat, "Latex", "1.0")

      LatexFormat() {
      };

      static void expectedParameters(exfel::util::Schema& expected);
       
      void configure(const exfel::util::Hash& input);

      void convert(std::stringstream& in, exfel::util::Schema& out);

      void convert(const exfel::util::Schema& in, std::stringstream& out);

      void formatExpectedParameters(const exfel::util::Schema& expected, std::stringstream& stream) const;

      virtual ~LatexFormat() {
      };

    private:
    };
  } // namespace io
} // namespace exfel

#endif	
