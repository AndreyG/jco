#include "jco/serialization.h"

#include <cassert>

namespace jco
{
    namespace serialization
    {
        array_scope::array_scope(out_stream & out)
            : out_(out)
        {
            out_.open_array();
        }

        array_scope::~array_scope()
        {
            out_.close_array();
        }

        object_scope::object_scope(out_stream & out)
            : out_(out)
        {
            out_.open_object();
        }

        object_scope::~object_scope()
        {
            out_.close_object();
        }

        key_tag key(boost::string_ref name)
        {
            return key_tag(name);
        }

        string_value_tag value(const char * str)
        {
            return value(boost::string_ref(str));
        }

        string_value_tag value(std::string const & str)
        {
            return value(boost::string_ref(str));
        }
    }
}
