#pragma once

#include <memory>
#include <boost/utility/string_ref.hpp>

namespace jco
{
    namespace serialization
    {
        struct Printer
        {
            virtual void print(boost::string_ref)   = 0;
            virtual void print(double)              = 0;
            virtual void print(bool)                = 0;
            virtual void print(std::nullptr_t)      = 0;

            virtual void open_array()               = 0;
            virtual void close_array()              = 0;
            virtual void separate_array_elements()  = 0;

            virtual void open_object()              = 0;
            virtual void close_object()             = 0;
            virtual void key(boost::string_ref)     = 0;
            virtual void separate_object_fields()   = 0;

            virtual ~Printer() {}
        };

        typedef std::unique_ptr<Printer> PrinterPtr;
    }
}
