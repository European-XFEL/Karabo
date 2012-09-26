#include <karabo/util/Factory.hh>
#include <karabo/util/Hash.hh>
#include "StringStreamWriter.hh"

//namespace karabo {
 //namespace io {

    KARABO_REGISTER_FACTORY_CC(karabo::io::Writer<karabo::util::Hash>, karabo::io::StringStreamWriter<karabo::util::Hash>)
    KARABO_REGISTER_FACTORY_CC(karabo::io::Writer<karabo::util::Schema>, karabo::io::StringStreamWriter<karabo::util::Schema>)
  //} // namespace io
//} // namespace karabo