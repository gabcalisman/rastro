#pragma once
#include <array>
namespace rastro
{
inline constexpr int moduleCount = 6;
inline constexpr const char *moduleNames[]{"GHOST", "CRUMBLE", "BLOOM", "WOBBLE", "DUST", "CRUSH"};
inline constexpr const char *moduleDescriptions[]{"ghost",  "crumble", "bloom",
                                                  "wobble", "dust",    "crush"};
inline constexpr const char *onIds[]{"gOn", "cOn", "bOn", "wOn", "dOn", "kOn"};
inline constexpr const char *presentIds[]{"gPresent", "cPresent", "bPresent",
                                          "wPresent", "dPresent", "kPresent"};
inline constexpr const char *controlIds[6][4]{
    {"gTime", "gFeedback", "gTone", "gMix"},   {"cRate", "cVariation", "cMix", nullptr},
    {"bSize", "bTone", "bDecay", "bMix"},      {"wDepth", "wRate", "wMix", nullptr},
    {"dLevel", "dCharacter", "dMix", nullptr}, {"kAmount", "kMix", nullptr, nullptr}};
inline constexpr const char *mixIds[]{"gMix", "cMix", "bMix", "wMix", "dMix", "kMix"};
inline constexpr const char *macroIds[]{"presence", "trail", "mutation", "playX", "playY"};
using Order = std::array<int, moduleCount>;
inline constexpr Order defaultOrder{{0, 1, 2, 3, 4, 5}};
// Factoradic permutations provide six arbitrary slots in one automatable parameter.
inline Order decodeOrder(int rank)
{
    Order result{}, available = defaultOrder;
    rank = rank < 0 ? 0 : (rank > 719 ? 719 : rank);
    int left = moduleCount;
    for (int i = 0; i < moduleCount; ++i)
    {
        int f = 1;
        for (int n = 2; n < left; ++n)
            f *= n;
        int choice = rank / f;
        rank %= f;
        result[i] = available[choice];
        for (int n = choice; n < left - 1; ++n)
            available[n] = available[n + 1];
        --left;
    }
    return result;
}
inline int encodeOrder(Order order)
{
    int rank = 0;
    for (int i = 0; i < moduleCount; ++i)
    {
        int lower = 0, f = 1;
        for (int j = i + 1; j < moduleCount; ++j)
            if (order[j] < order[i])
                ++lower;
        for (int n = 2; n < moduleCount - i; ++n)
            f *= n;
        rank += lower * f;
    }
    return rank;
}
} // namespace rastro
