#include <boost/version.hpp>
#if BOOST_VERSION < 107000
#include "BeastClientBase_1_68.hh"
#else
#include "BeastClientBase_1_81.hh"
#endif

namespace karabo {
    namespace net {
        int httpsPost(const std::string& host, const std::string& port, const std::string& target,
                      const std::string& body, int version, const std::function<void(bool, std::string)>& onComplete);
    }
} // namespace karabo
