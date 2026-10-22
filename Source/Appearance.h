#pragma once
#include <juce_core/juce_core.h>
namespace rastro
{
// This registry is shared by preset isolation, visual undo and sound-only patch loading.
// No member is read by the audio engine. Existing host parameter indices remain untouched.
inline constexpr const char *appearanceIds[]{
    "visualTheme",    "visualPalette",    "controlStyle",
    "avatarBody",     "avatarEyes",       "avatarAccessory",
    "knobDesign",     "visualBackground", "visualAccent",
    "visualControls", "avatarColour",     "avatarHair",
    "avatarHorns",    "avatarClothes",    "avatarGlasses",
    "avatarTail",     "avatarWings",      "avatarInstrument",
    "colourSet",      "cosmicArcade",     "avatarPresentation",
    "avatarHalo",     "avatarSpecies",    "avatarMaterial",
    "sceneKind",      "sceneColour",      "sceneLight",
    "sceneDecor",     "sceneReaction",    "animation",
    "danceStyle",     "danceAmount",      "gColour",
    "cColour",        "bColour",          "wColour",
    "dColour",        "kColour",          "gJamLock",
    "cJamLock",       "bJamLock",         "wJamLock",
    "dJamLock",       "kJamLock"};
inline constexpr const char *rackColourIds[]{"gColour", "cColour", "bColour",
                                             "wColour", "dColour", "kColour"};
inline constexpr const char *jamLockIds[]{"gJamLock", "cJamLock", "bJamLock",
                                          "wJamLock", "dJamLock", "kJamLock"};
inline int appearanceGroup(const juce::String &id)
{
    if (id.startsWith("avatar"))
        return 1;
    if (id.startsWith("scene"))
        return 2;
    if (id == "animation" || id.startsWith("dance"))
        return 3;
    for (auto key : rackColourIds)
        if (id == key)
            return 4;
    if (id.endsWith("JamLock"))
        return 4;
    return 0;
}
inline bool isAppearance(const juce::String &id)
{
    for (auto key : appearanceIds)
        if (id == key)
            return true;
    return false;
}
} // namespace rastro
