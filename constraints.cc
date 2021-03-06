#include "constraints.h"
#include "instance.h"

#include <gecode/scheduling.hh>
#include "gecode/minimodel.hh"
#include "gecode/kernel.hh"

#include <boost/foreach.hpp>

#include <boost/assert.hpp>

using namespace Gecode;


Constraints::Constraints()
{
    Instance & data = *Instance::get();

    ha = IntVarArray(*this, data.powerplant2 * data.campaigns, -1, data.weeks);
    has_outage = BoolVarArray(*this, data.powerplant2 * data.campaigns, 0, 1);
    IntVarArray has_outage_int(*this, data.powerplant2 * data.campaigns, 0, 1);

    for(int i=0; i < data.powerplant2 * data.campaigns; i++)
    {
        channel(*this, has_outage[i], has_outage_int[i]);
    }

    powerplant2 = data.powerplant2;
    campaigns = data.campaigns;
    Matrix<IntVarArray> ham(ha, data.powerplant2, data.campaigns);
    Matrix<BoolVarArray> optm(has_outage, data.powerplant2, data.campaigns);
    Matrix<IntVarArray> optm_i(has_outage_int, data.powerplant2, data.campaigns);
    IntVarArgs ends(data.powerplant2 * data.campaigns);
    Matrix<IntVarArgs> endsm(ends, data.powerplant2, data.campaigns);
    std::cout << "Variables created" << std::endl;

    for(int i=0; i < data.powerplant2; i++)
        for(int k=0; k < data.campaigns; k++)
        {
            endsm(i,k) = post(*this, ham(i,k) + data.plants2[i].durations[k]);
            optm(i,k) = post(*this, ~(ham(i,k) >= 0));
            //post(*this, imp(optm(i,k), ham(i,k) >= 0));
            //post(*this, imp(ham(i,k) >= 0, optm(i,k)));
        }


    //CT13 a) Earliest and latest outage
    BOOST_FOREACH(Constraint_13 ct13, data.ct13)
    {
        if(ct13.earliest_stop_time >= 0)
            post(*this, ham(ct13.powerplant_idx,ct13.campaign_idx) >= ct13.earliest_stop_time);
        if(ct13.latest_stop_time > 0)
            post(*this, ham(ct13.powerplant_idx,ct13.campaign_idx) <= ct13.latest_stop_time);
    }
    std::cout << "Ct 13a set" << std::endl;

    
    //CT13 b) two outages can't overlap
    for(int i = 0; i < data.powerplant2; i++)
        for(int k = 1; k < data.campaigns; k++)
        {
            post(*this, tt(optm(i,k) == 0 || ~(ham(i,k) >= ham(i,k-1) + data.plants2[i].durations[k-1])));
        }

    std::cout << "Ct 13b set" << std::endl;

    
    //CT14 min spacing/max overlap between outages
    BOOST_FOREACH(Constraint_14 ct14, data.ct14)
    {
        for(int k=0; k<data.campaigns; k++)
        {
            IntVarArgs start(ct14.set.size());
            IntArgs duration(ct14.set.size());
            BoolVarArgs optional(ct14.set.size());
            for(size_t Am = 0; Am < ct14.set.size(); Am++)
            {
                int i = ct14.set[Am];
                BOOST_ASSERT(i < data.powerplant2);
                start[Am] = ham(i, k);

                //NOTE : unary exige que la durées soit > 0
                //Je pense qu'on risque pas grand chose là.
                if(data.plants2[i].durations[k] + ct14.spacing > 0)
                {
                    duration[Am] = data.plants2[i].durations[k] + ct14.spacing;
                    optional[Am] = optm(i, k);
                }
                else 
                {
                    duration[Am] = 1;
                    optional[Am] = BoolVar(*this, 0, 0);
                }
                status();
            }
            unary(*this, start, duration, optional);
        }
    }

    std::cout << "Ct 14 set" << std::endl;

    //CT 15
    BOOST_FOREACH(Constraint_15 ct15, data.ct15)
    {
        for(int k=0; k < data.campaigns; k++)
        {
            size_t n = ct15.set.size();
            BoolVarArgs in_period(n);
            IntVarArgs start(n);
            IntArgs duration(n);
            for(size_t Am = 0; Am < n; Am++)
            {
                int i = ct15.set[Am];
                in_period[Am] = post(*this, ham(i,k) >= ct15.first_week && endsm(i,k) <= ct15.last_week && optm(i,k));
                start[Am] = ham(i, k);
                duration[Am] = data.plants2[i].durations[k] + ct15.spacing;
            }
            unary(*this, start, duration, in_period);
        }
    }
    std::cout << "Ct 15 set" << std::endl;

    
    //CT16 Minimum spacing between decoupling dates
    BOOST_FOREACH(Constraint_16 ct16, data.ct16)
    {
        for(int k=0; k<data.campaigns; k++)
        {
            IntVarArgs start(ct16.set.size());
            IntArgs duration(ct16.set.size());
            BoolVarArgs optional(ct16.set.size());
            for(size_t Am = 0; Am < ct16.set.size(); Am++)
            {
                BOOST_ASSERT(ct16.set[Am] < data.powerplant2);
                start[Am] = ham(ct16.set[Am], k);
                duration[Am] = ct16.spacing;
                optional[Am] = optm(ct16.set[Am], k);
            }
            unary(*this, start, duration, optional);
        }
    }
    std::cout << "Ct 16 set" << std::endl;

    
    //CT 17 Minimum spacing between coupling
    BOOST_FOREACH(Constraint_17 ct17, data.ct17)
    {
        for(int k=0; k<data.campaigns; k++)
        {
            IntVarArgs start(ct17.set.size());
            IntArgs duration(ct17.set.size());
            BoolVarArgs optional(ct17.set.size());
            for(size_t Am = 0; Am < ct17.set.size(); Am++)
            {
                BOOST_ASSERT(ct17.set[Am] < data.powerplant2);
                start[Am] = endsm(ct17.set[Am], k);
                duration[Am] = ct17.spacing;
                optional[Am] = optm(ct17.set[Am], k);
            }
            unary(*this, start, duration, optional);
        }
    }
    std::cout << "Ct 17 set" << std::endl;

    
    //CT 18 Minimum spacing between coupling & decoupling
    //WTF ?! Quelle différence avec CT14 ?!
    BOOST_FOREACH(Constraint_18 ct18, data.ct18)
    {
        for(int k=0; k<data.campaigns; k++)
        {
            IntVarArgs start(ct18.set.size());
            IntArgs duration(ct18.set.size());
            BoolVarArgs optional(ct18.set.size());
            for(size_t Am = 0; Am < ct18.set.size(); Am++)
            {
                int i = ct18.set[Am];
                BOOST_ASSERT(i < data.powerplant2);
                start[Am] = ham(i, k);
                duration[Am] = data.plants2[i].durations[k] + ct18.spacing;
                optional[Am] = optm(i, k);
            }
            unary(*this, start, duration, optional);
        }
    }
    std::cout << "Ct 18 set" << std::endl;

    
    
    //CT 19 Ressource constraint
    //TODO : réfléchir si on peut considérer toutes les campagnes comme des machines parallèles
    //TODO bis : surtout réussir à faire en sorte que cette contrainte ne plombe pas tout...
    BOOST_FOREACH(Constraint_19 ct19, data.ct19)
    {
        IntVarArgs start(ct19.set.size());
        IntVarArgs end(ct19.set.size());
        IntArgs duration(ct19.set.size());
        IntVarArgs height(ct19.set.size());
        IntArgs machine(ct19.set.size());
        IntArgs limit(1);
        for(int k=0; k < data.campaigns; k++)
        {
            for(size_t Am = 0; Am < ct19.set.size(); Am++)
            {
                BOOST_ASSERT(ct19.set[Am] < data.powerplant2);
                BOOST_ASSERT(ct19.set[Am] == ct19.periods[Am].powerplant);

                start[Am] = post(*this, ham(ct19.set[Am],k) + ct19.periods[Am].start[k]); 
                end[Am] = post(*this, start[Am] +ct19.periods[Am].duration[k] );
                duration[Am] = ct19.periods[Am].duration[k];
                height[Am] = optm_i(ct19.set[Am], k);
                machine[Am] = 0;
            }
            limit[0] = ct19.quantity;
            cumulatives(*this, machine, start, duration, end, height, limit, true);
        }
    }
    std::cout << "Ct 19 set" << std::endl;
    

    //CT 20 Maximum outage overlapping during one week
    BOOST_FOREACH(Constraint_20 ct20, data.ct20)
    {
        for(int k=0; k < data.campaigns; k++)
        {
            IntVarArray overlaps(*this, ct20.set.size(), 0, 1);
            for(size_t Am = 0; Am < ct20.set.size(); Am++)
            {
                BOOST_ASSERT(ct20.set[Am] < data.powerplant2);
                overlaps[Am] = channel(*this, post(*this, ~(ham(ct20.set[Am], k) >= ct20.week) && ~(endsm(ct20.set[Am], k) <= ct20.week)));
            }
            count(*this, overlaps, 1, IRT_LQ, ct20.max);

        }
    }
    std::cout << "Ct 20 set" << std::endl;
    
/*
    //CT21 Offline capacity
    // TODO: trouver une autre implem pour cette contrainte
    // Elle bouffe bcp de mémoire ! (et un peu de temps aussi)
    // Probablement à cause de ct21.max qui est très grand
    // Utiliser cummulative ?
    BOOST_FOREACH(Constraint_21 ct21, data.ct21)
    {
        int steps_per_week = data.timesteps/data.weeks;
        for(int k = 0; k < data.campaigns; k++)
        {
            for(int h = ct21.start; h <= ct21.end; h++)
            {
                int tmin = h * steps_per_week;
                int tmax = (h+1) * steps_per_week;
                IntArgs pmax(ct21.set.size());
                IntVarArray used(*this, ct21.set.size(), 0, 1);
                for(size_t Am = 0; Am < ct21.set.size(); Am++)
                {
                    int i = ct21.set[Am];
                    int pmin = data.plants2[i].pmax[tmin];
                    for(int t = tmin; t < tmax; t++)
                    {
                        if(pmin > data.plants2[i].pmax[t])
                            pmin = data.plants2[i].pmax[t];
                    }
                    pmax[Am] = pmin;
                    used[Am] = channel(*this,
                            post(*this, ham(i,k) >= h && endsm(i,k) <= h));
                }
                linear(*this, pmax, used, IRT_LQ, int(ct21.max));
            }
        }

    }
    */
    branch(*this, has_outage, INT_VAR_SIZE_MIN, INT_VAL_MAX);
    branch(*this, ha, INT_VAR_SIZE_MIN, INT_VAL_RND);

}

bool Constraints::is_out(int i, int h) const
{
    Instance & data = *Instance::get();
    for(int k = 0; k < data.campaigns; k++)
    {
        if( ha[i + k * powerplant2].val() <= h && ha[i+k * powerplant2].val() + data.plants2[i].durations[k] >= h)
            return true;
    }
    return false;

}

/// Constructor for cloning \a s
Constraints::Constraints(bool share, Constraints & s) :
    Space(share,s), powerplant2(s.powerplant2), campaigns(s.campaigns)
{
    ha.update(*this, share, s.ha);
    has_outage.update(*this, share, s.has_outage);
}

Space* Constraints::copy(bool share)
{
    return new Constraints(share, *this);
}

void Constraints::print(std::ostream& os) const
{
    for(int i=0; i < powerplant2; i++)
    {
        os << "Plant n°" << i << std::endl;
        for(int k = 0; k < campaigns; k++)
        {
            os << "  " << ha[i + k * powerplant2];
            if(has_outage[i + k * powerplant2].assigned() && has_outage[i + k * powerplant2].val() == 0)
                os << "X";
            else
                os << " ";
        }
        os << std::endl;

    }
}

int Constraints::get_campaign(int i, size_t t) const
{
    Instance & data = *Instance::get();
    int steps_per_week = data.timesteps / data.weeks;
    int h = t / steps_per_week;
    for (int k = 0; k < campaigns; k++)
    {
        if((ha[i + k*powerplant2].val() > h))
           return k - 1; 
    }
    return campaigns - 1;
}

bool Constraints::first_outage(int i, size_t t) const
{
    Instance & data = *Instance::get();
    int steps_per_week = data.timesteps / data.weeks;

    for (int k = 0; k < data.campaigns; k++)
    {
        if(ha[i + k*powerplant2].val() * steps_per_week == t)
            return true;
    }
    return false;
}
