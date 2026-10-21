#pragma once
#include "Rack.h"
#include <array>
#include <vector>
namespace rastro
{
struct Sample
{
    std::array<std::vector<float>, 2> data;
    std::array<float, 512> waveform{};
    double rate = 44100;
    int size() const { return (int)data[0].size(); }
};
struct Settings
{
    float root = 60, start = 0, end = 1, loop = 0, level = .7f, velocity = .5f, tempo = 120,
          presence = .5f, trail = .5f, mutation = .3f, drywet = 1;
    std::array<float, 4> ghost{{.5f, .35f, .6f, .3f}}, bloom{{.6f, .6f, .5f, .25f}};
    std::array<float, 3> crumble{{.5f, .3f, .2f}};
    std::array<float, 3> wobble{{.25f, .3f, .5f}}, dust{{.15f, .5f, .5f}};
    std::array<float, 2> crush{{.3f, .3f}};
    float playX = .5f, playY = .5f;
    std::array<bool, moduleCount> enabled{{true, true, true, true, true, true}};
    Order order = defaultOrder;
    int mode = 0, sliceCount = 8;
    std::array<float, 17> cuts{
        {0, .125f, .25f, .375f, .5f, .625f, .75f, .875f, 1, 1, 1, 1, 1, 1, 1, 1, 1}};
};
} // namespace rastro
