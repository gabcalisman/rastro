#include "Processor.h"
#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>
class RastroApplication : public juce::JUCEApplication
{
    juce::ApplicationProperties properties;
    struct Window : juce::StandaloneFilterWindow
    {
        explicit Window(juce::PropertySet *settings)
            : StandaloneFilterWindow("RASTRO Play", juce::Colour(0xff141829), settings, false)
        {
        }
        void closeButtonPressed() override { juce::JUCEApplicationBase::quit(); }
    };
    std::unique_ptr<Window> window;

  public:
    const juce::String getApplicationName() override { return "RASTRO"; }
    const juce::String getApplicationVersion() override { return "0.1.0"; }
    void initialise(const juce::String &) override
    {
        juce::PropertiesFile::Options options;
        options.applicationName = "RASTRO";
        options.filenameSuffix = ".settings";
        options.osxLibrarySubFolder = "Application Support";
#if JUCE_LINUX
        options.folderName = "~/.config";
#endif
        properties.setStorageParameters(options);
        window = std::make_unique<Window>(properties.getUserSettings());
        auto *holder = window->getPluginHolder();
        // Input is routed to the recorder. Monitoring in the processor remains OFF by default.
        holder->getMuteInputValue() = false;
        if (auto *p = dynamic_cast<RastroProcessor *>(holder->processor.get()))
        {
            p->standaloneAudioSettings = [holder]
            {
                holder->getMuteInputValue() = false;
                holder->showAudioSettingsDialog();
            };
            p->standaloneInputAvailable = [holder]
            {
                auto *d = holder->deviceManager.getCurrentAudioDevice();
                return d && d->getActiveInputChannels().countNumberOfSetBits() > 0;
            };
        }
        window->setVisible(true);
    }
    void shutdown() override
    {
        if (window)
            if (auto *p =
                    dynamic_cast<RastroProcessor *>(window->getPluginHolder()->processor.get()))
            {
                p->standaloneAudioSettings = {};
                p->standaloneInputAvailable = {};
            }
        window = nullptr;
        properties.saveIfNeeded();
    }
    void systemRequestedQuit() override { quit(); }
};
JUCE_CREATE_APPLICATION_DEFINE(RastroApplication)
