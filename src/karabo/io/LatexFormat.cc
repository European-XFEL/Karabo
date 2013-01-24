/*
 * $Id: LatexFormat.cc 6764 2012-07-18 09:29:46Z heisenb $
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on September 10, 2010, 10:31 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */
#include "LatexFormat.hh"

KARABO_REGISTER_FACTORY_CC(karabo::io::Format<karabo::util::Schema>, karabo::io::LatexFormat)

namespace karabo {
    namespace io {

        using namespace std;
        using namespace karabo::util;

        typedef Format<karabo::util::Schema> FormatConfigStringStream;

        void LatexFormat::expectedParameters(karabo::util::Schema& expected) {
        }

        void LatexFormat::configure(const karabo::util::Hash& input) {
        }

        void LatexFormat::convert(stringstream& in, Schema& out) {
        }

        void LatexFormat::convert(const Schema& in, stringstream& out) {

            formatExpectedParameters(in, out);

        }

        void LatexFormat::formatExpectedParameters(const Schema& expected, stringstream& stream) const {
            try {
                if (!expected.has("root")) return;

                stream << "\\begin{table}[ht]\\footnotesize" << endl;
                stream << "\\centering" << endl;
                stream << "\\begin{tabular}{ l | p{4cm} l l p{2cm}}" << endl;
                stream << "\\textbf{Key} & \\textbf{Description} & \\textbf{Type} & \\textbf{Default} & \\textbf{Range}\\\\ \\hline" << endl;

                //Getting elements
                const Schema& elements = expected.get<Schema > ("elements");

                for (Schema::const_iterator it = elements.begin(); it != elements.end(); it++) {
                    const Schema& desc = elements.get<Schema > (it);

                    string key("");
                    string description("");
                    string type("");
                    string defaultValue("");
                    string assignment("");
                    string expertLevel("");
                    string range("");

                    if (!desc.has("root")) {
                        key = desc.getAsString("key");
                    }
                    if (desc.has("description")) {
                        desc.get("description", description);
                    }
                    if (desc.has("simpleType")) {
                        type = Types::convert(desc.get<Types::ReferenceType > ("simpleType"));
                    } else if (desc.has("complexType")) {
                        Schema::OccuranceType occ = desc.get<Schema::OccuranceType > ("occurrence");
                        if (occ == Schema::EITHER_OR) {
                            type = "\\textit{COMPLEX}(\\textit{CHOICE})";
                        } else if (occ == Schema::ONE_OR_MORE) {
                            type = "\\textit{COMPLEX}(\\textit{NON_EMPTY_LIST})";
                        } else if (occ == Schema::ZERO_OR_MORE) {
                            type = "\\textit{COMPLEX}(\\textit{LIST})";
                        }
                    }
                    if (desc.has("default")) {
                        defaultValue = desc.getAsString("default");
                    }
                    if (desc.has("assignment")) {
                        Schema::AssignmentType at = desc.get<Schema::AssignmentType > ("assignment");
                        if (at == Schema::OPTIONAL_PARAM) {
                            assignment = "o";
                        } else if (at == Schema::MANDATORY_PARAM) {
                            assignment = "m";
                            key += "$^m$";
                        } else if (at == Schema::INTERNAL_PARAM) {
                            assignment = "i";
                            key += "$^i$";
                        }
                    }
                    if (desc.has("options")) {
                        string options = desc.getAsString("options");
                        vector<string> tmp;
                        boost::split(tmp, options, boost::is_any_of(","));
                        if (tmp.size() > 1) {
                            //range = "\\multilineL{";
                            for (size_t i = 0; i < tmp.size() - 1; ++i) {
                                range += tmp[i] + ", ";
                            }
                            range += tmp.back();
                        } else {
                            range += desc.getAsString("options");
                        }
                    } else {
                        if (desc.has("minInc")) {
                            range += "[" + desc.getAsString("minInc") + ", ";
                        } else if (desc.has("minExc")) {
                            range += "(" + desc.getAsString("minExc") + ", ";
                        } else {
                            if (type.substr(0, 3) == "UNS") {
                                range += "[0, ";
                            } else {
                                range += "($-\\infty, $";
                            }
                        }
                        if (desc.has("maxInc")) {
                            range += desc.getAsString("maxInc") + "]";
                        } else if (desc.has("maxExc")) {
                            range += desc.getAsString("maxExc") + ")";
                        } else {
                            if (range.substr(0, 3) == "($-") {
                                range = "$\\emptyset$";
                            } else {
                                range += "$+\\infty$)";
                            }
                        }
                    }
                    if (desc.has("expertLevel")) {
                        int level = desc.get<int>("expertLevel");
                        if (level > 0) {
                            key += "$^+$";
                        }
                    }
                    stream << "\\textbf{" << key << "}" << "&" << description << "&" << type << "&" << defaultValue << "&" << range << "\\\\" << endl;
                }
                stream << "\\end{tabular}" << endl;
                stream << "\\label{tab:[...]Factory: " << expected.get<string > ("root") << "}" << endl;
                stream << "\\end{table}" << endl;
            } catch (...) {
                KARABO_RETHROW;
            }
        }
    } // namespace io
} // namespace karabo
