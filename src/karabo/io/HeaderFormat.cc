#include <istream>
#include <ostream>
#include <vector>
#include <string>
#include <karabo/util/Factory.hh>
#include <karabo/util/Hash.hh>
#include "HeaderFormat.hh"


KARABO_REGISTER_FACTORY_CC(karabo::io::Format<karabo::util::Hash>, karabo::io::HeaderFormat)

using namespace std;
using namespace karabo::util;

namespace karabo {
    namespace io {

        void HeaderFormat::expectedParameters(Schema& expected) {

            STRING_ELEMENT(expected)
                    .key("separator")
                    .displayedName("Separator")
                    .description("Separator symbol used to represent Hash hierarchy")
                    .assignmentOptional().defaultValue("\n")
                    //.reconfigurable() // Maybe back later, ask BH
                    .commit();
        }

        void HeaderFormat::configure(const Hash& input) {
            if (input.get<string>("separator") == "") m_sep = "\n";
            else input.get("separator", m_sep);
        }

        int HeaderFormat::readKey(istream& is, string& path) {
            unsigned char size = 0;
            int length = this->readFrom(is, &size, sizeof (size));
            {
                boost::scoped_ptr<char> buffer(new char [size + 1]);
                is.read(buffer.get(), size);
                path.assign(buffer.get(), size);
            }
            return length + path.size();
        }

        void HeaderFormat::writeKey(ostream& os, const string& path) {
            unsigned char size = path.length();
            this->writeTo(os, &size, sizeof (size));
            os.write(path.c_str(), path.length());
        }
    }
}

