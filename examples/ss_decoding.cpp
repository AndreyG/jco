#include <iostream>
#include <cstring>

#include "jco/jco.h"

DEF_OBJECT(GeoPoint,
    DEF_FIELD(double, lat)
    DEF_FIELD(double, lon, "lng")
)

template<class Stream>
Stream& operator << (Stream& out, GeoPoint const & pt)
{
    return out << "(lat: " << pt.lat << ", lon: " << pt.lon << ")";
}

DEF_OBJECT(Bounds,
    DEF_FIELD(GeoPoint, southwest)
    DEF_FIELD(GeoPoint, northeast)
)

const char * serialized = "{ \
    \"southwest\" : {\"lat\" : 59.7452, \"lng\": 30.0903}, \
    \"northeast\" : {\"lat\" : 60.0897, \"lng\": 30.5598}  \
    }";

int main()
{
    auto bounds = jco::parse<Bounds>({ serialized, strlen(serialized) });
    std::cout << bounds.southwest << ", " << bounds.northeast << std::endl;
}
