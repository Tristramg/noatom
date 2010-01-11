#include<vector>
#include<string>

#ifndef INSTANCE_H
#define INSTANCE_H

struct file_problem{};

struct Powerplant_t1
{
    std::string name;
    int index;
    std::vector<std::vector<float> > pmin;
    std::vector<std::vector<float> > pmax;
    std::vector<std::vector<float> > cost;

    Powerplant_t1(size_t scenarios, size_t timesteps) :
        pmin(scenarios),
        pmax(scenarios),
        cost(scenarios)
    {
        for(size_t i=0; i < scenarios; i++)
        {
            pmin[i].reserve(timesteps);
            pmax[i].reserve(timesteps);
            cost[i].reserve(timesteps);
        }
    }
};

struct Powerplant_t2
{
    std::string name;
    int index;
    float stock;
    int campaigns;
    std::vector<int> durations;
    int current_max_modulus;
    std::vector<int> max_modulus;
    std::vector<int> max_refuel;
    std::vector<int> min_refuel;
    std::vector<int> refuel_ratio;
    int current_stock_threshold;
    std::vector<int> stock_threshold;
    std::vector<float> pmax;
    std::vector<float> max_stock_before_refueling;
    std::vector<float> max_stock_after_refueling;
    std::vector<float> refueling_cost;
    float fuel_price;
    std::vector<int> current_decrease_profile_idx;
    std::vector<float> current_decrease_profile_val;
    std::vector<std::vector<int> > decrease_profile_idx;
    std::vector<std::vector<float> > decrease_profile_val;
};

struct Constraint_13
{
    int powerplant_idx;
    int campaign_idx;
    int earliest_stop_time;
    int latest_stop_time;
};

struct Constraint_14
{
    std::vector<int> set;
    int spacing;
};

struct Constraint_15
{
    std::vector<int> set;
    int spacing;
    int first_week;
    int last_week;
};

struct Constraint_16
{
    std::vector<int> set;
    int spacing;
};

struct Constraint_17
{
    std::vector<int> set;
    int spacing;
};

struct Constraint_18
{
    std::vector<int> set;
    int spacing;
};

struct Constraint_19_period
{
    int powerplant;
    std::vector<int> start;
    std::vector<int> duration;
};

struct Constraint_19
{
    std::vector<int> set;
    int quantity;
    std::vector<Constraint_19_period> periods;
};

struct Constraint_20
{
    std::vector<int> set;
    int week;
    int max;
};

struct Constraint_21
{
    std::vector<int> set;
    int start;
    int end;
    float max;
};

struct Instance
{
    static Instance * data;

    static Instance * get();
    static void build(const std::string &);
    static void destroy();
    

    size_t timesteps;
    std::string dataset;
    int weeks;
    int campaigns;
    int scenario;
    float epsilon;
    int powerplant1;
    int powerplant2;
    int constraint13;
    int constraint14;
    int constraint15;
    int constraint16;
    int constraint17;
    int constraint18;
    int constraint19;
    int constraint20;
    int constraint21;
    std::vector<int> durations;
    std::vector<std::vector<float> > demand;

    std::vector<Powerplant_t1> plants1;
    std::vector<Powerplant_t2> plants2;

    std::vector<Constraint_13> ct13;
    std::vector<Constraint_14> ct14;
    std::vector<Constraint_15> ct15;
    std::vector<Constraint_16> ct16;
    std::vector<Constraint_17> ct17;
    std::vector<Constraint_18> ct18;
    std::vector<Constraint_19> ct19;
    std::vector<Constraint_20> ct20;
    std::vector<Constraint_21> ct21;
    private:
    Instance(const std::string & filename);
};

#endif
