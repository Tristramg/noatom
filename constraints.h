#include "instance.h"

#include "gecode/int.hh"
#include <gecode/driver.hh>

#ifndef CONSTRAINTS_H
#define CONSTRAINTS_H

using namespace Gecode;

class Constraints : public Gecode::Space 
{
    int powerplant2;
    int campaigns;
    BoolVarArray has_outage;
    public:
    IntVarArray ha;
        Constraints (const Options &);
        Constraints(bool share, Constraints & s);
        virtual Gecode::Space* copy(bool share);
        virtual void print(std::ostream& os) const;
        bool is_out(int plant, int week) const;
        bool first_outage(int plant, size_t timestep) const;
        int get_campaign(int plant, size_t timestep) const;
 
};
#endif
