#pragma once

#include <ostream>
#include <sstream>
#include <memory>

#include <boost/utility/string_ref.hpp>
#include <boost/preprocessor/cat.hpp>

namespace jco
{
    struct SerializationError : std::exception {};

    namespace serialization
    {
        struct out_stream;

        struct array_scope
        {
            explicit array_scope(out_stream &);
            ~array_scope();

        private:
            out_stream & out_;
        };

        struct object_scope
        {
            explicit object_scope(out_stream &);
            ~object_scope();

        private:
            out_stream & out_;
        };

        struct key_tag
        {
            explicit key_tag(boost::string_ref name)
                : name_(name)
            {}

            boost::string_ref name() const { return name_; }

        private:
            boost::string_ref name_;
        };

        key_tag key(boost::string_ref name);

        template<class Value>
        struct value_tag
        {
            Value value;
        };

        typedef value_tag<boost::string_ref>    string_value_tag;
        typedef value_tag<double>               number_value_tag;
        typedef value_tag<bool>                 bool_value_tag;

        template<class Value>
        value_tag<Value> value(Value v) { return { v }; }

        string_value_tag value(const char * str);
        string_value_tag value(std::string const & str);

        struct null_value_tag {};
        null_value_tag value(std::nullptr_t);

#define DEFINE_TAG(entity)                          \
        struct BOOST_PP_CAT(entity, _tag) {};       \
        const  BOOST_PP_CAT(entity, _tag) entity;   \
        struct BOOST_PP_CAT(entity, _value_tag) {}; \
        inline BOOST_PP_CAT(entity, _value_tag)     \
            value(BOOST_PP_CAT(entity, _tag)) { return {}; }

        DEFINE_TAG(array)
        DEFINE_TAG(object)

#undef DEFINE_TAG

        enum class Style
        {
            SingleLine, Pretty
        };

        struct out_stream
        {
            void operator << (boost::string_ref str);
            void operator << (double x);
            void operator << (bool f);
            void operator << (std::nullptr_t);

            void operator << (array_value_tag);
            void operator << (object_value_tag);

            out_stream& operator << (key_tag);

            out_stream& operator << (string_value_tag);
            out_stream& operator << (number_value_tag);
            out_stream& operator << (bool_value_tag);
            out_stream& operator << (null_value_tag);

            out_stream(std::ostream & backend, Style style);
            ~out_stream();

        private:

            friend struct array_scope;
            void open_array();
            void close_array();

            friend struct object_scope;
            void open_object();
            void close_object();

        private:
            struct implementation;
            std::unique_ptr<implementation> pimpl;
        };

        template<typename T>
        std::string to_string(T const & t)
        {
            std::ostringstream ss;
            {
                out_stream out(ss, Style::SingleLine);
                out << t;
            }
            return ss.str();
        }
    }
}
