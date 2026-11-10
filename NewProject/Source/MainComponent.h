#pragma once
#include <JuceHeader.h>

struct RecordedMidiEvent
{
    double timestampSeconds;
    juce::MidiMessage message;
};

class AudioChannelStrip : public juce::Component
{
public:
    AudioChannelStrip(int channelNumber, double defaultFreq)
        : chIndex(channelNumber)
    {
        addAndMakeVisible(waveSelector);
        waveSelector.addItem("SINE", 1);
        waveSelector.addItem("SQUARE", 2);
        waveSelector.addItem("SAW", 3);
        waveSelector.setSelectedId(1);
        addAndMakeVisible(loadAudioBtn);
        loadAudioBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2d4a53));
        addAndMakeVisible(clearAudioBtn);
        clearAudioBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff4a1c1c));
        clearAudioBtn.onClick = [this]
            {
                hasLoadedSample = false;
                samplePlayPos = 0;
                sampleBuffer.setSize(0, 0);
                loadAudioBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2d4a53));
            };


        auto setupMiniKnob = [this](juce::Slider& knob, double minV, double maxV, double defV, juce::Colour col)
            {
                addAndMakeVisible(knob);
                knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
                knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
                knob.setRange(minV, maxV, 0.01);
                knob.setValue(defV);
                knob.setColour(juce::Slider::rotarySliderFillColourId, col);
                knob.setColour(juce::Slider::thumbColourId, juce::Colours::white);
            };

        setupMiniKnob(freqSlider, 50.0, 2000.0, defaultFreq, juce::Colours::crimson);
        freqSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 46, 14);
        setupMiniKnob(panSlider, 0.0, 1.0, 0.5, juce::Colours::dodgerblue);

        setupMiniKnob(eqLowSlider, -12.0, 12.0, 0.0, juce::Colours::limegreen);
        setupMiniKnob(eqMidSlider, -12.0, 12.0, 0.0, juce::Colours::gold);
        setupMiniKnob(eqHighSlider, -12.0, 12.0, 0.0, juce::Colours::cyan);

        addAndMakeVisible(muteButton);
        muteButton.setClickingTogglesState(true);
        muteButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::red);

        addAndMakeVisible(soloButton);
        soloButton.setClickingTogglesState(true);
        soloButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::gold);
        soloButton.setColour(juce::TextButton::textColourOnId, juce::Colours::black);

        addAndMakeVisible(volumeSlider);
        volumeSlider.setSliderStyle(juce::Slider::LinearVertical);
        volumeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
        volumeSlider.setRange(0.0, 1.0, 0.01);
        volumeSlider.setValue(0.6);
        volumeSlider.setColour(juce::Slider::thumbColourId, juce::Colours::orangered);
        volumeSlider.setColour(juce::Slider::trackColourId, juce::Colours::white);

        updateEqFilters(48000.0);
    }

    void updateEqFilters(double sampleRate)
    {
        float lowGain = juce::Decibels::decibelsToGain((float)eqLowSlider.getValue());
        float midGain = juce::Decibels::decibelsToGain((float)eqMidSlider.getValue());
        float highGain = juce::Decibels::decibelsToGain((float)eqHighSlider.getValue());

        lowFilter.setCoefficients(juce::IIRCoefficients::makeLowShelf(sampleRate, 200.0, 0.7, lowGain));
        midFilter.setCoefficients(juce::IIRCoefficients::makePeakFilter(sampleRate, 1000.0, 1.0, midGain));
        highFilter.setCoefficients(juce::IIRCoefficients::makeHighShelf(sampleRate, 4000.0, 0.7, highGain));
    }

    float processSampleWithEq(float sample)
    {
        sample = lowFilter.processSingleSampleRaw(sample);
        sample = midFilter.processSingleSampleRaw(sample);
        sample = highFilter.processSingleSampleRaw(sample);
        return sample;
    }

    void paint(juce::Graphics& g) override
    {
        int w = getWidth();
        int h = getHeight();

        g.setColour(juce::Colour(0xff1c1c1e));
        g.fillRect(0, 0, w, h);
        g.setColour(juce::Colours::white.withAlpha(0.75f));
        g.drawRect(0, 0, w, h, 1);

        g.setColour(juce::Colours::white);
        g.fillRect(0, 0, w, 22);
        g.setColour(juce::Colours::black);
        g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        g.drawText("AUDIO " + juce::String(chIndex), 0, 0, w, 22, juce::Justification::centred);

        g.setColour(juce::Colours::lightgrey);
        g.setFont(9.0f);
        g.drawText("PITCH", 0, 50, w, 11, juce::Justification::centred);
        g.drawText("HI", 4, 114, 28, 11, juce::Justification::centred);
        g.drawText("MID", 34, 114, 28, 11, juce::Justification::centred);
        g.drawText("LOW", 64, 114, 28, 11, juce::Justification::centred);
        g.drawText("PAN", 0, 162, w, 11, juce::Justification::centred);
        g.drawText("LEVEL", 0, 245, w, 11, juce::Justification::centred);

        g.setColour(juce::Colours::white);
        g.fillRect(0, h - 20, w, 20);
        g.setColour(juce::Colours::black);
        g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
        g.drawText(juce::String(chIndex), 0, h - 20, w, 20, juce::Justification::centred);
    }

    void resized() override
    {
        int w = getWidth();
        int h = getHeight();

        waveSelector.setBounds(5, 25, w - 66, 21);
        loadAudioBtn.setBounds(w - 59, 25, 28, 21);
        clearAudioBtn.setBounds(w - 29, 25, 24, 21);

        freqSlider.setBounds(15, 61, w - 30, 50);
        eqHighSlider.setBounds(4, 125, 28, 32);
        eqMidSlider.setBounds(34, 125, 28, 32);
        eqLowSlider.setBounds(64, 125, 28, 32);
        panSlider.setBounds(27, 173, 40, 40);
        muteButton.setBounds(10, 218, 35, 22);
        soloButton.setBounds(50, 218, 35, 22);
        volumeSlider.setBounds(15, 258, w - 30, h - 282);
    }
    juce::TextButton loadAudioBtn{ "FILE" };
    juce::TextButton clearAudioBtn{ "CLR" };

    juce::AudioBuffer<float> sampleBuffer;
    int samplePlayPos = 0;
    bool hasLoadedSample = false;

    int chIndex;
    juce::ComboBox waveSelector;
    juce::Slider freqSlider, panSlider, volumeSlider;
    juce::Slider eqLowSlider, eqMidSlider, eqHighSlider;
    juce::IIRFilter lowFilter, midFilter, highFilter;
    juce::TextButton muteButton{ "M" }, soloButton{ "S" };
    double phase = 0.0;
};
class MidiChannelStrip : public juce::Component
{
public:
    MidiChannelStrip(int midiNumber, juce::AudioDeviceManager& dm)
        : midiIndex(midiNumber), deviceManager(dm)
    {
        addAndMakeVisible(inSelector);
        addAndMakeVisible(outSelector);

        inSelector.onChange = [this]()
            {
                int idx = inSelector.getSelectedId() - 2;
                auto inputs = juce::MidiInput::getAvailableDevices();
                if (idx >= 0 && idx < inputs.size())
                {
                    assignedInId = inputs[idx].identifier;
                    deviceManager.setMidiInputDeviceEnabled(assignedInId, true);
                }
                else
                {
                    assignedInId = "";
                }
            };

        outSelector.onChange = [this]()
            {
                int idx = outSelector.getSelectedId() - 2;
                auto outputs = juce::MidiOutput::getAvailableDevices();
                if (idx >= 0 && idx < outputs.size())
                    midiOut = juce::MidiOutput::openDevice(outputs[idx].identifier);
                else
                    midiOut = nullptr;
            };

        addAndMakeVisible(recButton);
        recButton.setClickingTogglesState(true);
        recButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::crimson);
        recButton.onClick = [this]()
            {
                if (recButton.getToggleState())
                {
                    recordedEvents.clear();
                    recordStartTime = juce::Time::getMillisecondCounterHiRes() * 0.001;
                    isRecording = true;
                    isLooping = false;
                    loopButton.setToggleState(false, juce::dontSendNotification);
                }
                else
                {
                    isRecording = false;
                    loopDuration = (juce::Time::getMillisecondCounterHiRes() * 0.001) - recordStartTime;
                    if (loopDuration < 0.5) loopDuration = 2.0;
                }
            };

        addAndMakeVisible(loopButton);
        loopButton.setClickingTogglesState(true);
        loopButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::limegreen);
        loopButton.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
        loopButton.onClick = [this]()
            {
                isLooping = loopButton.getToggleState() && !recordedEvents.empty();
                if (isLooping)
                {
                    isRecording = false;
                    recButton.setToggleState(false, juce::dontSendNotification);
                    loopStartTime = juce::Time::getMillisecondCounterHiRes() * 0.001;
                    lastLoopCheckTime = 0.0;
                }
            };

        addAndMakeVisible(muteButton);
        muteButton.setClickingTogglesState(true);
        muteButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::red);

        addAndMakeVisible(soloButton);
        soloButton.setClickingTogglesState(true);
        soloButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::gold);
        soloButton.setColour(juce::TextButton::textColourOnId, juce::Colours::black);

        addAndMakeVisible(velSlider);
        velSlider.setSliderStyle(juce::Slider::LinearVertical);
        velSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
        velSlider.setRange(0.0, 1.0, 0.01);
        velSlider.setValue(1.0);
        velSlider.setColour(juce::Slider::thumbColourId, juce::Colours::deepskyblue);
        velSlider.setColour(juce::Slider::trackColourId, juce::Colours::white);

        refreshDevices();
    }
    void refreshDevices()
    {
        auto inputs = juce::MidiInput::getAvailableDevices();
        auto outputs = juce::MidiOutput::getAvailableDevices();

        auto fill = [](juce::ComboBox& box, const juce::Array<juce::MidiDeviceInfo>& list)
            {
                int prev = box.getSelectedId();
                box.clear();
                box.addItem("None", 1);
                for (int i = 0; i < list.size(); ++i)
                    box.addItem(list[i].name, i + 2);
                box.setSelectedId(prev > 0 ? prev : 1, juce::dontSendNotification);
            };

        fill(inSelector, inputs);
        fill(outSelector, outputs);
    }

    void paint(juce::Graphics& g) override
    {
        int w = getWidth();
        int h = getHeight();

        g.setColour(juce::Colour(0xff182026));
        g.fillRect(0, 0, w, h);
        g.setColour(juce::Colours::deepskyblue.withAlpha(0.8f));
        g.drawRect(0, 0, w, h, 1);

        g.setColour(juce::Colours::deepskyblue);
        g.fillRect(0, 0, w, 22);
        g.setColour(juce::Colours::black);
        g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        g.drawText("MIDI " + juce::String(midiIndex), 0, 0, w, 22, juce::Justification::centred);

        g.setColour(hasActivity ? juce::Colours::limegreen : juce::Colours::darkgreen.darker());
        g.fillEllipse((float)(w - 16), 6.0f, 10.0f, 10.0f);
        hasActivity = false;

        g.setColour(juce::Colours::lightskyblue);
        g.setFont(9.5f);
        g.drawText("MIDI IN", 0, 28, w, 12, juce::Justification::centred);
        g.drawText("MIDI OUT", 0, 74, w, 12, juce::Justification::centred);
        g.drawText("CLIP LOOPER", 0, 122, w, 12, juce::Justification::centred);
        g.drawText("VELOCITY", 0, 245, w, 11, juce::Justification::centred);

        g.setColour(juce::Colour(0xff0f1418));
        g.fillRect(8, 168, w - 16, 18);
        g.setColour(isRecording ? juce::Colours::crimson : (isLooping ? juce::Colours::limegreen : juce::Colours::grey));
        g.drawRect(8, 168, w - 16, 18, 1);
        g.setFont(9.0f);
        juce::String statusTxt = isRecording ? "REC..." : (isLooping ? "PLAYING" : (recordedEvents.empty() ? "EMPTY" : "CLIP OK"));
        g.drawText(statusTxt, 8, 168, w - 16, 18, juce::Justification::centred);

        g.setColour(juce::Colours::deepskyblue);
        g.fillRect(0, h - 20, w, 20);
        g.setColour(juce::Colours::black);
        g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        g.drawText("M" + juce::String(midiIndex), 0, h - 20, w, 20, juce::Justification::centred);
    }

    void resized() override
    {
        int w = getWidth();
        int h = getHeight();

        inSelector.setBounds(6, 42, w - 12, 22);
        outSelector.setBounds(6, 88, w - 12, 22);
        recButton.setBounds(8, 138, 38, 22);
        loopButton.setBounds(50, 138, 38, 22);
        muteButton.setBounds(10, 218, 35, 22);
        soloButton.setBounds(50, 218, 35, 22);
        velSlider.setBounds(15, 258, w - 30, h - 282);
    }

    int midiIndex;
    juce::AudioDeviceManager& deviceManager;
    juce::ComboBox inSelector, outSelector;
    juce::TextButton recButton{ "REC" }, loopButton{ "LOOP" };
    juce::TextButton muteButton{ "M" }, soloButton{ "S" };
    juce::Slider velSlider;

    juce::String assignedInId;
    std::unique_ptr<juce::MidiOutput> midiOut;
    bool hasActivity = false;

    bool isRecording = false;
    bool isLooping = false;
    double recordStartTime = 0.0;
    double loopDuration = 2.0;
    double loopStartTime = 0.0;
    double lastLoopCheckTime = 0.0;
    std::vector<RecordedMidiEvent> recordedEvents;
};
class MainComponent : public juce::AudioAppComponent,
    public juce::Timer,
    public juce::MenuBarModel, public juce::MidiInputCallback

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
    juce::StringArray getMenuBarNames() override;
    juce::PopupMenu getMenuForIndex(int topLevelMenuIndex, const juce::String& menuName) override;
    void menuItemSelected(int menuItemID, int topLevelMenuIndex) override;

    void handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message) override;

private:
    juce::Viewport channelViewport;    std::unique_ptr<juce::MenuBarComponent> menuBar;
    void newProject();

    juce::Component channelContainer;

    juce::OwnedArray<AudioChannelStrip> audioChannels;
    juce::OwnedArray<MidiChannelStrip> midiChannels;

    void addAudioChannel(double defaultFreq = 440.0);
    void addMidiChannel();
    void layoutDynamicChannels();
    void triggerInternalSynths(const juce::MidiMessage& message, float velocityScale);

    // Grabador Master Unificado (Audio WAV + Archivo MIDI)
    juce::AudioBuffer<float> recordedAudioBuffer;
    int recordedSamplesCount = 0;
    juce::MidiMessageSequence recordedMidiSeq;
    double masterRecStartTime = 0.0;
    bool isRecordingMaster = false;
    void startMasterRecording();
    void stopMasterRecording();
    // Osciloscopio en tiempo real
    std::vector<float> scopeBuffer;
    int scopeWritePos = 0;
    void loadSampleIntoChannel(AudioChannelStrip* strip);
    juce::AudioFormatManager formatManager;
    // Metrónomo y BPM
    juce::Slider bpmSlider;
    juce::TextButton metronomeBtn{ "CLICK" };
    int metroSampleCounter = 0;
    int metroBeepSamplesLeft = 0;
    double metroPhase = 0.0;





    void saveSessionToFile();
    void loadSessionFromFile();
    std::unique_ptr<juce::FileChooser> fileChooser;

    juce::Slider reverbSizeSlider, reverbMixSlider;
    juce::Slider delayTimeSlider, delayFeedbackSlider;
    juce::Reverb masterReverb;
    juce::Reverb::Parameters reverbParams;
    std::vector<float> delayBufferLeft, delayBufferRight;
    int delayWritePosition = 0;

    juce::Slider masterVolumeSlider;
    float currentLevelLeft = 0.0f, currentLevelRight = 0.0f;

    juce::TextButton playButton{ "PLAY" };
    juce::TextButton stopButton{ "STOP" };
    juce::TextButton recWavButton{ "REC WAV" };
    juce::TextButton addAudioBtn{ "+ AUDIO" };
    juce::TextButton addMidiBtn{ "+ MIDI" };
    juce::TextButton saveBtn{ "SAVE" };
    juce::TextButton loadBtn{ "LOAD" };
    juce::TextButton setupButton{ "AUDIO SETUP" };

    std::unique_ptr<juce::AudioDeviceSelectorComponent> audioSetupComp;

    bool isPlaying = false;
    bool showAudioSetup = false;

    float generateWave(double phase, int waveType);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
