#include "jco/serialization.h"

#include <cassert>

namespace jco
{
    namespace serialization
    {
        void out_stream::write(boost::string_ref str)
        {
            backend_ << "\"" << str << "\"";
        }

        void out_stream::write(double x)
        {
            backend_ << x;
        }

        void out_stream::write(bool f)
        {
            backend_ << (f ? "true" : "false");
        }

        void out_stream::write(std::nullptr_t)
        {
            backend_ << "null";
        }

        void out_stream::write_comma()
        {
            backend_ << ", ";
        }

        out_stream::State out_stream::current_state() const
        {
            if (state_.empty())
                throw SerializationError();
            return state_.top();
        }

        void out_stream::set_current_state(State s)
        {
            state_.top() = s;
        }

        void out_stream::finish_arr_element()
        {
            if (current_state() == State::ArrayBegin)
                set_current_state(State::Array);
        }

        void out_stream::open_array()
        {
            switch (current_state())
            {
            case State::Initial:
                set_current_state(State::Terminal);
                break;
            case State::ValueArr:
            case State::ArrayBegin:
                break;
            case State::Array:
                write_comma();
                break;
            default:
                throw SerializationError();
            }

            backend_ << "[ ";
            state_.push(State::ArrayBegin);
        }

        void out_stream::close_array()
        {
            expect(State::ArrayBegin | State::Array);
            state_.pop();

            backend_ << " ]";

            if (current_state() == State::ValueArr)
                state_.pop();
            else
                finish_arr_element();
        }

        void out_stream::open_object()
        {
            switch (current_state())
            {
            case State::Initial:
                state_.top() = State::Terminal;
                break;
            case State::ValueObj:
            case State::ArrayBegin:
                break;
            case State::Array:
                write_comma();
                break;
            default:
                throw SerializationError();
            }

            backend_ << "{ ";
            state_.push(State::ObjectBegin);
        }

        void out_stream::close_object()
        {
            expect(State::ObjectBegin | State::Object);
            state_.pop();

            backend_ << " }";

            if (current_state() == State::ValueObj)
                state_.pop();
            else
                finish_arr_element();
        }

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

        void out_stream::operator <<(object_value_tag)
        {
            expect(State::Key);
            state_.top() = State::ValueObj;
        }

        out_stream& out_stream::operator << (key_tag key)
        {
            switch (current_state())
            {
            case State::ObjectBegin:
                state_.top() = State::Object;
                break;
            case State::Object:
                backend_ << ", ";
                break;
            default:
                throw SerializationError();
            }
            backend_ << "\"" << key.name()  << "\" : ";
            state_.push(State::Key);
            return *this;
        }

        out_stream::out_stream(std::ostream & backend)
            : backend_(backend)
        {
            state_.push(State::Initial);
        }

        out_stream::~out_stream()
        {
            if ((state_.size() != 1) || (state_.top() != State::Terminal))
                throw SerializationError();
        }

        void out_stream::expect(State s) const
        {
            if (state_.empty() || state_.top() != s)
                throw SerializationError();
        }

        key_tag key(boost::string_ref name)
        {
            return key_tag(name);
        }

        string_value_tag value(const char * str)
        {
            return value(boost::string_ref(str));
        }
    }
}
