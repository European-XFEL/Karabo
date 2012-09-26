/*
 * $Id: LatexFormat.hh 4644 2011-11-04 16:04:36Z heisenb@DESY.DE $
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on September 10, 2010, 10:31 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_IO_LATEXFORMAT_HH
#define	KARABO_IO_LATEXFORMAT_HH

#include <karabo/util/Factory.hh>
#include <boost/foreach.hpp>

#include "Format.hh"

namespace karabo {

  namespace io {

    /**
     * The LatexFormat class.
     */
    class LatexFormat : public Format<karabo::util::Schema> {
    public:
      
      KARABO_CLASSINFO(LatexFormat, "Latex", "1.0")

      LatexFormat() {
      };

      static void expectedParameters(karabo::util::Schema& expected);
       
      void configure(const karabo::util::Hash& input);

      void convert(std::stringstream& in, karabo::util::Schema& out);

      void convert(const karabo::util::Schema& in, std::stringstream& out);

      void formatExpectedParameters(const karabo::util::Schema& expected, std::stringstream& stream) const;

      virtual ~LatexFormat() {
      };

    private:
    };
  } // namespace io
} // namespace karabo

#endif	
