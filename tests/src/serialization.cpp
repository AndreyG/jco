#include <iostream>
#include <gtest/gtest.h>

#include "jco/serialization.h"

namespace
{
    using namespace jco::serialization;

    TEST(serialization, object)
    {
        std::ostringstream ss;
        {
            out_stream out(ss, Style::Pretty);
            object_scope os(out);

            out << key("\\Key\n")   << value("Value");
            out << key("null")      << value(nullptr);
            out << key("Array")     << value(array);

            array_stream(out) << true
                              << 999.
                              << "string"
                              << nullptr
                              << "\\\a\n\r\b\tabcABC\"";
        }
        std::string expected =
                "{\n"
                "  \"\\\\Key\\n\" : \"Value\",\n"
                "  \"null\" : null,\n"
                "  \"Array\" : [\n"
                "    true,\n"
                "    999,\n"
                "    \"string\",\n"
                "    null,\n"
                "    \"\\\\\\u0007\\n\\r\\b\\tabcABC\\\"\"\n"
                "  ]\n"
                "}";
        EXPECT_EQ(ss.str(), expected);
    }
}
