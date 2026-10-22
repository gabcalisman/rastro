#include "Editor.h"
#include <ctime>
#include <iostream>
// Optional local measurement harness; runs the actual JUCE message loop and native window.
class UiBenchmark : private juce::Timer
{
    RastroProcessor p;
    RastroEditor *editor = nullptr;
    juce::DocumentWindow window{"RASTRO UI benchmark", juce::Colours::black, 0};
    int phase = 0, frames = 0;
    std::clock_t startCpu{};
    double startWall = 0;
#ifdef RASTRO_UI_METRICS
    RastroEditor::UiMetrics initial{};
#endif
    void begin()
    {
        frames = 0;
        p.set("animation", phase == 0 ? 0.f : 2.f);
        if (phase == 2)
            for (auto *c : editor->getChildren())
                if (auto *b = dynamic_cast<juce::TextButton *>(c);
                    b && b->getComponentID() == "customize")
                    b->onClick();
        if (phase == 3)
            if (auto *sheet = editor->findChildWithID("visualCustomizer"))
                if (auto *b =
                        dynamic_cast<juce::TextButton *>(sheet->findChildWithID("visualTab2")))
                    b->onClick();
#ifdef RASTRO_UI_METRICS
        initial = editor->uiMetrics();
#endif
        startCpu = std::clock();
        startWall = juce::Time::getMillisecondCounterHiRes();
    }
    void timerCallback() override
    {
        juce::AudioBuffer<float> buffer(2, 1920);
        juce::MidiBuffer midi;
        if (phase > 0 && frames % 7 == 0)
            midi.addEvent(juce::MidiMessage::noteOn(1, 60, .8f), 0);
        p.processBlock(buffer, midi);
        ++frames;
        const double wall = juce::Time::getMillisecondCounterHiRes() - startWall;
        if (wall < 2000)
            return;
        const double cpu = 1000. * (std::clock() - startCpu) / CLOCKS_PER_SEC;
        std::cout << "phase=" << phase << " wall_ms=" << wall << " process_cpu_ms=" << cpu
                  << " cpu_percent=" << 100 * cpu / wall << std::endl;
#ifdef RASTRO_UI_METRICS
        auto m = editor->uiMetrics();
        std::cout << "ticks=" << m.ticks - initial.ticks
                  << " layouts=" << m.layouts - initial.layouts
                  << " main_avatar=" << m.mainAvatar - initial.mainAvatar
                  << " preview_avatar=" << m.previewAvatar - initial.previewAvatar
                  << " deck_updates=" << m.deckUpdates - initial.deckUpdates
                  << " full_repaints=" << m.fullRepaints - initial.fullRepaints << std::endl;
#endif
        if (++phase == 4)
        {
            stopTimer();
            juce::MessageManager::getInstance()->stopDispatchLoop();
        }
        else
            begin();
    }

  public:
    UiBenchmark()
    {
        p.prepareToPlay(48000, 1920);
        p.loadFile(juce::File(RASTRO_TEST_DATA_DIR).getChildFile("mono.mp3"));
        p.set("loop", 1);
        editor = new RastroEditor(p);
        window.setContentOwned(editor, true);
        window.centreWithSize(1180, 840);
        window.setVisible(true);
        begin();
        startTimerHz(25);
    }
    ~UiBenchmark() override
    {
        stopTimer();
        window.clearContentComponent();
    }
};
int main()
{
    juce::ScopedJuceInitialiser_GUI gui;
    UiBenchmark bench;
    juce::MessageManager::getInstance()->runDispatchLoop();
}
