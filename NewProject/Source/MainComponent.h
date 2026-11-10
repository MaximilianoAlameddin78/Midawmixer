#include <JuceHeader.h>

class MainComponent : public juce::AudioAppComponent,
    public juce::Timer,
    public juce::MidiInputCallback // Permite recibir datos del KeyStep y APC mini
{
public:
    MainComponent();
    ~MainComponent() override;

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    // Función oficial de JUCE que recibe cada tecla o movimiento de fader MIDI
    void handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message) override;

private:
    // Controles del Canal 1
    juce::Slider volumeSlider1;
    juce::Slider freqSlider1;
    juce::Slider panSlider1;

    // Controles del Canal 2
    juce::Slider volumeSlider2;
    juce::Slider freqSlider2;
    juce::Slider panSlider2;

    juce::TextButton playButton;
    juce::TextButton stopButton;
    juce::TextButton setupButton;

    std::unique_ptr<juce::AudioDeviceSelectorComponent> audioSetupComp;

    bool isPlaying = false;
    bool showAudioSetup = false;

    // Variables de audio independientes para Canal 1
    double phase1 = 0.0;
    double phaseIncrement1 = 0.0;

    // Variables de audio independientes para Canal 2
    double phase2 = 0.0;
    double phaseIncrement2 = 0.0;

    // Variables para medir la intensidad del Vúmetro LED (Izquierda y Derecha)
    float currentLevelLeft = 0.0f;
    float currentLevelRight = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
