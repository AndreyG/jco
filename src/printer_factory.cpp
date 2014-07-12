#include "jco/serialization.h"

#include "printer.h"

namespace jco
{
    namespace serialization
    {
        PrinterPtr make_single_line_printer (std::ostream & backend);
        PrinterPtr make_pretty_printer      (std::ostream & backend);

        PrinterPtr make_printer(std::ostream & backend, Style style)
        {
            switch (style)
            {
            case Style::SingleLine:
                return make_single_line_printer(backend);
            case Style::Pretty:
                return make_pretty_printer(backend);
            default:
                throw std::logic_error("unknown style");
            }
        }
    }
}
