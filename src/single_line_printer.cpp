#include "printer_base.h"

namespace jco
{
    namespace serialization
    {
        struct SingleLinePrinter : PrinterBase
        {
            explicit SingleLinePrinter(std::ostream & backend)
                : PrinterBase(backend)
                , backend_(backend)
            {}

            void open_array()               override;
            void close_array()              override;
            void separate_array_elements()  override;

            void open_object()              override;
            void close_object()             override;
            void key(boost::string_ref)     override;
            void separate_object_fields()   override;

        private:
            std::ostream & backend_;
        };

        void SingleLinePrinter::open_array()
        {
            backend_ << "[";
        }

        void SingleLinePrinter::close_array()
        {
            backend_ << "]";
        }

        void SingleLinePrinter::open_object()
        {
            backend_ << "{ ";
        }

        void SingleLinePrinter::close_object()
        {
            backend_ << " }";
        }

        void SingleLinePrinter::separate_array_elements()
        {
            backend_ << ", ";
        }

        void SingleLinePrinter::separate_object_fields()
        {
            backend_ << ", ";
        }

        void SingleLinePrinter::key(boost::string_ref key)
        {
            print(key);
            backend_ << " : ";
        }

        PrinterPtr make_single_line_printer(std::ostream & backend)
        {
            return PrinterPtr(new SingleLinePrinter(backend));
        }
    }
}
