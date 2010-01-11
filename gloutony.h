#include <boost/multi_array.hpp>
#include "constraints.h"
#include "boost/date_time/local_time/local_time.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "instance.h"
#ifndef GLOUTONY_H
#define GLOUTONY_H

struct Solution
{
    float cost;
    boost::multi_array<float, 3> p1, p2, x;
    boost::multi_array<float, 2> r;
 
    Solution();
    Solution(const Constraints & c, const Instance & data);
    void write(const std::string & filename, const Constraints & c, boost::posix_time::ptime) const;
};

#endif
