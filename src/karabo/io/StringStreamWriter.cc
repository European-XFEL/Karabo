#include <karabo/util/Factory.hh>
#include <karabo/util/Hash.hh>
#include "StringStreamWriter.hh"

//namespace exfel {
 //namespace io {

    EXFEL_REGISTER_FACTORY_CC(exfel::io::Writer<exfel::util::Hash>, exfel::io::StringStreamWriter<exfel::util::Hash>)
    EXFEL_REGISTER_FACTORY_CC(exfel::io::Writer<exfel::util::Schema>, exfel::io::StringStreamWriter<exfel::util::Schema>)
  //} // namespace io
//} // namespace exfel