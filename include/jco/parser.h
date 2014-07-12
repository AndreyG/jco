#pragma once

#include <vector>
#include <string>
#include <memory>
#include <map>
#include <functional>
#include <cassert>
#include <cstring>

#include <boost/utility/string_ref.hpp>

namespace jco
{
    struct utf8_text
    {
        const char *    data;
        std::size_t     size;
    };

    inline utf8_text from_string(boost::string_ref str)
    {
        return { str.cbegin(), str.size() };
    }

    namespace details
    {
        struct ParserState
        {
            utf8_text   txt;
            std::size_t ptr;
        };

        enum class Token
        {
            ObjBegin, ObjEnd, ArrBegin, ArrEnd, Comma, Colon, Quote, Number, Constant, EOT
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

        std::string parse_string();

        bool eot();

        void expect(details::Token);
        void expect(boost::string_ref str);

        details::Token next_token();

    private:
        details::ParserState st_;
    };

    template<class T>
    struct TypedParser
    {
        typedef std::unique_ptr<T>              TPtr;
        typedef std::function<TPtr (Parser &)>  Factory;

        TPtr parse_single(utf8_text const & txt)
        {
            Parser parser(txt);

            auto res = parse_single_impl(parser);
            if (!parser.eot())
                throw ParseError();

            return res;
        }

        void parse_array(utf8_text const & txt, std::function<void (TPtr)> proc)
        {
            using details::Token;

            Parser parser(txt);

            parser.expect(Token::ArrBegin);
            if (parser.next_token() != Token::ArrEnd)
            {
                proc(parse_single_impl(parser, true));
                for (;;)
                {
                    switch (parser.next_token())
                    {
                    case Token::ArrEnd:
                        return;
                    case Token::Comma:
                        proc(parse_single_impl(parser));
                        break;
                    default:
                        throw ParseError();
                    }
                }
            }
        }

        void register_factory(std::string const & type, Factory factory)
        {
            assert(!factories_.count(type));
            factories_[type] = std::move(factory);
        }

    private:
        TPtr parse_single_impl(Parser & parser, bool obj_started = false)
        {
            using details::Token;

            if (!obj_started)
                parser.expect(Token::ObjBegin);

            parser.expect("type");
            parser.expect(Token::Colon);

            auto type = parser.parse_string();

            if (auto f = find_factory(type))
            {
                parser.expect(Token::Comma);
                parser.expect("description");
                parser.expect(Token::Colon);
                auto res = (*f)(parser);
                parser.expect(Token::ObjEnd);
                return res;
            }
            else
                throw std::logic_error("unknown type \"" + type + "\"");
        }

    private:
        Factory const * find_factory(std::string const & type) const
        {
            auto it = factories_.find(type);
            if (it == factories_.end())
                return nullptr;
            else
                return &it->second;
        }

    private:
        std::map<std::string, Factory> factories_;
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
