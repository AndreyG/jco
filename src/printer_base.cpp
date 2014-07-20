#include "printer_base.h"

namespace jco
{
    namespace serialization
    {
        void PrinterBase::print(boost::string_ref str)
        {
            backend_ << "\"";
            for (char c : str)
            {
                switch (c)
                {
                case '\"':
                    backend_ << "\\\"";
                    break;
                case '\\':
                    backend_ << "\\\\";
                    break;
                case '\b':
                    backend_ << "\\b";
                    break;
                case '\n':
                    backend_ << "\\n";
                    break;
                case '\r':
                    backend_ << "\\r";
                    break;
                case '\t':
                    backend_ << "\\t";
                    break;
                default:
                    if (c < 32)
                    {
                        int i = c;
                        backend_ << "\\u00" << ((i & 0xF0) >> 4);
                        i &= 0xF;
                        if (i < 10)
                            backend_ << i;
                        else
                            backend_ << char('A' + (i - 10));
                    }
                    else
                        backend_ << c;
                }
            }
            backend_ << "\"";
        }

        void PrinterBase::print(bool f)
        {
            backend_ << (f ? "true" : "false");
        }

        void PrinterBase::print(double x)
        {
            backend_ << x;
        }

        void PrinterBase::print(std::nullptr_t)
        {
            backend_ << "null";
        }

        PrinterBase::PrinterBase(std::ostream & backend)
            : backend_(backend)
        {}
    }
}
