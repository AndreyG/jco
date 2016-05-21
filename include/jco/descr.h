#pragma once

#define BOOST_PP_VARIADICS

#include <boost/preprocessor/facilities/overload.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>

#define DEF_FIELD_2(type, ct_name) \
    DEF_FIELD_3(type, ct_name, #ct_name)

#define DEF_FIELD_3(type, ct_name, rt_name) \
    ((type, ct_name, rt_name))

#define DEF_FIELD(...) BOOST_PP_OVERLOAD(DEF_FIELD_,__VA_ARGS__)(__VA_ARGS__)

#define GET_TYPE(field)     BOOST_PP_TUPLE_ELEM(0, field)
#define GET_CT_NAME(field)  BOOST_PP_TUPLE_ELEM(1, field)
#define GET_RT_NAME(field)  BOOST_PP_TUPLE_ELEM(2, field)

#define DECLARE_FIELD(r, data, elem) \
    GET_TYPE(elem) GET_CT_NAME(elem);

#define DECLARE_FIELDS(fields) \
    BOOST_PP_SEQ_FOR_EACH(DECLARE_FIELD, fake_data, fields)

#define DEFINE_STRUCT_IMPL(name, fields)        \
    struct name                                 \
    {                                           \
        DECLARE_FIELDS(fields)                  \
    };

#define CALL(r, data, elem) f(s.GET_CT_NAME(elem), GET_RT_NAME(elem));

#define DEFINE_FOREACH(struct_name, fields)                 \
    template<class F>                                       \
    void for_each(struct_name & s, F f)                     \
    {                                                       \
        BOOST_PP_SEQ_FOR_EACH(CALL, fake_data, fields)      \
    }                                                       \

#define DEF_OBJECT(name, fields)        \
    DEFINE_STRUCT_IMPL(name, fields)    \
    DEFINE_FOREACH(name, fields)

