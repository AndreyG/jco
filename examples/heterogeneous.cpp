#include "jco/jco.h"

#include <iostream>
#include <sstream>

class IMyInterface
{
public:
    virtual ~IMyInterface() = 0;

    virtual std::string to_string() const = 0;
};

IMyInterface::~IMyInterface() {}

#define DECLARE_JCO_FACTORY(classname, jconame)                                         \
public:                                                                                 \
    static constexpr const char * JCO_CLASS_NAME = #jconame;                            \
    static std::unique_ptr<classname> from_jco(jco::Parser &);                          \
    void serialize_descr(jco::serialization::out_stream &) const;                       \
    friend jco::serialization::out_stream & operator <<                                 \
        (jco::serialization::out_stream & out, classname const & o)                     \
    {                                                                                   \
        using namespace jco::serialization;                                             \
        object_scope os(out);                                                           \
        out << key("type") << value(JCO_CLASS_NAME)                                     \
            << key("description") << value(object);                                     \
        o.serialize_descr(out);                                                         \
        return out;                                                                     \
    }

#define REGISTER_JCO_FACTORY(parser, classname) \
    parser.register_factory(classname::JCO_CLASS_NAME, &classname::from_jco);

namespace mynamespace
{
    class Impl1 : public IMyInterface
    {
        DECLARE_JCO_FACTORY(Impl1, mynamespace::Impl1)

        Impl1(std::string a, double b)
            : a_(std::move(a))
            , b_(b)
        {}

        std::string to_string() const override
        {
            return a_ + " " + std::to_string(b_);
        }

        bool operator == (Impl1 const & other) const
        {
            return (a_ == other.a_) && (b_ == other.b_);
        }

    private:
        std::string a_;
        double b_;
    };

    namespace details1
    {
        DEF_OBJECT(jco_repr,
            DEF_FIELD(std::string, a)
            DEF_FIELD(double, b)
        )
    }

    std::unique_ptr<Impl1> Impl1::from_jco(jco::Parser & parser)
    {
        auto jr = parser.parse<details1::jco_repr>();
        return std::make_unique<Impl1>(std::move(jr.a), jr.b);
    }

    void Impl1::serialize_descr(jco::serialization::out_stream & out) const
    {
        using namespace jco::serialization;

        object_scope os(out);

        out << key("a") << value(a_)
            << key("b") << value(b_);
    }

    class Impl2 : public IMyInterface
    {
        DECLARE_JCO_FACTORY(Impl2, mynamespace::Impl2)

        Impl2(double x, double y, std::string name)
            : x_(x)
            , y_(y)
            , name_(name)
        {}

        std::string to_string() const override
        {
            std::ostringstream ss;
            ss << "(" << x_ << ", " << y_ << "): " << name_;
            return ss.str();
        }

    private:
        double x_, y_;
        std::string name_;
    };

    namespace details2
    {
        DEF_OBJECT(jco_repr,
            DEF_FIELD(double, x)
            DEF_FIELD(double, y)
            DEF_FIELD(std::string, name)
        )
    }

    std::unique_ptr<Impl2> Impl2::from_jco(jco::Parser & parser)
    {
        auto jr = parser.parse<details2::jco_repr>();
        return std::make_unique<Impl2>(jr.x, jr.y, std::move(jr.name));
    }

    void Impl2::serialize_descr(jco::serialization::out_stream & out) const
    {
        using namespace jco::serialization;

        object_scope os(out);

        out << key("x")     << value(x_)
            << key("y")     << value(y_)
            << key("name")  << value(name_);
    }

    Impl1 obj1("AAA", 239);
    std::string serialized1 = jco::serialization::to_string(obj1);
}

int main()
{
    std::cout << "\n" << mynamespace::serialized1 << std::endl;

    {
        using namespace jco::serialization;
        out_stream out(std::cout);
        array_scope as(out);
        out << mynamespace::Impl1("AAA", 239) << mynamespace::Impl2(1, 2, "3");
    }

    std::cout << std::endl;

    jco::TypedParser<IMyInterface> parser;
    REGISTER_JCO_FACTORY(parser, mynamespace::Impl1);
    REGISTER_JCO_FACTORY(parser, mynamespace::Impl2);

    auto deserialized1 = parser.parse_single(jco::from_string(mynamespace::serialized1));
    assert(mynamespace::obj1 == dynamic_cast<mynamespace::Impl1 &>(*deserialized1));

    auto s1 = "{\"type\" : \"mynamespace::Impl1\", \"description\" : { \"a\" : \"AAA\", \"b\" : 239 } }";
    auto s2 = "{\"type\" : \"mynamespace::Impl2\", \"description\" : { \"y\" : 30, \"name\" : \"QQQ\", \"x\" : 566 } }";

    std::cout << parser.parse_single(jco::from_string(s1))->to_string() << std::endl;
    std::cout << parser.parse_single(jco::from_string(s2))->to_string() << std::endl;
}