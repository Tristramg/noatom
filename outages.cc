#include "outages.h"

Outages::Outages(const Instance & data) : data(data)
{
    // Par défaut on va dire que les campagnes sont équitablement réparties
    campaign_start.resize(data.powerplant2);
    decoupling.resize(data.powerplant2);
    int campaign_duration = data.weeks / data.campaigns;
    for(int i = 0; i < data.powerplant2; i++)
    {
        campaign_start[i].push_back(0);
        decoupling[i].push_back(10);
        for(int k = 1; k < data.campaigns ; k++)
        {
            campaign_start[i].push_back(campaign_start[i].back() + campaign_duration); 
            //Par défaut découplage la 1e semaine
            decoupling[i].push_back(10);
         }
   }
}

// Outage timesteps
std::vector<size_t> Outages::ea(int i, int k) const
{
    size_t steps_per_week = data.timesteps / data.weeks;
    std::vector<size_t> ret;
    if(decoupling[i][k] >= 0)
    {
        size_t start = (campaign_start[i][k] + decoupling[i][k]) * steps_per_week;
        size_t end = start + data.plants2[i].durations.at(k) * steps_per_week;
        //Attention ! Vérifier la borne sup
        for(size_t t = start; t < end; t++)
        {
            ret.push_back(t);
        }
    }

    return ret;
}

// Production timesteps
std::vector<size_t> Outages::ec(int i, int k) const
{
    int steps_per_week = data.timesteps / data.weeks;
    std::vector<size_t> ret;
    size_t start = campaign_start[i][k] * steps_per_week;
    size_t end;
    if(k >= data.campaigns - 1)
        end = data.timesteps - 1;
    else
        end = campaign_start[i][k+1] * steps_per_week;
    size_t dec_start = (campaign_start[i][k] + decoupling[i][k]) * steps_per_week;
    size_t dec_end = dec_start + data.plants2[i].durations.at(k) * steps_per_week - 1;

    for(size_t t = start; t < end; t++)
    {
        //Attention ! Vérifier la borne sup
        if( t < dec_start || t > dec_end)
            ret.push_back(t);
    }

    return ret;
}



// Week of decoupling during a campaign. -1 if no decoupling happens
int Outages::ha(int i, int k) const
{
    return decoupling[i][k];
}

std::ostream & operator<<(std::ostream & os, const Outages & o)
{
    for(int i = 0; i < o.data.powerplant2; i++)
    {
        os << "Plant " << i << std::endl;
        os << "    Campaigns ";
        for(size_t t = 0; t < o.campaign_start[i].size(); t++)
        {
            os << o.campaign_start[i][t] << " (" << o.decoupling[i][t] << ")  ";
        }
        os << std::endl;
    }
    os << std::endl;
    return os;
}

