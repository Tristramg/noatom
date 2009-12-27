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
    IntVarArray ha;
    BoolVarArray has_outage;
    BoolVarArray out;
    public:
        Constraints (const Options &);
        Constraints(bool share, Constraints & s);
        virtual Gecode::Space* copy(bool share);
        virtual void print(std::ostream& os) const;
 
};
#endif
