#include "instance.h"

#include <iostream>
#ifndef OUTAGES_H
#define OUTAGES_H

//Brainstorm
// Il faut pour chaque centrale définir quand aura lieu la campagne (en gros trouver 5 instants de début)
// À l'itérieur de chaque campagne, il faut définir qd aura lieu le outage
class Outages
{
    public:
    // Timestep of the campaign start for every plant
    std::vector<std::vector<int> > campaign_start;
    std::vector<std::vector<int> > decoupling;
    const Instance & data;
    Outages(const Instance &);
    std::vector<size_t> ea(int i, int k) const;
    std::vector<size_t> ec(int i, int k) const;
    int ha(int i, int k) const;

};

std::ostream & operator<<(std::ostream & os, const Outages & o);
#endif
