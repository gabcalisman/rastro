// Included after the shared GUI/fixture helpers in ProcessorTests.cpp.
void appearanceWorkshopTests()
{
    RastroProcessor p;
    p.prepareToPlay(48000, 512);
    auto audio = makeAudio(false, false);
    check(p.loadData(audio.getData(), audio.getSize(), "workshop.wav"), "workshop source");
    p.set("ampAttack", .23f);
    p.set("root", 55);
    p.set("pitchMode", 1);
    auto source = p.source.load();
    auto wait = [&](auto action)
    {
        bool done = false, ok = false;
        action(
            [&](bool result)
            {
                done = true;
                ok = result;
            });
        for (int i = 0; i < 500 && !done; ++i)
            pumpUi(5);
        check(done, "worker finishes appearance/patch request");
        return ok;
    };
    std::array<float, std::size(rastro::appearanceIds)> chosen{};
    for (size_t i = 0; i < chosen.size(); ++i)
    {
        auto id = rastro::appearanceIds[i];
        auto *param = p.params.getParameter(id);
        chosen[i] = param->getNormalisableRange().end;
        p.set(id, chosen[i]);
    }
    for (int scope = 0; scope <= 4; ++scope)
    {
        juce::TemporaryFile file(".rastro-look");
        for (size_t i = 0; i < chosen.size(); ++i)
            p.set(rastro::appearanceIds[i], chosen[i]);
        check(wait([&](auto done) { p.appearanceFileAsync(file.getFile(), true, scope, done); }),
              "save bounded visual preset");
        for (auto id : rastro::appearanceIds)
            p.set(id, 0);
        check(wait([&](auto done) { p.appearanceFileAsync(file.getFile(), false, 0, done); }),
              "load bounded visual preset");
        for (size_t i = 0; i < chosen.size(); ++i)
            check(p.value(rastro::appearanceIds[i]) ==
                      (scope == 0 || rastro::appearanceGroup(rastro::appearanceIds[i]) == scope
                           ? chosen[i]
                           : 0),
                  "preset replaces only its declared visual groups");
        check(p.source.load() == source && p.value("root") == 55 &&
                  std::abs(p.value("ampAttack") - .23f) < 1e-6,
              "visual files preserve sound and sample");
        auto xml = juce::XmlDocument::parse(file.getFile());
        auto *bad = xml->createNewChildElement("PARAM");
        bad->setAttribute("id", "root");
        bad->setAttribute("value", 90);
        file.getFile().replaceWithText(xml->toString());
        auto previous = p.value("sceneKind");
        check(!wait([&](auto done) { p.appearanceFileAsync(file.getFile(), false, 0, done); }),
              "visual file cannot inject sound parameters");
        check(p.value("root") == 55 && p.value("sceneKind") == previous,
              "invalid visual file is transactional");
    }
    juce::TemporaryFile patch(".rastro");
    p.set("sceneKind", 2);
    p.set("avatarSpecies", 2);
    p.set("danceStyle", 2);
    p.language = 1;
    p.cc[0] = 48;
    check(wait([&](auto done) { p.patchFileAsync(patch.getFile(), true, done); }),
          "save full sound and look");
    p.set("sceneKind", 3);
    p.set("avatarSpecies", 1);
    p.set("danceStyle", 1);
    p.set("root", 60);
    p.set("ampAttack", .01f);
    p.set("uiCompact", 1);
    p.language = 0;
    p.cc[0] = 73;
    check(wait([&](auto done) { p.patchFileAsync(patch.getFile(), false, done, true); }),
          "load sound-only patch");
    check(p.value("root") == 55 && std::abs(p.value("ampAttack") - .23f) < 1e-6,
          "sound-only actually restores sound");
    check(p.value("sceneKind") == 3 && p.value("avatarSpecies") == 1 &&
              p.value("danceStyle") == 1 && p.value("uiCompact") == 1 && p.language == 0 &&
              p.cc[0] == 73,
          "sound-only preserves look and preferences");
    check(p.source.load()->data == source->data, "sound-only patch retains embedded PCM exactly");
    check(wait([&](auto done) { p.patchFileAsync(patch.getFile(), false, done, false); }),
          "load full patch");
    check(p.value("sceneKind") == 2 && p.value("avatarSpecies") == 2 &&
              p.value("danceStyle") == 2 && p.language == 1 && p.cc[0] == 48,
          "full patch explicitly restores sound and appearance");
    for (auto id : rastro::jamLockIds)
        p.set(id, 0);
    p.set("gJamLock", 1);
    p.set("mutation", 1);
    auto ghost = p.value("gFeedback"), crush = p.value("kAmount");
    p.mutate();
    check(p.value("gFeedback") == ghost && p.value("kAmount") != crush,
          "JAM respects module locks and varies active unlocked modules");
    p.undoMutation();
    check(p.value("gFeedback") == ghost && p.value("kAmount") == crush,
          "JAM exact undo preserves locked controls");
    juce::MemoryBlock state;
    p.getStateInformation(state);
    auto xml = juce::AudioProcessor::getXmlFromBinary(state.getData(), int(state.getSize()));
    auto tree = juce::ValueTree::fromXml(*xml);
    for (auto id : {"avatarSpecies", "avatarMaterial", "sceneKind", "sceneColour", "sceneLight",
                    "sceneDecor", "sceneReaction", "danceStyle", "danceAmount"})
        tree.removeChild(tree.getChildWithProperty("id", id), nullptr);
    tree.setProperty("stateVersion", 4, nullptr);
    juce::AudioProcessor::copyXmlToBinary(*tree.createXml(), state);
    p.setStateInformation(state.getData(), int(state.getSize()));
    check(p.value("sceneKind") == 0 && p.value("avatarSpecies") == 0 && p.value("danceAmount") == 1,
          "v4 sessions keep original stage and character rig");
    p.language = 0;
    p.set("uiCompact", 0);
    std::unique_ptr<RastroEditor> editor(new RastroEditor(p));
    editor->addToDesktop(0);
    editor->setVisible(true);
    editor->synchroniseUi();
    auto *room = findComponent(*editor, "mainAvatar");
    auto *envelope = findComponent(*editor, "mainEnvelope");
    check(room && room->getHeight() >= 280 && envelope && envelope->isShowing(),
          "room has a usable main-stage area and real ADSR readout");
    clickUi(*editor, "customize");
    auto *sheet = dynamic_cast<VisualCustomizer *>(findComponent(*editor, "visualCustomizer"));
    for (int tab : {3, 4, 5})
    {
        clickUi(*editor, "visualTab" + juce::String(tab));
        editor->synchroniseUi();
        checkUiLayout(*editor);
        const char *id = tab == 3 ? "sceneKind" : tab == 4 ? "danceStyle" : "gColour";
        auto *combo = dynamic_cast<juce::ComboBox *>(findComponent(*editor, id));
        auto before = p.value(id);
        combo->setSelectedId(before == 2 ? 2 : 3, juce::sendNotificationSync);
        auto edited = p.value(id);
        check(edited != before, "visible workshop selector changes real state");
        clickUi(*editor, "undoVisuals");
        check(p.value(id) == before, "workshop undo exact");
        clickUi(*editor, "redoVisuals");
        check(p.value(id) == edited, "workshop redo exact");
        editor->synchroniseUi();
        saveUi(*editor, tab == 3   ? "rastro-room-customizer.png"
                        : tab == 4 ? "rastro-dance-customizer.png"
                                   : "rastro-rack-customizer.png");
    }
    sheet->keyPressed(juce::KeyPress(juce::KeyPress::escapeKey));
    p.set("avatarSpecies", 1);
    p.set("avatarColour", 2);
    p.set("avatarHair", 1);
    p.set("avatarAccessory", 1);
    p.set("avatarInstrument", 1);
    for (int kind = 0; kind < 4; ++kind)
    {
        p.set("sceneKind", float(kind));
        p.set("sceneDecor", float(kind));
        p.set("colourSet", kind == 3 ? 4 : 1);
        p.set("sceneLight", kind == 3 ? 2 : 1);
        editor->synchroniseUi();
        saveUi(*editor, ("rastro-room-" + juce::String(kind) + ".png").toRawUTF8());
    }
    p.set("playMode", 4);
    editor->synchroniseUi();
    checkUiLayout(*editor);
    saveUi(*editor, "rastro-slice-room.png");
    p.set("playMode", 0);
    editor->synchroniseUi();
    RastroAvatar portrait(p);
    for (int species : {1, 2})
        for (int presentation = 0; presentation < 3; ++presentation)
        {
            p.set("avatarSpecies", float(species));
            p.set("avatarPresentation", float(presentation));
            p.set("colourSet", presentation == 1 ? 2 : 1);
            p.set("sceneKind", 2);
            portrait.setSize(380, 340);
            portrait.update(0);
            saveUi(portrait, ("rastro-creature-" + juce::String(species) + "-" +
                              juce::String(presentation) + ".png")
                                 .toRawUTF8());
            portrait.setSize(160, 144);
            check(portrait.createComponentSnapshot(portrait.getLocalBounds()).isValid(),
                  "each creature/presentation renders at small scale");
        }
    auto *mainEnvelope = dynamic_cast<EnvelopeView *>(envelope);
    check(mainEnvelope->keyPressed(juce::KeyPress(juce::KeyPress::returnKey)),
          "main ADSR supports keyboard opening");
    check(findComponent(*editor, "synthEnable")->isShowing(),
          "main ADSR opens real synth controls");
    clickUi(*editor, "closeInstrument");
    // Every appearance parameter differs, while all audio settings and PCM remain identical.
    RastroProcessor a, b;
    a.prepareToPlay(48000, 512);
    b.prepareToPlay(48000, 512);
    a.loadData(audio.getData(), audio.getSize(), "same.wav");
    b.loadData(audio.getData(), audio.getSize(), "same.wav");
    for (auto id : rastro::appearanceIds)
    {
        a.set(id, 0);
        b.set(id, b.params.getParameter(id)->getNormalisableRange().end);
    }
    juce::AudioBuffer<float> left(2, 512), right(2, 512);
    juce::MidiBuffer m, n;
    m.addEvent(juce::MidiMessage::noteOn(1, 60, .8f), 0);
    n.addEvent(juce::MidiMessage::noteOn(1, 60, .8f), 0);
    a.processBlock(left, m);
    b.processBlock(right, n);
    for (int c = 0; c < 2; ++c)
        for (int i = 0; i < 512; ++i)
            check(left.getSample(c, i) == right.getSample(c, i),
                  "all visual and room variations leave sound bit identical");
    editor->removeFromDesktop();
    std::cout << "PASS workshop: scoped visual files, transactional errors, sound-only/full "
                 "patches, JAM locks, v4 migration, undo/redo, scenes and bit-identical audio\n";
}
