#include "TimeZone.h"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/local_time_adjustor.hpp>
#include <boost/date_time/c_local_time_adjustor.hpp>

using boost::posix_time::ptime;
using boost::posix_time::microsec_clock;
using boost::posix_time::second_clock;
using boost::posix_time::to_iso_extended_string;
using boost::posix_time::to_simple_string;
using boost::gregorian::day_clock;

std::string TimeZone::getUtcDateTime_ms()
{
    ptime todayUtc(day_clock::universal_day(), microsec_clock::universal_time().time_of_day());

    std::stringstream ss;
    ss << to_iso_extended_string(todayUtc) << "Z";
    return ss.str();
}

std::string TimeZone::getUtcDateTimeSimple()
{
    ptime todayUtc(day_clock::universal_day(), second_clock::universal_time().time_of_day());
    return to_simple_string(todayUtc);
}
