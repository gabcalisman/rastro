// Included after the native GUI helpers. These exercise the real editor and processor.
void rackDanceTests()
{
    RastroProcessor p;
    p.prepareToPlay(48000, 512);
    auto audio = makeAudio(false, false);
    check(p.loadData(audio.getData(), audio.getSize(), "dance-fixture.wav"), "dance audio fixture");
    p.set("start", .12f);
    p.set("end", .9f);
    p.set("playMode", 1);
    p.set("drywet", 0);
    std::unique_ptr<RastroEditor> editor(static_cast<RastroEditor *>(p.createEditor()));
    editor->addToDesktop(juce::ComponentPeer::windowIsTemporary);
    editor->setVisible(true);
    editor->getPeer()->handleFocusGain();
    auto *viewport = dynamic_cast<juce::Viewport *>(findComponent(*editor, "rackViewport"));
    check(viewport != nullptr, "active rack owns a real scroll viewport");
    for (auto size : {juce::Point<int>(1100, 840), juce::Point<int>(1000, 760),
                      juce::Point<int>(980, 700), juce::Point<int>(1680, 1100)})
    {
        editor->setSize(size.x, size.y);
        editor->synchroniseUi();
        checkUiLayout(*editor);
        for (int m = 0; m < rastro::moduleCount; ++m)
        {
            auto *card = findComponent(*editor, "rack_" + juce::String(m));
            check(card && card->getHeight() == 178, "card measured height includes footer");
            for (auto *control : card->getChildren())
                if (control->isVisible())
                    check(card->getLocalBounds().contains(control->getBounds()),
                          "every card child wholly inside card");
            auto *lock = findComponent(*editor, "jamLock_" + juce::String(m));
            auto target =
                viewport->getViewedComponent()->getLocalArea(lock, lock->getLocalBounds());
            viewport->setViewPosition(
                std::max(0, target.getRight() - viewport->getMaximumVisibleWidth()),
                std::max(0, target.getBottom() - viewport->getMaximumVisibleHeight()));
            clickUi(*editor, "jamLock_" + juce::String(m));
            check(p.value(rastro::jamLockIds[m]) == 1, "each visible JAM lock changes real state");
            clickUi(*editor, "jamLock_" + juce::String(m));
            int slot = rastro::timingSlot(m);
            if (slot >= 0)
            {
                auto *timing = findComponent(*editor, "effectTiming_" + juce::String(m));
                auto inEditor = editor->getLocalArea(timing, timing->getLocalBounds());
                check(editor->getComponentAt(inEditor.getCentre()) == timing,
                      "FREE/SYNC footer fully reachable after scrolling");
                check(editor->getLocalArea(viewport, viewport->getLocalBounds()).contains(inEditor),
                      "entire FREE/SYNC hit area in viewport");
            }
        }
        viewport->setViewPosition(0, 0);
        if (size.y == 840 || size.y == 760)
        {
            for (int m = 0; m < rastro::moduleCount; ++m)
            {
                auto *card = findComponent(*editor, "rack_" + juce::String(m));
                check(viewport->getLocalBounds().contains(
                          viewport->getLocalArea(card, card->getLocalBounds())),
                      "all six complete cards fit standard and compact presets");
            }
        }
        const char *file = size.y == 840   ? "rastro-rack-standard.png"
                           : size.y == 760 ? "rastro-rack-compact.png"
                           : size.y == 700 ? "rastro-rack-minimum.png"
                                           : "rastro-rack-large.png";
        saveUi(*editor, file);
        if (size.y == 700)
        {
            viewport->setViewPosition(2000, 2000);
            saveUi(*editor, "rastro-rack-minimum-scrolled.png");
        }
    }
    editor->setSize(1100, 840);
    p.set("avatarSpecies", 1);
    p.set("avatarInstrument", 1);
    editor->synchroniseUi();
    auto *avatar = dynamic_cast<RastroAvatar *>(findComponent(*editor, "mainAvatar"));
    check(avatar, "real main avatar");
    clickUi(*editor, "customize");
    clickUi(*editor, "visualTab4");
    editor->synchroniseUi();
    auto *dancer = dynamic_cast<RastroAvatar *>(findComponent(*editor, "customizerAvatar"));
    check(dancer && !dancer->isPreviewing(), "preview begins explicitly off");
    for (int i = 0; i < 120; ++i)
        dancer->update(2);
    check(dancer->motion().footLeft == 0 && dancer->motion().armRight == 0,
          "silence never starts a dance");
    auto soundSnapshot = [&]
    {
        std::vector<float> values;
        for (auto *parameter : p.getParameters())
            if (auto *ranged = dynamic_cast<juce::RangedAudioParameter *>(parameter))
                if (!rastro::isAppearance(ranged->paramID) && ranged->paramID != "uiCompact")
                    values.push_back(ranged->getValue());
        return values;
    };
    auto soundBefore = soundSnapshot();
    auto noteSerial = p.engine.noteSerial.load();
    for (int style = 0; style < 3; ++style)
    {
        p.set("danceStyle", (float)style);
        clickUi(*editor, "dancePreview");
        for (int i = 0; i < 28; ++i)
            editor->synchroniseUi();
        check(dancer->isPreviewing() && std::abs(dancer->motion().footLeft) > .01f,
              "each style moves without audio or MIDI");
        check(process(p, 2) == 0 && p.engine.noteSerial == noteSerial,
              "visual preview produces neither audio nor MIDI");
        saveUi(*editor, ("rastro-dance-preview-" + juce::String(style) + ".png").toRawUTF8());
        clickUi(*editor, "dancePause");
        auto held = dancer->motion();
        for (int i = 0; i < 24; ++i)
            editor->synchroniseUi();
        check(dancer->motion().phase == held.phase && dancer->motion().footLeft == held.footLeft,
              "visual pause holds exact phase and pose");
        clickUi(*editor, "dancePause");
        editor->synchroniseUi();
        check(dancer->motion().phase != held.phase, "visual resume continues phase");
        clickUi(*editor, "danceStop");
        editor->synchroniseUi();
        check(!dancer->isPreviewing() && dancer->motion().footLeft == 0,
              "visual stop returns natural pose");
    }
    clickUi(*editor, "dancePreview");
    p.set("animation", 1);
    for (int i = 0; i < 120; ++i)
        editor->synchroniseUi();
    check(dancer->motion().armLeft == 0 && dancer->motion().torso == 0,
          "reduced removes secondary choreography");
    saveUi(*editor, "rastro-dance-reduced.png");
    p.set("animation", 0);
    editor->synchroniseUi();
    check(dancer->motion().footLeft == 0 && dancer->motion().armRight == 0,
          "off resets all animation, including preview");
    saveUi(*editor, "rastro-dance-off.png");
    p.set("animation", 2);
    p.pausePlayback(true);
    editor->synchroniseUi();
    check(!dancer->isPreviewing(), "audio transport pause also cancels visual preview");
    p.pausePlayback(false);
    auto *sheet = dynamic_cast<VisualCustomizer *>(findComponent(*editor, "visualCustomizer"));
    sheet->keyPressed(juce::KeyPress(juce::KeyPress::escapeKey));
    editor->synchroniseUi();
    check(process(p, 2, 0) > 0, "real MIDI starts sound");
    bool danced = false;
    for (int i = 0; i < 30; ++i)
    {
        process(p, 2);
        editor->synchroniseUi();
        auto motion = avatar->motion();
        danced |=
            std::abs(motion.footLeft) + std::abs(motion.footRight) + std::abs(motion.armRight) >
            .05f;
    }
    check(avatar->motion().activity > 0 && danced, "real musical activity dances");
    auto *deck = dynamic_cast<SampleDeck *>(findComponent(*editor, "sampleDeck"));
    check(deck, "real sampler deck");
    auto heldDeck = deck->motion();
    clickUi(*editor, "playbackPause");
    auto held = avatar->motion();
    float position = p.engine.position;
    process(p, 2); // 64-sample de-click ramp, engine already frozen.
    check(process(p, 2) == 0 && p.engine.position == position,
          "paused audio is silent and sample position frozen");
    for (int i = 0; i < 24; ++i)
        editor->synchroniseUi();
    check(avatar->motion().phase == held.phase && avatar->motion().footLeft == held.footLeft,
          "real playback pause freezes dance immediately");
    check(deck->motion().angle == heldDeck.angle && !deck->motion().playing,
          "deck also freezes on pause and reports stopped playback");
    saveUi(*editor, "rastro-dance-playback-paused.png");
    clickUi(*editor, "playbackPause");
    check(process(p, 2) > 0 && p.engine.position != position,
          "resume advances preserved sample position");
    editor->synchroniseUi();
    clickUi(*editor, "playbackStop");
    process(p, 2);
    check(process(p, 2) == 0 && std::abs(p.engine.position - p.value("start")) < .0001f,
          "stop clears voices/tails and rewinds selection start");
    editor->synchroniseUi();
    check(avatar->motion().phase == 0 && avatar->motion().footLeft == 0,
          "stop returns natural pose");
    check(deck->motion().angle == 0, "stop resets deck position too");
    saveUi(*editor, "rastro-dance-playback-stopped.png");
    check(process(p, 2, 0) > 0, "new MIDI works after stop");
    p.stopPlayback();
    process(p, 2);
    clickUi(*editor, "samplePreview");
    check(process(p, 2) > 0, "real sample playback starts after stop");
    editor->synchroniseUi();
    clickUi(*editor, "playbackStop");
    process(p, 2);
    clickUi(*editor, "customize");
    clickUi(*editor, "visualTab1");
    for (int design = 0; design < 10; ++design)
    {
        clickUi(*editor, "knobChoice" + juce::String(design));
        editor->synchroniseUi();
        check(visuals::knobStyle(p) == design,
              "each hardware choice drives the actual look and feel");
        checkUiLayout(*editor);
        saveUi(*editor, ("rastro-knob-customizer-" + juce::String(design) + ".png").toRawUTF8());
        clickUi(*editor, "closeCustomizer");
        editor->synchroniseUi();
        saveUi(*editor, ("rastro-knob-applied-" + juce::String(design) + ".png").toRawUTF8());
        clickUi(*editor, "customize");
    }
    clickUi(*editor, "closeCustomizer");
    juce::MemoryBlock saved;
    p.getStateInformation(saved);
    RastroProcessor restored;
    restored.setStateInformation(saved.getData(), int(saved.getSize()));
    check(visuals::knobStyle(restored) == 9, "new hardware survives session restore");
    auto legacy = p.params.copyState();
    legacy.removeChild(legacy.getChildWithProperty("id", "knobEdition"), nullptr);
    juce::MemoryBlock oldState;
    juce::AudioProcessor::copyXmlToBinary(*legacy.createXml(), oldState);
    restored.setStateInformation(oldState.getData(), int(oldState.getSize()));
    check(restored.value("knobEdition") == 0 &&
              restored.params.getParameter("knobDesign")->getNormalisableRange().end == 4,
          "old sessions reset new override; old automation normalization is preserved");
    // Preview and customization preserve the complete set of sound parameter values.
    auto soundAfter = soundSnapshot();
    check(soundBefore == soundAfter, "dance and transport UI preserve every sound parameter");
}
