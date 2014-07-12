#include "printer.h"

namespace jco
{
    namespace serialization
    {
        struct PrinterBase : Printer
        {
            void print(boost::string_ref)   override;
            void print(double)              override;
            void print(bool)                override;
            void print(std::nullptr_t)      override;

            explicit PrinterBase(std::ostream & backend);

        private:
            std::ostream & backend_;
        };
    }
}
