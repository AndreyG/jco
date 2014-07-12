#include "printer_base.h"

namespace jco
{
    namespace serialization
    {
        void PrinterBase::print(boost::string_ref str)
        {
            backend_ << "\"" << str << "\"";
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
