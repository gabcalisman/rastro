#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
namespace visuals
{
inline juce::Colour accent(int palette)
{
    constexpr juce::uint32 colours[]{0xffdf682f, 0xff68b8ab, 0xffd89aba, 0xffced968};
    return juce::Colour(colours[juce::jlimit(0, 3, palette)]);
}
inline juce::Colour background(int theme)
{
    constexpr juce::uint32 colours[]{0xff261e19, 0xff182d39, 0xff30213f, 0xff24372b};
    return juce::Colour(colours[juce::jlimit(0, 3, theme)]);
}
} // namespace visuals
