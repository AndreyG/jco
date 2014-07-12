#include "jco/parser.h"

#include <cassert>
#include <iostream>

#include <boost/locale/encoding_utf.hpp>
#include <boost/range/algorithm/copy.hpp>

namespace jco
{
    namespace details
    {
        void skip_BOM(ParserState & st)
        {
            static const char magick_seq[] = {
                static_cast<char>(0xEF),
                static_cast<char>(0xBB),
                static_cast<char>(0xBF)
            };
            if ((st.txt.size >= 3) && std::equal(magick_seq, magick_seq + 3, st.txt.data))
                st.ptr = 3;
        }

        enum SkipSymbol : char
        {
            Space = 0x20,
            Tab = 0x09,
            LF = 0x0a,
            CR = 0x0d
        };

        bool is_skip_symbol(char c)
        {
            switch (c)
            {
            case SkipSymbol::Space:
            case SkipSymbol::Tab:
            case SkipSymbol::LF:
            case SkipSymbol::CR:
                return true;
            default:
                return false;
            }
        }

        SSStatus skip_spaces(ParserState & st)
        {
            assert(st.ptr < st.txt.size);

            for (;; ++st.ptr)
            {
                if (st.ptr == st.txt.size)
                    return SSStatus::EOT;

                if (!is_skip_symbol(st.txt.data[st.ptr]))
                    return SSStatus::Normal;
            }
        }

        char get_symbol(ParserState const & st)
        {
            return st.txt.data[st.ptr];
        }

        bool end_of_text(ParserState const & st)
        {
            return st.ptr == st.txt.size;
        }

        Token next_token(ParserState & st)
        {
            if (st.ptr >= st.txt.size)
                throw ParseError();

            if (skip_spaces(st) == SSStatus::EOT)
                return Token::EOT;

            auto c = get_symbol(st);
            ++st.ptr;

            if ((c >= '0') && (c <= '9'))
                return Token::Number;

            switch (c)
            {
            case '{':   return Token::ObjBegin;
            case '}':   return Token::ObjEnd;
            case '[':   return Token::ArrBegin;
            case ']':   return Token::ArrEnd;
            case ',':   return Token::Comma;
            case ':':   return Token::Colon;
            case Quote: return Token::Quote;
            case '-':   return Token::Number;
            case 't':
            case 'f':
            case 'n':
                return Token::Constant;
            default:
                throw ParseError();
            }
        }

        std::uint16_t read_utf16_symbol(ParserState & st)
        {
            std::uint16_t res = 0;

            for (size_t i = 0; i != 4; ++i, ++st.ptr)
            {
                res <<= 4;

                char c = get_symbol(st);
                if (c >= '0' && c <= '9')
                    res += c - '0';
                if (c >= 'a' && c <= 'f')
                    res += 10 + c - 'a';
                if (c >= 'A' && c <= 'F')
                    res += 10 + c - 'A';
            }

            return res;
        }

        template<class OutIter>
        void convert_to_utf8(char16_t c, OutIter out)
        {
            auto str = boost::locale::conv::utf_to_utf<char>(&c, &c + 1);
            boost::copy(str, out);
        }

        template<class OutIter>
        void convert_to_utf8(char16_t c1, char16_t c2, OutIter out)
        {
            auto c = { c1, c2 };
            auto str = boost::locale::conv::utf_to_utf<char>(std::begin(c), std::end(c));
            boost::copy(str, out);
        }

        std::string read_string(ParserState & st)
        {
            assert(get_symbol(st) == Quote);
            ++st.ptr;

            std::vector<char> res;

            for (;;)
            {
                if (end_of_text(st))
                    throw ParseError();

                auto c = get_symbol(st);

                switch (c)
                {
                case Quote:
                    ++st.ptr;
                    return std::string(res.begin(), res.end());
                case '\\':
                    ++st.ptr;
                    if (end_of_text(st))
                        throw ParseError();
                    c = get_symbol(st);
                    switch (c)
                    {
                    case Quote:
                    case '\\':
                    case '/':
                        res.push_back(c);
                        ++st.ptr;
                        break;
                    case 'b':
                        res.push_back(0x08);
                        ++st.ptr;
                        break;
                    case 'f':
                        res.push_back(0x0C);
                        ++st.ptr;
                        break;
                    case 'n':
                        res.push_back(SkipSymbol::LF);
                        ++st.ptr;
                        break;
                    case 'r':
                        res.push_back(SkipSymbol::CR);
                        ++st.ptr;
                        break;
                    case 't':
                        res.push_back(SkipSymbol::Tab);
                        ++st.ptr;
                        break;
                    case 'u':
                        ++st.ptr;
                        if (st.ptr + 4 > st.txt.size)
                            throw ParseError();
                        auto utf16 = read_utf16_symbol(st);
                        if ((utf16 < 0xD800) || (utf16 > 0xDFFF))
                        {
                            convert_to_utf8(utf16, std::back_inserter(res));
                        }
                        else
                        {
                            if ((st.ptr + 5 >= st.txt.size) || (get_symbol(st) != '\\') || (st.txt.data[st.ptr + 1] != 'u'))
                                throw ParseError();
                            st.ptr += 2;
                            convert_to_utf8(utf16, read_utf16_symbol(st), std::back_inserter(res));
                        }
                        break;
                    }
                    break;
                default:
                    res.push_back(c);
                    ++st.ptr;
                }
            }
        }

        void skip_number(ParserState & st)
        {
            enum { INT_START, INT, FRAC_START, FRAC, EXP_START, EXP } state = INT_START;

            for (;; ++st.ptr)
            {
                if (st.ptr == st.txt.size)
                    break;

                auto c = st.txt.data[st.ptr];
                if (c == '-')
                {
                    switch (state)
                    {
                    case INT_START:
                        state = INT;
                        break;
                    case EXP_START:
                        state = EXP;
                        break;
                    default:
                        throw ParseError();
                    }
                }
                else if ((c == 'e') || (c == 'E'))
                {
                    switch (state)
                    {
                    case INT:
                    case FRAC:
                        state = EXP_START;
                        break;
                    default:
                        throw ParseError();
                    }
                }
                else if (c == '.')
                {
                    if (state == INT)
                        state = FRAC_START;
                    else
                        throw ParseError();
                }
                else if (c >= '0' && c <= '9')
                {
                    switch (state)
                    {
                    case INT_START:
                        state = INT;
                        break;
                    case FRAC_START:
                        state = FRAC;
                        break;
                    case EXP_START:
                        state = EXP;
                        break;
                    default:
                        break;
                    }
                }
                else
                    break;
            }

            switch (state)
            {
            case INT_START:
            case FRAC_START:
            case EXP_START:
                throw ParseError();
            default:
                break;
            }
        }

        void read_number(ParserState & st, double & out)
        {
            std::size_t begin = st.ptr;
            skip_number(st);
            out = std::stod(std::string(reinterpret_cast<const char *>(st.txt.data + begin), st.ptr - begin));
        }

        void skip_string(ParserState & st)
        {
            for (;;)
            {
                if (end_of_text(st))
                    throw ParseError();

                auto c = get_symbol(st);

                switch (c)
                {
                case Quote:
                    ++st.ptr;
                    return;
                case '\\':
                    ++st.ptr;
                    if (end_of_text(st))
                        throw ParseError();
                    c = get_symbol(st);
                    switch (c)
                    {
                    case Quote:
                    case '\\':
                    case '/':
                    case 'b':
                    case 'f':
                    case 'n':
                    case 'r':
                    case 't':
                        ++st.ptr;
                        break;
                    case 'u':
                        ++st.ptr;
                        if (st.ptr + 4 > st.txt.size)
                            throw ParseError();
                        auto utf16 = read_utf16_symbol(st);
                        if ((utf16 >= 0xD800) && (utf16 <= 0xDFFF))
                        {
                            if ((st.ptr + 5 >= st.txt.size) || (get_symbol(st) != '\\') || (st.txt.data[st.ptr + 1] != 'u'))
                                throw ParseError();
                            st.ptr += 6;
                        }
                        break;
                    }
                    break;
                default:
                    ++st.ptr;
                }
            }
        }

        void skip_object(ParserState & st)
        {
            for (;;)
            {
                switch (next_token(st))
                {
                case Token::ObjEnd:
                    return;
                case Token::Quote:
                    skip_string(st);
                    if (next_token(st) != Token::Colon)
                        throw ParseError();
                    skip_value(st);

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

        void skip_array(ParserState & st)
        {
            for (;;)
            {
                switch (next_token(st))
                {
                case Token::ArrEnd:
                    return;
                case Token::EOT:
                case Token::ObjEnd:
                    throw ParseError();
                default:
                    --st.ptr;
                    skip_value(st);

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

        void skip_value(ParserState & st)
        {
            switch(next_token(st))
            {
            case Token::ObjBegin:
                skip_object(st);
                break;
            case Token::ArrBegin:
                skip_array(st);
                break;
            case Token::Quote:
                skip_string(st);
                break;
            case Token::Number:
                --st.ptr;
                skip_number(st);
                break;
            default:
                throw ParseError();
            }
        }
    }

    Parser::Parser(utf8_text const & txt)
        : st_{ txt, 0 }
    {
        details::skip_BOM(st_);
    }

    bool Parser::eot()
    {
        return details::end_of_text(st_) || (details::skip_spaces(st_) == details::SSStatus::EOT);
    }

    std::string Parser::parse_string()
    {
        using namespace details;

        if (skip_spaces(st_) != SSStatus::Normal)
            throw ParseError();

        return read_string(st_);
    }

    details::Token Parser::next_token()
    {
        return details::next_token(st_);
    }

    void Parser::expect(details::Token expected)
    {
        auto real = next_token();
        if (real != expected)
            throw ParseError();
    }

    void Parser::expect(boost::string_ref str)
    {
        if (parse_string() != str)
            throw ParseError();
    }
}
