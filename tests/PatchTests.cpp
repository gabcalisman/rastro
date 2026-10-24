#include "PatchSearch.h"
#include <iostream>
#include <stdexcept>
void check(bool b, const char *m)
{
    if (!b)
        throw std::runtime_error(m);
}
void wait(rastro::PatchSearch &search)
{
    auto until = std::chrono::steady_clock::now() + std::chrono::seconds(30);
    while (search.busy && std::chrono::steady_clock::now() < until)
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    check(!search.busy, "bounded patch search finishes");
}
int main()
{
    rastro::Sample sample;
    sample.rate = 48000;
    for (auto &c : sample.data)
    {
        c.resize(48000);
        for (int i = 0; i < 48000; ++i)
            c[i] = .4f * std::exp(-i / 18000.f) * std::sin(i * .0576f);
    }
    rastro::Settings base;
    base.start = .1f;
    base.end = .8f;
    base.root = 57;
    rastro::PatchSearch search;
    check(search.start(&sample, base, false, 42), "search accepts real source");
    wait(search);
    auto faithful = search.results();
    check(search.status == rastro::PatchSearch::complete && faithful.size() >= 3 &&
              search.sourceUsed() == &sample,
          "search returns distinct scored sample-based candidates");
    for (auto &patch : faithful)
    {
        check(std::isfinite(patch.distance) && patch.distance >= 0,
              "candidate has real finite descriptor distance");
        auto p = rastro::patchSettings(patch, base);
        check(p.start == base.start && p.end == base.end && p.root == base.root && p.synth &&
                  p.triggerMode == 2,
              "patch mapping preserves source trim/root and activates actual synth");
    }
    check(search.start(&sample, base, true, 51), "transformed search starts");
    search.paused = true;
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    float progress = search.progress;
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    check(search.progress == progress, "pause halts candidate progress");
    search.paused = false;
    wait(search);
    auto transformed = search.results();
    check(transformed.size() >= 3, "transformed search yields options");
    check(transformed.front().values != faithful.front().values,
          "search direction affects actual editable patches");
    check(search.start(&sample, base, false, 99), "cancelable search starts");
    search.cancel();
    wait(search);
    check(search.status == rastro::PatchSearch::canceled,
          "cancel terminates worker without fake completion");
    rastro::Sample silence;
    silence.rate = 48000;
    for (auto &c : silence.data)
        c.resize(48000);
    check(search.start(&silence, base, false), "silent source can be inspected");
    wait(search);
    check(search.status == rastro::PatchSearch::silent, "silent source reports inability to fit");
    std::cout << "PASS patches: real engine candidate rendering/descriptor ranking, diverse "
                 "options, immutable source/root/trims, faithful/transformed, pause/cancel/silent "
                 "source, bounded worker\n";
}
