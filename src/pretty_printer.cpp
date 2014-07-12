#include "printer_base.h"

#include <stack>
#include <cassert>

namespace jco
{
    namespace serialization
    {
        struct PrettyPrinter : PrinterBase
        {
            explicit PrettyPrinter(std::ostream & backend)
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

            void print(boost::string_ref)   override;
            void print(double)              override;
            void print(bool)                override;
            void print(std::nullptr_t)      override;

        private:
            void print_indents();
            void pre_print_value();

        private:
            std::ostream & backend_;
            size_t indents_num_ = 0;
            bool after_key_ = false;
        };

        void PrettyPrinter::print_indents()
        {
            for (size_t i = indents_num_; i != 0; --i)
                backend_ << "  ";
        }

        void PrettyPrinter::open_array()
        {
            pre_print_value();
            backend_ << "[" << std::endl;
            ++indents_num_;
        }

        void PrettyPrinter::close_array()
        {
            --indents_num_;
            backend_ << std::endl;
            print_indents();
            backend_ << "]";
        }

        void PrettyPrinter::open_object()
        {
            pre_print_value();
            backend_ << "{" << std::endl;
            ++indents_num_;
        }

        void PrettyPrinter::close_object()
        {
            --indents_num_;
            backend_ << std::endl;
            print_indents();
            backend_  << "}";
        }

        void PrettyPrinter::separate_array_elements()
        {
            backend_ << "," << std::endl;
        }

        void PrettyPrinter::separate_object_fields()
        {
            backend_ << "," << std::endl;
        }

        void PrettyPrinter::key(boost::string_ref key)
        {
            print_indents();
            PrinterBase::print(key);
            backend_ << " : ";
            after_key_ = true;
        }

        void PrettyPrinter::pre_print_value()
        {
            if (after_key_)
                after_key_ = false;
            else
                print_indents();
        }

        void PrettyPrinter::print(boost::string_ref s)
        {
            pre_print_value();
            PrinterBase::print(s);
        }

        void PrettyPrinter::print(double x)
        {
            pre_print_value();
            PrinterBase::print(x);
        }

        void PrettyPrinter::print(bool f)
        {
            pre_print_value();
            PrinterBase::print(f);
        }

        void PrettyPrinter::print(std::nullptr_t)
        {
            pre_print_value();
            PrinterBase::print(nullptr);
        }

        PrinterPtr make_pretty_printer(std::ostream & backend)
        {
            return PrinterPtr(new PrettyPrinter(backend));
        }
    }
}
