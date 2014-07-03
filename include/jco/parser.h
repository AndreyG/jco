#pragma once

#include <vector>
#include <string>

#include <boost/utility/string_ref.hpp>

namespace jco
{
    struct utf8_text
    {
        const char *    data;
        std::size_t     size;
    };

    namespace details
    {
        struct ParserState
        {
            utf8_text   txt;
            std::size_t ptr;
        };

        enum class SSStatus
        {
            Normal, EOT
        };

        SSStatus skip_spaces(ParserState & st);

        template<class Res>
        void parse(ParserState & st, Res & res);

        template<class Res>
        Res parse(ParserState & st)
        {
            Res res;
            parse(st, res);
            return res;
        }
    }

    struct ParseError : std::exception {};

    struct Parser
    {
        explicit Parser(utf8_text const & txt);

        template<typename Res>
        Res parse()
        {
            return details::parse<Res>(st_);
        }

        bool eot();

    private:
        details::ParserState st_;
    };

    template<typename Res>
    Res parse(utf8_text const & txt)
    {
        Parser parser(txt);
        Res res = parser.parse<Res>();
        if (!parser.eot())
            throw ParseError();
        return res;
    }

    namespace details
    {
        enum class Token
        {
            ObjBegin, ObjEnd, ArrBegin, ArrEnd, Comma, Colon, Quote, Number, Constant, EOT
        };

        const char Quote = '\"';

        Token next_token(ParserState &);

        std::string read_string(ParserState &);

        void skip_number(ParserState &);

        void read_number(ParserState &, double & out);

        template<class T>
        struct expected_token_impl
        {
            static const Token value = Token::ObjBegin;
        };

        template<>
        struct expected_token_impl<double>
        {
            static const Token value = Token::Number;
        };

        template<>
        struct expected_token_impl<std::string>
        {
            static const Token value = Token::Quote;
        };

        template<class Element>
        struct expected_token_impl<std::vector<Element>>
        {
            static const Token value = Token::ArrBegin;
        };

        template<class T>
        constexpr Token expected_token() { return expected_token_impl<T>::value; }

        template<>
        inline void parse<double>(ParserState & st, double & out)
        {
            read_number(st, out);
        }

        template<>
        inline void parse<std::string>(ParserState & st, std::string & out)
        {
            out = read_string(st);
        }

        template<class Element>
        void parse(ParserState & st, std::vector<Element> & out)
        {
            if (next_token(st) != Token::ArrBegin)
                throw ParseError();

            for (;;)
            {
                auto token = next_token(st);
                switch (token)
                {
                case Token::ArrEnd:
                    return;
                case Token::EOT:
                case Token::ObjEnd:
                    throw ParseError();
                default:
                    if (token != expected_token<Element>())
                        throw ParseError();

                    --st.ptr;
                    out.push_back(parse<Element>(st));

                    switch (next_token(st))
                    {
                    case Token::ArrEnd:
                        return;
                    case Token::Comma:
                        break;
                    default:
                        throw ParseError();
                    }
                }
            }
        }

        struct value_reader
        {
            template<typename Field>
            void operator() (Field & f, const char * key)
            {
                if (key_was_found_)
                    return;
                else if (key_was_found_ = (key_ == key))
                    parse(st_, f);
            }

            value_reader(boost::string_ref key, ParserState & st, bool & key_was_found)
                : key_(key)
                , st_(st)
                , key_was_found_(key_was_found)
            {
                key_was_found = false;
            }

        private:
            boost::string_ref key_;
            ParserState & st_;
            bool & key_was_found_;
        };

        void skip_value(ParserState &);

        template<class Res>
        void read_key_value_pair(ParserState & st, Res & res)
        {
            std::string key = read_string(st);
            if (next_token(st) != Token::Colon)
                throw ParseError();
            skip_spaces(st);
            bool key_was_found;
            for_each(res, value_reader(key, st, key_was_found));
            if (!key_was_found)
                skip_value(st);
        }

        template<class Res>
        void parse(ParserState & st, Res & res)
        {
            if (next_token(st) != Token::ObjBegin)
                throw ParseError();

            for (;;)
            {
                switch (next_token(st))
                {
                case Token::ObjEnd:
                    return;
                case Token::Quote:
                    --st.ptr;
                    read_key_value_pair(st, res);
                    switch (next_token(st))
                    {
                    case Token::ObjEnd:
                        return;
                    case Token::Comma:
                        break;
                    default:
                        throw ParseError();
                    }
                    break;
                default:
                    throw ParseError();
                }
            }
        }
    }
}
