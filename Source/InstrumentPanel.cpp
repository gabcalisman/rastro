#include "InstrumentPanel.h"
InstrumentPanel::InstrumentPanel(RastroProcessor &processor)
    : p(processor), patches(processor), envelope(processor)
{
    addAndMakeVisible(synthTab);
    addAndMakeVisible(patchTab);
    addChildComponent(patches);
    synthTab.setButtonText("SYNTH");
    patchTab.setButtonText("PATCHES");
    synthTab.setComponentID("synthTab");
    patchTab.setComponentID("patchTab");
    synthTab.onClick = [this]
    {
        patchVisible = false;
        patches.setVisible(false);
        viewport.setVisible(true);
        repaint();
    };
    patchTab.onClick = [this]
    {
        patchVisible = true;
        viewport.setVisible(false);
        patches.setVisible(true);
        patches.update();
        repaint();
    };
    setComponentID("instrumentPanel");
    setWantsKeyboardFocus(true);
    setFocusContainerType(FocusContainerType::focusContainer);
    addAndMakeVisible(close);
    close.setComponentID("closeInstrument");
    close.onClick = [this]
    {
        setVisible(false);
        if (onClose)
            onClose();
    };
    addAndMakeVisible(viewport);
    viewport.setViewedComponent(&content, false);
    viewport.setScrollBarsShown(true, false);
    viewport.setComponentID("instrumentViewport");
    content.addAndMakeVisible(envelope);
    content.addAndMakeVisible(amplitudeTitle);
    content.addAndMakeVisible(filterTitle);
    amplitudeTitle.setFont(juce::Font(juce::FontOptions(15, juce::Font::bold)));
    filterTitle.setFont(juce::Font(juce::FontOptions(14, juce::Font::bold)));
    content.addAndMakeVisible(enableLoop);
    enableLoop.setComponentID("instrumentLoop");
    loopAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        p.params, "regionLoop", enableLoop);
    content.addAndMakeVisible(enable);
    enable.setComponentID("synthEnable");
    enabled = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        p.params, "synthEnable", enable);
    content.addAndMakeVisible(loopRegion);
    loopRegion.setComponentID("loopRegionSet");
    regionAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        p.params, "loopRegionSet", loopRegion);
    for (const char *id : {"loopStart", "loopEnd", "loopCrossfade", "ampAttack", "ampDecay",
                           "ampSustain", "ampRelease", "filterCutoff", "filterResonance",
                           "envAmount", "lfoRate", "lfoDepth", "glide", "bendRange"})
    {
        auto f = std::make_unique<SliderField>();
        f->id = id;
        content.addAndMakeVisible(f->label);
        content.addAndMakeVisible(f->slider);
        f->slider.setComponentID(id);
        f->slider.setWantsKeyboardFocus(true);
        f->slider.setSliderStyle(juce::Slider::LinearHorizontal);
        f->slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 70, 24);
        f->attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            p.params, id, f->slider);
        if (f->id == "filterCutoff")
            f->slider.setSkewFactorFromMidPoint(1500);
        if (f->id == "ampAttack" || f->id == "ampDecay" || f->id == "ampRelease" ||
            f->id == "glide")
            f->slider.setSkewFactorFromMidPoint(.25);
        f->slider.textFromValueFunction = [key = f->id](double v)
        {
            if (key == "ampSustain")
                return juce::String(v * 100, 0) + " %";
            return juce::String(v, key == "filterCutoff" ? 0 : 2) +
                   (key == "filterCutoff" || key == "lfoRate"                        ? " Hz"
                    : key.startsWith("amp") && key != "ampSustain" || key == "glide" ? " s"
                    : key == "bendRange"                                             ? " st"
                    : key == "loopCrossfade"                                         ? " ms"
                                                                                     : "");
        };
        if (f->id == "ampSustain")
            f->slider.valueFromTextFunction = [](const juce::String &t)
            { return t.getDoubleValue() * .01; };
        if (f->id.startsWith("amp"))
        {
            f->slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            f->slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 100, 24);
        }
        f->slider.updateText();
        sliders.push_back(std::move(f));
    }
    for (const char *id : {"voiceMode", "triggerMode", "envTarget", "lfoTarget", "lfoSync",
                           "lfoTiming", "lfoDivision"})
    {
        auto f = std::make_unique<MenuField>();
        f->id = id;
        content.addAndMakeVisible(f->label);
        content.addAndMakeVisible(f->menu);
        f->menu.setComponentID(id);
        // Populate before creating attachment; update() refreshes translated names.
        int count = f->id == "lfoSync" || f->id == "lfoDivision"                             ? 4
                    : f->id == "voiceMode" || f->id == "triggerMode" || f->id == "lfoTiming" ? 3
                                                                                             : 2;
        for (int i = 0; i < count; ++i)
            f->menu.addItem(juce::String(i), i + 1);
        f->attach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            p.params, id, f->menu);
        menus.push_back(std::move(f));
    }
    update();
}
void InstrumentPanel::update()
{
    if (patchVisible)
        patches.update();
    envelope.update();
    auto pal = visuals::palette(p);
    amplitudeTitle.setColour(juce::Label::textColourId, pal.text);
    filterTitle.setColour(juce::Label::textColourId, pal.text);
    auto enabledText = p.value("synthEnable") > .5f
                           ? (p.language == 0 ? "SYNTH ATIVO" : "SYNTH ON")
                           : (p.language == 0 ? "ATIVAR SYNTH" : "ENABLE SYNTH");
    enable.setButtonText(enabledText);
    for (auto &f : sliders)
    {
        f->label.setColour(juce::Label::textColourId, pal.text);
        f->slider.setColour(juce::Slider::textBoxTextColourId, pal.text);
        f->slider.setColour(juce::Slider::trackColourId, pal.control);
        bool synth = p.value("synthEnable") > .5f;
        bool amp = f->id.startsWith("amp");
        bool enabled = amp                        ? (synth || p.value("triggerMode") == 2)
                       : f->id == "glide"         ? p.value("voiceMode") > 0
                       : f->id == "bendRange"     ? true
                       : f->id.startsWith("loop") ? p.value("regionLoop") > .5f
                                                  : synth;
        if (f->id == "lfoRate" && p.value("lfoTiming") == 2)
            enabled = false;
        f->slider.setEnabled(enabled);
    }
    for (auto &f : menus)
    {
        f->menu.setEnabled(f->id == "lfoSync"       ? p.value("lfoTiming") == 0
                           : f->id == "lfoDivision" ? p.value("lfoTiming") == 2
                                                    : true);
        f->label.setColour(juce::Label::textColourId, pal.text);
        f->menu.setColour(juce::ComboBox::textColourId, pal.text);
    }
    if (lastLanguage == p.language)
        return;
    lastLanguage = p.language;
    enableLoop.setButtonText("LOOP");
    enableLoop.setTooltip(text::get("regionLoopHelp", p.language));
    close.setButtonText(p.language == 0 ? "VOLTAR" : "BACK");
    loopRegion.setButtonText(text::get("loopRegionSet", p.language));
    loopRegion.setTooltip(text::get("regionLoopHelp", p.language));
    amplitudeTitle.setText(
        p.language == 0 ? juce::String::fromUTF8("AMPLITUDE · ATTACK / DECAY / SUSTAIN / RELEASE")
                        : juce::String::fromUTF8("AMPLITUDE · ATTACK / DECAY / SUSTAIN / RELEASE"),
        juce::dontSendNotification);
    filterTitle.setText(p.language == 0
                            ? juce::String::fromUTF8("FILTRO · MODULAÇÃO · REGIÃO DO LOOP")
                            : juce::String::fromUTF8("FILTER · MODULATION · LOOP REGION"),
                        juce::dontSendNotification);
    enable.setTooltip(text::get("synthHelp", p.language));
    for (auto &f : sliders)
    {
        f->label.setText(text::get(f->id, p.language), juce::dontSendNotification);
        f->slider.setTooltip(text::get(f->id + "Help", p.language));
    }
    for (auto &f : menus)
    {
        f->label.setText(text::get(f->id, p.language), juce::dontSendNotification);
        auto selection = f->menu.getSelectedId();
        f->menu.clear(juce::dontSendNotification);
        juce::StringArray choices;
        if (f->id == "voiceMode")
            choices = {"Poly (16)", "Mono", "Legato"};
        else if (f->id == "triggerMode")
            choices = {p.language == 0 ? "Legado (por modo)" : "Legacy (by mode)", "One Shot",
                       "Gate / Hold"};
        else if (f->id == "lfoTiming")
            choices = {p.language == 0 ? "Legado" : "Legacy", "FREE", "SYNC"};
        else if (f->id == "lfoDivision")
            choices = {"1/4", "1/8", "1/8.", "1/8T"};
        else if (f->id == "lfoSync")
            choices = {p.language == 0 ? "Livre (Hz)" : "Free (Hz)", "1/4", "1/8", "1/2"};
        else
            choices = {"Cutoff", "Pitch"};
        f->menu.addItemList(choices, 1);
        f->menu.setSelectedId(selection, juce::dontSendNotification);
        f->menu.setTooltip(text::get(f->id + "Help", p.language));
    }
    repaint();
}
bool InstrumentPanel::keyPressed(const juce::KeyPress &key)
{
    if (key != juce::KeyPress::escapeKey)
        return false;
    close.triggerClick();
    return true;
}
void InstrumentPanel::resized()
{
    close.setBounds(getWidth() - 118, 14, 96, 30);
    synthTab.setBounds(270, 14, 110, 30);
    patchTab.setBounds(390, 14, 120, 30);
    patches.setBounds(16, 60, getWidth() - 32, getHeight() - 72);
    viewport.setBounds(16, 60, getWidth() - 32, getHeight() - 94);
    int available = getWidth() - 70, columns = available < 980 ? 3 : 4, width = available / columns;
    enable.setBounds(8, 0, 210, 30);
    enableLoop.setBounds(232, 0, 100, 30);
    loopRegion.setBounds(344, 0, available - 344, 30);
    int selectedMenu = 0;
    for (auto &f : menus)
        if (f->id == "voiceMode" || f->id == "triggerMode")
        {
            int x = 8 + selectedMenu++ * (available / 2);
            f->label.setBounds(x, 40, available / 2 - 16, 18);
            f->menu.setBounds(x, 60, available / 2 - 16, 30);
        }
    amplitudeTitle.setBounds(8, 106, available, 24);
    int ampWidth = int(available * .55f) / 4;
    int amp = 0;
    for (auto &f : sliders)
        if (f->id.startsWith("amp"))
        {
            int x = 8 + amp++ * ampWidth;
            f->label.setBounds(x, 138, ampWidth - 10, 20);
            f->slider.setBounds(x, 160, ampWidth - 10, 116);
        }
    envelope.setBounds(16 + ampWidth * 4, 138, available - ampWidth * 4 - 16, 138);
    filterTitle.setBounds(8, 294, available, 24);
    int row = 0;
    for (auto &f : sliders)
        if (!f->id.startsWith("amp"))
        {
            int x = 8 + row % columns * width, y = 328 + row / columns * 65;
            f->label.setBounds(x, y, width - 16, 20);
            f->slider.setBounds(x, y + 22, width - 16, 32);
            ++row;
        }
    int menuTop = 328 + ((row + columns - 1) / columns) * 65 + 12;
    row = 0;
    for (auto &f : menus)
        if (f->id != "voiceMode" && f->id != "triggerMode")
        {
            int x = 8 + row % columns * width, y = menuTop + row / columns * 60;
            f->label.setBounds(x, y, width - 16, 20);
            f->menu.setBounds(x, y + 22, width - 16, 30);
            ++row;
        }
    content.setSize(getWidth() - 52, menuTop + ((row + columns - 1) / columns) * 60 + 10);
}
void InstrumentPanel::paint(juce::Graphics &g)
{
    auto pal = visuals::palette(p);
    g.setColour(pal.bg);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 14);
    g.setColour(pal.outline);
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1), 14, 1.5f);
    g.setColour(pal.text);
    g.setFont(juce::Font(juce::FontOptions(23, juce::Font::bold)));
    g.drawText("SYNTH / ADSR", 24, 14, 230, 32, juce::Justification::left);
    g.setFont(juce::Font(juce::FontOptions(12)));
    g.setColour(pal.muted);
    if (!patchVisible)
        g.drawText(p.language == 0 ? juce::String::fromUTF8(
                                         "LOOP repete a região na velocidade do PITCH MODE. Sem "
                                         "time-stretch ou ajuste da duração ao BPM.")
                                   : "LOOP repeats its region at PITCH MODE speed. No time-stretch "
                                     "or duration sync to BPM.",
                   24, getHeight() - 29, getWidth() - 48, 22, juce::Justification::left);
}
