#include <iostream>
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_lists.hpp> 
#include <boost/spirit/include/classic_file_iterator.hpp>
#include <boost/spirit/include/classic_push_back_actor.hpp>

#include "instance.h"
#include "coin/ClpSimplex.hpp"
#include <boost/assert.hpp>
using namespace BOOST_SPIRIT_CLASSIC_NS;
typedef file_iterator<char>   iterator_t;
typedef scanner<iterator_t>     scanner_t;
typedef rule<scanner_t>         rule_t;

struct VarIndex
{
    int offset;
    static int max;
    int max1, max2, max3;

    VarIndex(int max1, int max2, int max3) : offset(max), max1(max1), max2(max2), max3(max3)
    {
        max += max1 * max2 * max3;
    }

    VarIndex(int max1, int max2) : offset(max), max1(max1), max2(max2), max3(1)
    {
        max *= max1 * max2 * max3;
    }

    int operator()(int v1, int v2) const
    {
        return this->operator()(v1, v2, 0);
    }

    int operator()(int v1, int v2, int v3) const
    {
        BOOST_ASSERT(v1 < max1);
        BOOST_ASSERT(v2 < max2);
        BOOST_ASSERT(v3 < max3);
        return offset + v1 + v2 * max1 + v3 * max1 * max2; 
    }
};

int VarIndex::max = 0;

int main(int argc, char ** argv)
{
    if(argc != 2)
    {
        std::cout << "Use: " << argv[0] << " data_file" << std::endl;
        return 1;
    }

    file_iterator<> first(argv[1]);
    if(!first)
    {
        std::cout << "Unable to open file " << argv[1] << std::endl;
        return 1;
    }
    file_iterator<> last = first.make_end();

    size_t timesteps = 0;
    int weeks = 0;
    int campaigns = 0;
    int scenario = 0;
    float epsilon = 0;
    int powerplant1 = 0;
    int powerplant2 = 0;
    int constraint13 = 0;
    int constraint14 = 0;
    int constraint15 = 0;
    int constraint16 = 0;
    int constraint17 = 0;
    int constraint18 = 0;
    int constraint19 = 0;
    int constraint20 = 0;
    int constraint21 = 0;
    std::vector<int> durations;

    rule_t header_r = "begin main" >> eol_p
        >> "timesteps " >> int_p[assign_a(timesteps)] >> eol_p
        >> "weeks " >> int_p[assign_a(weeks)] >> eol_p
        >> "campaigns " >> int_p[assign_a(campaigns)] >> eol_p
        >> "scenario " >> int_p[assign_a(scenario)] >> eol_p
        >> "epsilon " >> real_p[assign_a(epsilon)] >> eol_p
        >> "powerplant1 " >> int_p[assign_a(powerplant1)] >> eol_p
        >> "powerplant2 " >> int_p[assign_a(powerplant2)] >> eol_p
        >> "constraint13 " >> int_p[assign_a(constraint13)] >> eol_p
        >> "constraint14 " >> int_p[assign_a(constraint14)] >> eol_p
        >> "constraint15 " >> int_p[assign_a(constraint15)] >> eol_p
        >> "constraint16 " >> int_p[assign_a(constraint16)] >> eol_p
        >> "constraint17 " >> int_p[assign_a(constraint17)] >> eol_p
        >> "constraint18 " >> int_p[assign_a(constraint18)] >> eol_p
        >> "constraint19 " >> int_p[assign_a(constraint19)] >> eol_p
        >> "constraint20 " >> int_p[assign_a(constraint20)] >> eol_p
        >> "constraint21 " >> int_p[assign_a(constraint21)] >> eol_p
        >> "durations" >> *(' '|int_p[push_back_a(durations)]) >> eol_p;


    std::cout << "Parsing header: " << std::flush;
    parse_info<iterator_t> info = parse(first, last, header_r);
    BOOST_ASSERT(info.hit);
    BOOST_ASSERT(durations.size() == timesteps);
    std::cout << "Ok" << std::endl; 


    std::cout << "Parsing demand: " << std::flush;
    std::vector<std::vector<double> > demand(scenario);

    for(int i=0; i < scenario; i++)
    {
        demand[i].reserve(timesteps);
        rule_t demand_r = "demand" >> *(' '|real_p[push_back_a(demand[i])]) >> eol_p;
        info = parse(info.stop, last, demand_r);
        BOOST_ASSERT(info.hit);
        BOOST_ASSERT(demand[i].size() == timesteps);
    }
    std::cout << "Ok" << std::endl;

    info = parse(info.stop, last, "end main" >> eol_p);
    BOOST_ASSERT(info.hit);
    std::cout << "Main section done" << std::endl << std::endl;

    std::cout << "Parsing Powerplant type1: " << std::flush;
    std::vector<Powerplant_t1> plants1 (powerplant1, Powerplant_t1(scenario, timesteps));
    for(int i=0; i < powerplant1; i++)
    {
        rule_t type1_r = "begin powerplant" >> eol_p
            >> "name " >> (*~space_p)[assign_a(plants1[i].name)] >> eol_p
            >> "type 1" >> eol_p
            >> "index " >> int_p[assign_a(plants1[i].index)] >> eol_p
            >> "scenario " >> int_p >> eol_p
            >> "timesteps " >> int_p >> eol_p;

        info = parse(info.stop, last, type1_r);
        BOOST_ASSERT(info.hit);

        for(int s=0; s < scenario; s++)
        {
            rule_t data_r = "pmin" >> *(' '|real_p[push_back_a(plants1[i].pmin[s])]) >> eol_p
                >> "pmax" >> *(' '|real_p[push_back_a(plants1[i].pmax[s])]) >> eol_p
                >> "cost" >> *(' '|real_p[push_back_a(plants1[i].cost[s])]) >> eol_p;
            info = parse(info.stop, last, data_r);
            BOOST_ASSERT(info.hit);
            BOOST_ASSERT(plants1[i].pmin[s].size() == timesteps);
            BOOST_ASSERT(plants1[i].pmax[s].size() == timesteps);
            BOOST_ASSERT(plants1[i].cost[s].size() == timesteps);
        }
        info = parse(info.stop, last, "end powerplant" >> eol_p);
        BOOST_ASSERT(info.hit);
    }
    std::cout << "Ok" << std::endl;

    std::cout << "Parsing Powerplant type2: " << std::flush;
    std::vector<Powerplant_t2> plants2 (powerplant2, Powerplant_t2());
    for(int i=0; i < powerplant2; i++)
    {
        rule_t type2_r = "begin powerplant" >> eol_p
            >> "name " >> (*~space_p)[assign_a(plants2[i].name)] >> eol_p
            >> "type 2" >> eol_p
            >> "index " >> int_p[assign_a(plants2[i].index)] >> eol_p
            >> "stock " >> real_p[assign_a(plants2[i].stock)] >> eol_p
            >> "campaigns " >> int_p[assign_a(plants2[i].campaigns)] >> eol_p
            >> "durations" >> *(' '|int_p[push_back_a(plants2[i].durations)]) >> eol_p 
            >> "current_campaign_max_modulus " >> int_p[assign_a(plants2[i].current_max_modulus)] >> eol_p
            >> "max_modulus" >> *(' '|int_p[push_back_a(plants2[i].max_modulus)]) >> eol_p
            >> "max_refuel" >> *(' '|int_p[push_back_a(plants2[i].max_refuel)]) >> eol_p
            >> "min_refuel" >> *(' '|int_p[push_back_a(plants2[i].min_refuel)]) >> eol_p
            >> "refuel_ratio" >> *(' '|int_p[push_back_a(plants2[i].refuel_ratio)]) >> eol_p
            >> "current_campaign_stock_threshold " >> int_p[assign_a(plants2[i].current_stock_threshold)] >> eol_p
            >> "stock_threshold" >> *(' '|int_p[push_back_a(plants2[i].stock_threshold)]) >> eol_p
            >> "pmax" >> *(' '|real_p[push_back_a(plants2[i].pmax)]) >> eol_p
            >> "max_stock_before_refueling" >> *(' '|int_p[push_back_a(plants2[i].max_stock_before_refueling)]) >> eol_p
            >> "max_stock_after_refueling" >> *(' '|int_p[push_back_a(plants2[i].max_stock_after_refueling)]) >> eol_p
            >> "refueling_cost" >> *(' '|real_p[push_back_a(plants2[i].refueling_cost)]) >> eol_p
            >> "fuel_price " >> real_p[assign_a(plants2[i].fuel_price)] >> eol_p
            >> "begin current_campaign_profile" >> eol_p
            >> "profile_points " >> int_p >> eol_p
            >> "decrease_profile" >> *(' ' >> int_p[push_back_a(plants2[i].current_decrease_profile_idx)] >> ' ' >> real_p[push_back_a(plants2[i].current_decrease_profile_val)]) >> eol_p
            >> "end current_campaign_profile" >> eol_p;
        info = parse(info.stop, last, type2_r);
        BOOST_ASSERT(info.hit);

        plants2[i].decrease_profile_idx.resize(plants2[i].campaigns);
        plants2[i].decrease_profile_val.resize(plants2[i].campaigns);
        int profile_points;
        for(int s=0; s < plants2[i].campaigns; s++)
        {
            rule_t profile_r = "begin profile" >> eol_p
                >> "campaign_profile " >> int_p >> eol_p
                >> "profile_points " >> int_p[assign_a(profile_points)] >> eol_p
                >> "decrease_profile" >> *(' ' >> int_p[push_back_a(plants2[i].decrease_profile_idx[s])] >> ' ' >> real_p[push_back_a(plants2[i].decrease_profile_val[s])]) >> eol_p
                >> "end profile" >> eol_p;

            info = parse(info.stop, last, profile_r);
            BOOST_ASSERT(info.hit);
        }
        info = parse(info.stop, last, "end powerplant" >> eol_p);
        BOOST_ASSERT(info.hit);
    }
    std::cout << "Ok" << std::endl;

    std::cout << "Parsing constraints type 13: " << std::flush;
    std::vector<Constraint_13> ct13(constraint13);
    for(int i=0; i < constraint13; i++)
    {
        rule_t ct13_r = "begin constraint" >> eol_p
            >> "type 13" >> eol_p
            >> "index " >> int_p >> eol_p
            >> "powerplant " >> int_p[assign_a(ct13[i].powerplant_idx)] >> eol_p
            >> "campaign " >> int_p[assign_a(ct13[i].campaign_idx)] >> eol_p
            >> "earliest_stop_time " >> int_p[assign_a(ct13[i].earliest_stop_time)] >> eol_p
            >> "latest_stop_time " >> int_p[assign_a(ct13[i].latest_stop_time)] >> eol_p
            >> "end constraint" >> eol_p;
        info = parse(info.stop, last, ct13_r);
        BOOST_ASSERT(info.hit);
    }
    std::cout << "Ok" << std::endl;

    std::cout << "Parsing constraints type 14: " << std::flush;
    std::vector<Constraint_14> ct14(constraint14);
    for(int i=0; i < constraint14; i++)
    {
        rule_t ct14_r = "begin constraint" >> eol_p
            >> "type 14" >> eol_p
            >> "index " >> int_p >> eol_p
            >> "set" >> *(' '|int_p[push_back_a(ct14[i].set)]) >> eol_p
            >> "spacing " >> int_p[assign_a(ct14[i].spacing)] >> eol_p
            >> "end constraint" >> eol_p;
        info = parse(info.stop, last, ct14_r);
        BOOST_ASSERT(info.hit);
    }
    std::cout << "Ok" << std::endl;

    std::cout << "Parsing constraints type 15: " << std::flush;
    std::vector<Constraint_15> ct15(constraint15);
    for(int i=0; i < constraint15; i++)
    {
        rule_t ct15_r = "begin constraint" >> eol_p
            >> "type 15" >> eol_p
            >> "index " >> int_p >> eol_p
            >> "set" >> *(' '|int_p[push_back_a(ct15[i].set)]) >> eol_p
            >> "spacing " >> int_p[assign_a(ct15[i].spacing)] >> eol_p
            >> "first_week_of_the_constraint " >> int_p[assign_a(ct15[i].first_week)] >> eol_p
            >> "last_week_of_the_constraint " >> int_p[assign_a(ct15[i].last_week)] >> eol_p
            >> "end constraint" >> eol_p;
        info = parse(info.stop, last, ct15_r);
        BOOST_ASSERT(info.hit);
    }
    std::cout << "Ok" << std::endl;

    std::cout << "Parsing constraints type 16: " << std::flush;
    std::vector<Constraint_16> ct16(constraint16);
    for(int i=0; i < constraint16; i++)
    {
        rule_t ct16_r = "begin constraint" >> eol_p
            >> "type 16" >> eol_p
            >> "index " >> int_p >> eol_p
            >> "set" >> *(' '|int_p[push_back_a(ct16[i].set)]) >> eol_p
            >> "spacing " >> int_p[assign_a(ct16[i].spacing)] >> eol_p
            >> "end constraint" >> eol_p;
        info = parse(info.stop, last, ct16_r);
        BOOST_ASSERT(info.hit);
    }
    std::cout << "Ok" << std::endl;

    std::cout << "Parsing constraints type 17: " << std::flush;
    std::vector<Constraint_17> ct17(constraint17);
    for(int i=0; i < constraint17; i++)
    {
        rule_t ct17_r = "begin constraint" >> eol_p
            >> "type 17" >> eol_p
            >> "index " >> int_p >> eol_p
            >> "set" >> *(' '|int_p[push_back_a(ct17[i].set)]) >> eol_p
            >> "spacing " >> int_p[assign_a(ct17[i].spacing)] >> eol_p
            >> "end constraint" >> eol_p;
        info = parse(info.stop, last, ct17_r);
        BOOST_ASSERT(info.hit);
    }
    std::cout << "Ok" << std::endl;

    std::cout << "Parsing constraints type 18: " << std::flush;
    std::vector<Constraint_18> ct18(constraint18);
    for(int i=0; i < constraint18; i++)
    {
        rule_t ct18_r = "begin constraint" >> eol_p
            >> "type 18" >> eol_p
            >> "index " >> int_p >> eol_p
            >> "set" >> *(' '|int_p[push_back_a(ct18[i].set)]) >> eol_p
            >> "spacing " >> int_p[assign_a(ct18[i].spacing)] >> eol_p
            >> "end constraint" >> eol_p;
        info = parse(info.stop, last, ct18_r);
        BOOST_ASSERT(info.hit);
    }
    std::cout << "Ok" << std::endl;

    std::cout << "Parsing constraints type 19: " << std::flush;
    std::vector<Constraint_19> ct19(constraint19);
    for(int i=0; i < constraint19; i++)
    {
        rule_t ct19_r = "begin constraint" >> eol_p
            >> "type 19" >> eol_p
            >> "index " >> int_p >> eol_p
            >> "quantity " >> int_p[assign_a(ct19[i].quantity)] >> eol_p
            >> "set" >> *(' '|int_p[push_back_a(ct19[i].set)]) >> eol_p;
        info = parse(info.stop, last, ct19_r);
        BOOST_ASSERT(info.hit);

        ct19[i].periods.resize(ct19[i].set.size());

        for(size_t j=0; j < ct19[i].set.size(); j++)
        {
            ct19_r = "begin period" >> eol_p
                >> "powerplant " >> int_p[assign_a(ct19[i].periods[j].powerplant)] >> eol_p
                >> "start" >> *(' '|int_p[push_back_a(ct19[i].periods[j].start)]) >> eol_p
                >> "duration" >> *(' '|int_p[push_back_a(ct19[i].periods[j].duration)]) >> eol_p
                >> "end period" >> eol_p;
            info = parse(info.stop, last, ct19_r);
            BOOST_ASSERT(info.hit);
        }
        info = parse(info.stop, last, "end constraint" >> eol_p);
        BOOST_ASSERT(info.hit);
    }
    std::cout << "Ok" << std::endl;

    std::cout << "Parsing constraints type 20: " << std::flush;
    std::vector<Constraint_20> ct20(constraint20);
    for(int i=0; i < constraint20; i++)
    {
        std::string foo;
        rule_t ct20_r = "begin constraint" >> eol_p
            >> "type 20" >> eol_p
            >> "index " >> int_p >> eol_p
            >> "week " >> int_p[assign_a(ct20[i].week)] >> eol_p
            >> "set" >> *(' '|int_p[push_back_a(ct20[i].set)]) >> eol_p
            >> "max " >> int_p[assign_a(ct20[i].max)] >> eol_p
            >> "end constraint" >> eol_p;
        info = parse(info.stop, last, ct20_r);
        BOOST_ASSERT(info.hit);
    }
    std::cout << "Ok" << std::endl;

    std::cout << "Parsing constraints type 21: " << std::flush;
    std::vector<Constraint_21> ct21(constraint21);
    for(int i=0; i < constraint21; i++)
    {
        rule_t ct21_r = "begin constraint" >> eol_p
            >> "type 21" >> eol_p
            >> "index " >> int_p >> eol_p
            >> "set" >> *(' '|int_p[push_back_a(ct21[i].set)]) >> eol_p
            >> "startend " >> int_p[assign_a(ct21[i].start)] >> " " >> int_p[assign_a(ct21[i].end)] >> eol_p
            >> "max " >> real_p[assign_a(ct21[i].end)] >> eol_p
            >> "end constraint" >> *eol_p;
        info = parse(info.stop, last, ct21_r);
        BOOST_ASSERT(info.hit);
    }
    std::cout << "Ok" << std::endl;



    //Let's manage the variables !
    VarIndex p1(powerplant1, timesteps, scenario); //p(j,t,s)
    VarIndex p2(powerplant2, timesteps, scenario); //p(i,t,s)
    VarIndex r(powerplant2, campaigns); // r(i,k)
    VarIndex x(powerplant2, timesteps, scenario); //x(i, t, s)
    int numCols = timesteps * scenario * powerplant1 // p(j,t,s)
        + timesteps * scenario * powerplant2 // p(i,t,s)
        + campaigns * powerplant2 // r(i,k)
        + timesteps * scenario * powerplant2; //x(i,t,s)

    std::vector<double> objective(numCols, 0);
    std::vector<double> colLower(numCols, 0);
    std::vector<double> colUpper(numCols, DBL_MAX);

    //Set the objectives
    for(int i = 0; i < powerplant2; i++)
    {
        for(int k = 0; k < campaigns; k++)
        {
            objective[r(i,k)] = plants2[i].refueling_cost[k];
        }
    }

    for(int s = 0; s < scenario; s++)
    {
        for(size_t t = 0; t < timesteps; t++)
        {
            for(int j = 0; j < powerplant1; j++)
            {
                objective[p1(j,t,s)] = plants1[j].cost[s][t] * durations[t] / scenario;
            }
        }
        for(int i = 0; i < powerplant2; i++)
        {
            // ATTENTION : est-ce que c'est vraiment 1 ?
            objective[x(i,timesteps - 1, s)] = - plants2[i].fuel_price / scenario;
        }
    }

    //[CT2] Min and max production on Type 1 plants
    for(int j = 0; j < powerplant1; j++)
    {
        for(size_t t = 0; t < timesteps; t++)
        {
            for(int s = 0; s < scenario; s++)
            {
                colLower[p1(j,t,s)] = plants1[j].pmin[s][t];
                colUpper[p1(j,t,s)] = plants1[j].pmax[s][t];
            }
        }
    }

    //[CT8] Initial fuel stock
    for(int i = 0; i < powerplant2; i++)
    {
        for(int s = 0; s < scenario; s++)
        {
            colLower[x(i,0,s)] = plants2[i].stock;
            colUpper[x(i,0,s)] = plants2[i].stock;
        }
    }

    std::vector<int> rows;
    std::vector<int> cols;
    std::vector<double> vals;
    std::vector<double> rowLower;
    std::vector<double> rowUpper;

    rows.reserve(scenario*timesteps*(powerplant1 + powerplant2));
    cols.reserve(scenario*timesteps*(powerplant1 + powerplant2));
    vals.reserve(scenario*timesteps*(powerplant1 + powerplant2));
    rowLower.reserve(scenario*timesteps);
    rowUpper.reserve(scenario*timesteps);
    
    //[CT1] Production must meet load
    int row = 0;
    for(int s = 0; s < scenario; s++)
    {
        for(size_t t = 0; t < timesteps; t++)
        {
            for(int j = 0; j < powerplant1; j++)
            {
                rows.push_back(row);
                cols.push_back(p1(j,t,s));
                vals.push_back(1);
            }
            for(int i = 0; i < powerplant2; i++)
            {
                rows.push_back(row);
                cols.push_back(p2(i,t,s));
                vals.push_back(1);
            }
            rowLower.push_back(demand[s][t]);
            rowUpper.push_back(demand[s][t]);
            row++;
        }
    }

    CoinPackedMatrix constraints(true, &rows[0], &cols[0], &vals[0], rows.size());

    ClpSimplex modele;

    modele.loadProblem(constraints, &colLower[0], &colUpper[0], &objective[0], &rowLower[0], &rowUpper[0]);


    return 0;
}

