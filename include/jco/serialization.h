#pragma once

#include <ostream>
#include <stack>
#include <sstream>
#include <array>

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

        struct out_stream
        {
            void operator << (boost::string_ref str);
            void operator << (double x);
            void operator << (bool f);
            void operator << (std::nullptr_t);

            void operator << (array_value_tag);
            void operator << (object_value_tag);

            out_stream& operator << (key_tag);

            template<class Value>
            out_stream& operator << (value_tag<Value> v)
            {
                expect(State::Key);
                write(v.value);
                state_.pop();
                expect(State::ObjectBegin | State::Object);
                state_.top() = State::Object;
                return *this;
            }

            explicit out_stream(std::ostream & backend);
            ~out_stream();

        private:
            enum class State
            {
                Initial, Terminal, ArrayBegin, Array, ObjectBegin, Object, Key, ValueArr, ValueObj,
            };

            template<size_t N>
            using StateList = std::array<State, N>;

        private:
            State current_state() const;
            void set_current_state(State s);

            friend struct array_scope;
            void open_array();
            void close_array();

            friend struct object_scope;
            void open_object();
            void close_object();

            void write_comma();

            void write(double);
            void write(boost::string_ref);
            void write(bool);
            void write(std::nullptr_t);

            void finish_arr_element();

            void expect(State) const;

            template<size_t N>
            void expect(StateList<N> states) const
            {
                if (state_.empty() || std::find(begin(states), end(states), state_.top()) == end(states))
                    throw SerializationError();
            }

            friend StateList<2> operator | (State s1, State s2) { return { s1, s2 }; }

        private:
            std::ostream & backend_;
            std::stack<State> state_;
        };

        template<typename T>
        std::string to_string(T const & t)
        {
            std::ostringstream ss;
            {
                out_stream out(ss);
                out << t;
            }
            return ss.str();
        }
    }
}
