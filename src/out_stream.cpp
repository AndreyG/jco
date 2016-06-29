#include "jco/serialization.h"

#include <stack>
#include <boost/range/algorithm/find.hpp>

#include "printer.h"

namespace jco
{
    namespace serialization
    {
        enum class State
        {
            Initial, Terminal, ArrayBegin, Array, ObjectBegin, Object, Key, ValueArr, ValueObj,
        };

        template<size_t N>
        using StateList = std::array<State, N>;

        StateList<2> operator | (State s1, State s2) { return { s1, s2 }; }

        struct out_stream::implementation
        {
            PrinterPtr printer;

            template<class Value>
            out_stream& write(value_tag<Value> v);

            out_stream& write(null_value_tag);

            template<class Value>
            out_stream& write_primitive(Value);

            void finish_arr_element();

            State current_state() const;
            void set_current_state(State s);

            void    push_state(State);
            State   pop_state();

            void expect(State) const;

            template<size_t N>
            void expect(StateList<N> states) const;

            explicit implementation(out_stream & ostream)
                : ostream_(ostream)
            {
                push_state(State::Initial);
            }

            ~implementation() noexcept(false)
            {
                if ((state_.size() != 1) || (state_.top() != State::Terminal))
                    throw SerializationError();
            }

        private:
            out_stream & ostream_;
            std::stack<State> state_;
        };

        void out_stream::implementation::push_state(State s)
        {
            state_.push(s);
        }

        State out_stream::implementation::pop_state()
        {
            if (state_.empty())
                throw SerializationError();

            State res = state_.top();
            state_.pop();
            return res;
        }

        namespace details
        {
            void expect(State real, State expected)
            {
                if (real != expected)
                    throw SerializationError();
            }

            template<size_t N>
            void expect(State real, StateList<N> expected)
            {
                if (boost::find(expected, real) == end(expected))
                    throw SerializationError();
            }
        }

        void out_stream::implementation::expect(State expected) const
        {
            details::expect(current_state(), expected);
        }

        template<size_t N>
        void out_stream::implementation::expect(StateList<N> expected) const
        {
            details::expect(current_state(), expected);
        }

        template<class Value>
        out_stream& out_stream::implementation::write(value_tag<Value> v)
        {
            expect(State::Key);
            printer->print(v.value);
            state_.pop();
            expect(State::ObjectBegin | State::Object);
            state_.top() = State::Object;
            return ostream_;
        }

        out_stream& out_stream::implementation::write(null_value_tag)
        {
            expect(State::Key);
            printer->print(nullptr);
            state_.pop();
            expect(State::ObjectBegin | State::Object);
            state_.top() = State::Object;
            return ostream_;
        }

        State out_stream::implementation::current_state() const
        {
            if (state_.empty())
                throw SerializationError();
            return state_.top();
        }

        void out_stream::implementation::set_current_state(State s)
        {
            state_.top() = s;
        }

        void out_stream::implementation::finish_arr_element()
        {
            if (current_state() == State::ArrayBegin)
                set_current_state(State::Array);
        }

        template<class Value>
        out_stream& out_stream::implementation::write_primitive(Value v)
        {
            switch (current_state())
            {
            case State::Initial:
                printer->print(v);
                set_current_state(State::Terminal);
                break;
            case State::ArrayBegin:
                printer->print(v);
                set_current_state(State::Array);
                break;
            case State::Array:
                printer->separate_array_elements();
                printer->print(v);
                break;
            default:
                throw SerializationError();
            }
            return ostream_;
        }

        void out_stream::open_array()
        {
            switch (pimpl->current_state())
            {
            case State::Initial:
                pimpl->set_current_state(State::Terminal);
                break;
            case State::ValueArr:
            case State::ArrayBegin:
                break;
            case State::Array:
                pimpl->printer->separate_array_elements();
                break;
            default:
                throw SerializationError();
            }

            pimpl->printer->open_array();
            pimpl->push_state(State::ArrayBegin);
        }

        void out_stream::close_array()
        {
            details::expect(pimpl->pop_state(), State::ArrayBegin | State::Array);

            pimpl->printer->close_array();

            if (pimpl->current_state() == State::ValueArr)
                pimpl->pop_state();
            else
                pimpl->finish_arr_element();
        }

        void out_stream::open_object()
        {
            switch (pimpl->current_state())
            {
            case State::Initial:
                pimpl->set_current_state(State::Terminal);
                break;
            case State::ValueObj:
            case State::ArrayBegin:
                break;
            case State::Array:
                pimpl->printer->separate_array_elements();
                break;
            default:
                throw SerializationError();
            }

            pimpl->printer->open_object();
            pimpl->push_state(State::ObjectBegin);
        }

        void out_stream::close_object()
        {
            details::expect(pimpl->pop_state(), State::ObjectBegin | State::Object);

            pimpl->printer->close_object();

            if (pimpl->current_state() == State::ValueObj)
                pimpl->pop_state();
            else
                pimpl->finish_arr_element();
        }

        out_stream& out_stream::operator <<(number_value_tag x)
        {
            return pimpl->write(x);
        }

        out_stream& out_stream::operator <<(string_value_tag s)
        {
            return pimpl->write(s);
        }

        void out_stream::operator <<(object_value_tag)
        {
            pimpl->expect(State::Key);
            pimpl->set_current_state(State::ValueObj);
        }

        void out_stream::operator <<(array_value_tag)
        {
            pimpl->expect(State::Key);
            pimpl->set_current_state(State::ValueArr);
        }

        out_stream& out_stream::operator << (key_tag key)
        {
            switch (pimpl->current_state())
            {
            case State::ObjectBegin:
                pimpl->set_current_state(State::Object);
                break;
            case State::Object:
                pimpl->printer->separate_object_fields();
                break;
            default:
                throw SerializationError();
            }
            pimpl->printer->key(key.name());
            pimpl->push_state(State::Key);
            return *this;
        }

        out_stream& out_stream::operator << (null_value_tag tag)
        {
            return pimpl->write(tag);
        }

        out_stream& out_stream::operator << (bool f)
        {
            return pimpl->write_primitive(f);
        }

        out_stream& out_stream::operator << (double x)
        {
            return pimpl->write_primitive(x);
        }

        out_stream& out_stream::operator << (const char * str)
        {
            return pimpl->write_primitive(boost::string_ref(str));
        }

        out_stream& out_stream::operator << (std::nullptr_t)
        {
            return pimpl->write_primitive(nullptr);
        }

        PrinterPtr make_printer(std::ostream & backend, Style style);

        out_stream::out_stream(std::ostream & backend, Style style)
            : pimpl(new implementation(*this))
        {
            pimpl->printer = make_printer(backend, style);
        }

        out_stream::~out_stream() {}
    }
}
