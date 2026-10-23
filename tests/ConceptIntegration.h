// Real editor + original vector collection, not a gallery of the source concept images.
void conceptCollectionTests()
{
    RastroProcessor p;
    p.prepareToPlay(48000, 512);
    auto bytes = makeAudio(false, false);
    check(p.loadData(bytes.getData(), bytes.getSize(), "concept-fixture.wav"), "concept sample");
    auto source = p.source.load();
    std::unique_ptr<RastroEditor> editor(static_cast<RastroEditor *>(p.createEditor()));
    editor->addToDesktop(juce::ComponentPeer::windowIsTemporary);
    editor->setVisible(true);
    editor->getPeer()->handleFocusGain();
    editor->setSize(1100, 840);
    editor->synchroniseUi();
    clickUi(*editor, "customize");
    clickUi(*editor, "visualTab2");
    editor->synchroniseUi();
    auto *viewport = dynamic_cast<juce::Viewport *>(findComponent(*editor, "characterViewport"));
    auto *avatar = dynamic_cast<RastroAvatar *>(findComponent(*editor, "customizerAvatar"));
    check(viewport && avatar, "real collection viewport and live avatar");
    auto choose = [&](const char *id, int value)
    {
        auto *combo = dynamic_cast<juce::ComboBox *>(findComponent(*editor, id));
        check(combo && combo->isShowing(), "equipment choice is present in actual UI");
        auto target = viewport->getViewedComponent()->getLocalArea(combo, combo->getLocalBounds());
        viewport->setViewPosition(
            0, std::max(0, target.getBottom() - viewport->getMaximumVisibleHeight()));
        auto hit = editor->getComponentAt(
            editor->getLocalArea(combo, combo->getLocalBounds()).getCentre());
        check(hit == combo || combo->isParentOf(hit),
              "equipment selector hit area unobscured after scrolling");
        combo->setSelectedId(value + 1, juce::sendNotificationSync);
        editor->synchroniseUi();
    };
    choose("stageHeadwear", 1);
    choose("avatarHair", 1);
    check(p.value("stageHeadwear") == 0, "legacy hair replaces headwear, one head slot");
    choose("stageHeadwear", 2);
    check(p.value("avatarHair") == 0 && p.value("avatarHorns") == 0,
          "hat replaces legacy head items");
    for (int model = 0; model < 7; ++model)
    {
        choose("avatarSpecies", model);
        check((model < 3 && p.value("creatureModel") == 0) ||
                  (model >= 3 && p.value("creatureModel") == model - 2),
              "model selection uses append-only compatibility mapping");
        for (auto id :
             {"stageHeadwear", "stageShirt", "stageJacket", "stagePants", "stageShoes",
              "stageHeadphones", "stagePatch", "stageLanyard", "stageJewelry", "stageEarring",
              "instrumentEdition", "glassesEdition", "avatarAccessory", "avatarGlasses",
              "avatarInstrument", "avatarHalo", "avatarTail", "avatarWings"})
            p.set(id, 0);
        editor->synchroniseUi();
        auto base = avatar->createComponentSnapshot(avatar->getLocalBounds());
        for (auto id : {"stageHeadwear", "stageShirt", "stageJacket", "stagePants", "stageShoes",
                        "stageHeadphones", "stagePatch", "stageLanyard", "stageJewelry",
                        "stageEarring", "instrumentEdition", "glassesEdition"})
        {
            p.set(id, 1);
            editor->synchroniseUi();
            check(imageDiffers(base, avatar->createComponentSnapshot(avatar->getLocalBounds())),
                  "every independent item visibly equips on each creature");
            p.set(id, 0);
            editor->synchroniseUi();
            check(!imageDiffers(base, avatar->createComponentSnapshot(avatar->getLocalBounds())),
                  "each item removes independently, restoring actual base");
        }
        if (model >= 3)
        {
            viewport->setViewPosition(0, 0);
            saveUi(*editor,
                   ("rastro-concept-base-" + juce::String(model - 2) + ".png").toRawUTF8());
        }
        for (int collection = 1; collection <= 4; ++collection)
        {
            p.set("stageHeadwear", collection == 1   ? 1
                                   : collection == 2 ? 2
                                   : collection == 3 ? 3
                                                     : 0);
            p.set("stageShirt", collection == 1   ? 0
                                : collection == 2 ? 1
                                : collection == 3 ? 2
                                                  : 3);
            p.set("stageJacket", collection == 1   ? 1
                                 : collection == 2 ? 2
                                 : collection == 3 ? 3
                                                   : 4);
            p.set("stagePants", collection);
            p.set("stageShoes", collection);
            p.set("stageHeadphones", collection == 1 ? 1 : 0);
            p.set("stagePatch", collection == 1 ? 1 : 5);
            p.set("stageLanyard", collection == 3 ? 1 : 0);
            p.set("stageJewelry", collection == 2 ? 1 : 0);
            p.set("stageEarring", collection >= 3 ? 1 : 0);
            p.set("instrumentEdition", collection);
            p.set("glassesEdition", collection == 2 ? 1 : 0);
            editor->synchroniseUi();
            checkUiLayout(*editor);
            if (model >= 3 && collection == model - 2)
            {
                viewport->setViewPosition(0, 0);
                saveUi(*editor,
                       ("rastro-concept-outfit-" + juce::String(collection) + ".png").toRawUTF8());
                clickUi(*editor, "visualTab4");
                clickUi(*editor, "dancePreview");
                for (int frame = 0; frame < 3; ++frame)
                {
                    for (int tick = 0; tick < 8; ++tick)
                        editor->synchroniseUi();
                    saveUi(*editor, ("rastro-concept-dance-" + juce::String(collection) + "-" +
                                     juce::String(frame) + ".png")
                                        .toRawUTF8());
                }
                clickUi(*editor, "visualTab2");
                editor->synchroniseUi();
            }
        }
    }
    choose("avatarInstrument", 0);
    check(p.value("instrumentEdition") == 0 && p.value("avatarInstrument") == 0,
          "none removes even a new instrument");
    choose("avatarGlasses", 0);
    check(p.value("glassesEdition") == 0 && p.value("avatarGlasses") == 0,
          "none removes new eyewear");
    for (auto size : {juce::Point<int>(1000, 760), juce::Point<int>(980, 700)})
    {
        editor->setSize(size.x, size.y);
        editor->synchroniseUi();
        checkUiLayout(*editor);
        choose("stageEarring", 1);
        checkUiLayout(*editor);
        saveUi(*editor,
               size.y == 760 ? "rastro-concept-compact.png" : "rastro-concept-minimum.png");
    }
    juce::MemoryBlock state;
    p.getStateInformation(state);
    RastroProcessor q;
    q.prepareToPlay(48000, 512);
    q.setStateInformation(state.getData(), int(state.getSize()));
    for (auto id : rastro::appearanceIds)
        check(q.value(id) == p.value(id), "all collection slots survive session recall");
    check(p.source.load() == source, "collection leaves sample ownership untouched");
    check(p.params.getParameter("avatarSpecies")->getNormalisableRange().end == 2 &&
              p.params.getParameter("avatarInstrument")->getNormalisableRange().end == 2 &&
              p.params.getParameter("avatarGlasses")->getNormalisableRange().end == 2,
          "legacy visual automation normalization retained");
}
