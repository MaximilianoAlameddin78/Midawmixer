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
    AudioChannelStrip(int channelNumber, double /*defaultFreq*/)
        : chIndex(channelNumber)
    {
        addAndMakeVisible(inputSelector);
        inputSelector.addItem("Off (Solo File)", 1);
        inputSelector.addItem("In 1 (Mono) - Guitar/Mic", 2);
        inputSelector.addItem("In 2 (Mono) - Bass/Mic", 3);
        inputSelector.addItem("In 1+2 (Stereo)", 4);
        inputSelector.setSelectedId(1);

        addAndMakeVisible(monitorBtn);
        monitorBtn.setClickingTogglesState(true);
        monitorBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2c2c2e));
        monitorBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colours::limegreen);
        monitorBtn.setColour(juce::TextButton::textColourOnId, juce::Colours::black);

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

        setupMiniKnob(inputGainSlider, 0.0, 2.0, 1.0, juce::Colours::orange);
        inputGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 46, 14);
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
        addAndMakeVisible(recButton);
        recButton.setClickingTogglesState(true);
        recButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::red);

        addAndMakeVisible(volumeSlider);
        volumeSlider.setSliderStyle(juce::Slider::LinearVertical);
        volumeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
        volumeSlider.setRange(0.0, 1.0, 0.01);
        volumeSlider.setValue(0.75);
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

        g.setColour(juce::Colour(0xff00b4d8));
        g.fillRect(0, 0, w, 22);
        g.setColour(juce::Colours::black);
        g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        g.drawText("AUDIO " + juce::String(chIndex), 0, 0, w, 22, juce::Justification::centred);

        g.setColour(juce::Colours::lightgrey);
        g.setFont(9.0f);
        g.drawText("IN GAIN", 0, 50, w, 11, juce::Justification::centred);
        g.drawText("HI", 4, 114, 28, 11, juce::Justification::centred);
        g.drawText("MID", 34, 114, 28, 11, juce::Justification::centred);
        g.drawText("LOW", 64, 114, 28, 11, juce::Justification::centred);
        g.drawText("PAN", 0, 162, w, 11, juce::Justification::centred);
        g.drawText("LEVEL", 0, 245, w, 11, juce::Justification::centred);

        g.setColour(juce::Colour(0xff00b4d8));
        g.fillRect(0, h - 20, w, 20);
        g.setColour(juce::Colours::black);
        g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
        g.drawText(juce::String(chIndex), 0, h - 20, w, 20, juce::Justification::centred);
    }

    void resized() override
    {
        int w = getWidth();
        int h = getHeight();

        inputSelector.setBounds(4, 25, w - 8, 21);
        monitorBtn.setBounds(4, 48, 28, 18);
        loadAudioBtn.setBounds(w - 57, 48, 28, 18);
        clearAudioBtn.setBounds(w - 27, 48, 23, 18);

        inputGainSlider.setBounds(15, 67, w - 30, 44);
        eqHighSlider.setBounds(4, 125, 28, 32);
        eqMidSlider.setBounds(34, 125, 28, 32);
        eqLowSlider.setBounds(64, 125, 28, 32);
        panSlider.setBounds(27, 173, 40, 40);
        muteButton.setBounds(6, 218, 26, 22);
        soloButton.setBounds(34, 218, 26, 22);
        recButton.setBounds(62, 218, 26, 22);

        volumeSlider.setBounds(15, 258, w - 30, h - 282);
    }

    juce::ComboBox inputSelector;
    juce::ComboBox& waveSelector = inputSelector;
    juce::TextButton monitorBtn{ "MON" };
    juce::TextButton loadAudioBtn{ "FILE" };
    juce::TextButton clearAudioBtn{ "CLR" };

    juce::AudioBuffer<float> sampleBuffer;
    int samplePlayPos = 0;
    bool hasLoadedSample = false;

    int chIndex;
    juce::Slider inputGainSlider, panSlider, volumeSlider;
    juce::Slider& freqSlider = inputGainSlider;
    juce::TextButton recButton{ "R" };
    juce::AudioBuffer<float> trackRecBuffer;
    int trackRecordedSamples = 0;

    void startTrackRecording(double sampleRate)
    {
        trackRecordedSamples = 0;
        trackRecBuffer.setSize(1, (int)(sampleRate * 600.0)); // Hasta 10 minutos aislados por pista
        trackRecBuffer.clear();
    }

    void appendTrackSample(float sample)
    {
        if (!recButton.getToggleState() || trackRecBuffer.getNumSamples() == 0)
            return;
        if (trackRecordedSamples < trackRecBuffer.getNumSamples())
            trackRecBuffer.setSample(0, trackRecordedSamples++, sample);
    }

    juce::Slider eqLowSlider, eqMidSlider, eqHighSlider;
    juce::IIRFilter lowFilter, midFilter, highFilter;
    juce::TextButton muteButton{ "M" }, soloButton{ "S" };
    double phase = 0.0;

    };
    //==============================================================================
    // Instrumento Virtual Nativo Polifónico por Canal (Synth con Pitch Bend y Mod)
    struct MidawSynthSound : public juce::SynthesiserSound
    {
        bool appliesToNote(int) override { return true; }
        bool appliesToChannel(int) override { return true; }
    };

    struct MidawSynthVoice : public juce::SynthesiserVoice
    {
        std::vector<float> stringBuffer;
        int stringWritePos = 0;
        float stringPrevSample = 0.0f;
        float pluckBurst = 0.0f;


        bool canPlaySound(juce::SynthesiserSound* sound) override
        {
            return dynamic_cast<MidawSynthSound*> (sound) != nullptr;
        }

    
        
        void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int currentPitchWheelPosition) override
        {
            currentAngle = 0.0;
            level = velocity * 0.75;
            tailOff = 0.0;
            currentMidiNote = midiNoteNumber;
            drumEnv = 1.0;
            pitchEnv = 1.0;

            double sr = getSampleRate() > 0.0 ? getSampleRate() : 48000.0;
            auto cyclesPerSecond = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
            baseAngleDelta = cyclesPerSecond / sr;
            if (presetType == 5 || presetType == 6 || presetType == 7)
            {
                double targetHz = (presetType == 5 && midiNoteNumber >= 48) ? cyclesPerSecond * 0.5 : cyclesPerSecond;
                int delayLen = juce::jlimit(16, 4096, (int)(sr / juce::jmax(20.0, targetHz)));
                stringBuffer.assign(delayLen, 0.0f);
                stringWritePos = 0;
                stringPrevSample = 0.0f;

                float lp = 0.0f;
                float smooth = (presetType == 5) ? 0.86f : (presetType == 6 ? 0.45f : 0.30f);
                for (int i = 0; i < delayLen; ++i)
                {
                    float rnd = (drumRng.nextFloat() * 2.0f - 1.0f);
                    lp = smooth * lp + (1.0f - smooth) * rnd;
                    float fund = (float)std::sin(2.0 * 3.14159265359 * (double)i / (double)delayLen);
                    stringBuffer[i] = (lp * 0.78f + fund * 0.42f);
                }
            }

            if (presetType == 4)
            {
                int padType = getDrumPieceType(midiNoteNumber);
                if (padType == 0)      baseAngleDelta = 150.0 / sr;  // Kick (Bombo con pegada y caida a 48 Hz)
                else if (padType == 1) baseAngleDelta = 210.0 / sr;  // Snare (Caja con golpe y bordona)
                else if (padType == 2) baseAngleDelta = 1200.0 / sr; // Closed Hi-Hat (Cerrado corto)
                else if (padType == 3) baseAngleDelta = 950.0 / sr;  // Open Hi-Hat (Abierto)
                else if (padType == 4) baseAngleDelta = 140.0 / sr;  // Low Tom (Tom grave)
                else if (padType == 5) baseAngleDelta = 220.0 / sr;  // High Tom (Tom agudo)
                else if (padType == 6) baseAngleDelta = 340.0 / sr;  // Clap / Rimshot
                else                   baseAngleDelta = 700.0 / sr;  // Crash Cymbal (Platillo largo)
            }

            double normPitch = (currentPitchWheelPosition - 8192) / 8192.0;
            pitchBendFactor = std::pow(2.0, (normPitch * 2.0) / 12.0);
            angleDelta = baseAngleDelta * pitchBendFactor;
        }

        void stopNote(float, bool allowTailOff) override
        {
            if (presetType == 4)
                return;

            if (allowTailOff)
            {
                if (tailOff == 0.0) tailOff = 1.0;
            }
            else
            {
                clearCurrentNote();
                angleDelta = 0.0;
                baseAngleDelta = 0.0;
            }
        }

        void pitchWheelMoved(int newPitchWheelValue) override
        {
            double normPitch = (newPitchWheelValue - 8192) / 8192.0;
            pitchBendFactor = std::pow(2.0, (normPitch * 2.0) / 12.0);
            angleDelta = baseAngleDelta * pitchBendFactor;
        }

        void controllerMoved(int controllerNumber, int newControllerValue) override
        {
            if (controllerNumber == 1)
                modWheelDepth = (float)newControllerValue / 127.0f;
        }

        void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override
        {
            if (baseAngleDelta > 0.0)
            {
                while (--numSamples >= 0)
                {
                    lfoPhase += 4.5 / getSampleRate();
                    if (lfoPhase >= 1.0) lfoPhase -= 1.0;
                    double vibrato = 1.0 + std::sin(lfoPhase * 2.0 * 3.14159265359) * (0.015 * modWheelDepth);
                    angleDelta = baseAngleDelta * vibrato;

                    float raw = 0.0f;
                    if (presetType == 1) // PAD ATMOSFERICO
                    {
                        double twoPi = 2.0 * 3.14159265359;
                        raw = (float)((std::sin(currentAngle * twoPi) * 0.55 + std::sin(currentAngle * 1.004 * twoPi) * 0.45) * 0.8);
                    }
                    else if (presetType == 2) // PIANO / KEYS
                    {
                        double twoPi = 2.0 * 3.14159265359;
                        raw = (float)((std::sin(currentAngle * twoPi) * 0.65 + std::sin(currentAngle * 2.0 * twoPi) * 0.25 + std::sin(currentAngle * 3.0 * twoPi) * 0.10) * 0.85);
                    }
                    else if (presetType == 3) // SAW LEAD / BASS ANALOGICO
                    {
                        double phase = currentAngle - std::floor(currentAngle);
                        raw = (float)(2.0 * phase - 1.0) * 0.65f;
                    }
                    else if (presetType == 5) // BAJO ELECTRICO REAL (FINGER / PICK BASS)
                    {
                        if (!stringBuffer.empty())
                        {
                            int len = (int)stringBuffer.size();
                            float cur = stringBuffer[stringWritePos];
                            float next = stringBuffer[(stringWritePos + 1) % len];
                            float filtered = 0.9965f * (0.68f * cur + 0.32f * next);
                            stringBuffer[stringWritePos] = filtered;
                            stringWritePos = (stringWritePos + 1) % len;
                            raw = std::tanh(filtered * 1.85f) * 1.15f;
                        }
                    }
                    else if (presetType == 6) // GUITARRA ACUSTICA REAL (CUERDAS DE ACERO + CAJA)
                    {
                        if (!stringBuffer.empty())
                        {
                            int len = (int)stringBuffer.size();
                            float cur = stringBuffer[stringWritePos];
                            float next = stringBuffer[(stringWritePos + 1) % len];
                            float filtered = 0.9945f * (0.50f * cur + 0.50f * next);
                            stringBuffer[stringWritePos] = filtered;
                            stringWritePos = (stringWritePos + 1) % len;
                            stringPrevSample = 0.75f * stringPrevSample + 0.25f * filtered;
                            raw = (filtered * 1.15f + stringPrevSample * 0.45f);
                        }
                    }
                    else if (presetType == 7) // GUITARRA ELECTRICA REAL (AMPLIFICADOR + OVERDRIVE)
                    {
                        if (!stringBuffer.empty())
                        {
                            int len = (int)stringBuffer.size();
                            float cur = stringBuffer[stringWritePos];
                            float next = stringBuffer[(stringWritePos + 1) % len];
                            float filtered = 0.9978f * (0.52f * cur + 0.48f * next);
                            stringBuffer[stringWritePos] = filtered;
                            stringWritePos = (stringWritePos + 1) % len;
                            float gain = 2.4f + modWheelDepth * 4.5f;
                            float amp = std::tanh(filtered * gain);
                            stringPrevSample = 0.62f * stringPrevSample + 0.38f * amp;
                            raw = stringPrevSample * 0.95f;
                        }
                    }
                    else if (presetType == 4) // BATERIA
                    {
                        int padType = getDrumPieceType(currentMidiNote);
                        float white = drumRng.nextFloat() * 2.0f - 1.0f;
                        float metallic = (std::sin(currentAngle * 23.0) + std::sin(currentAngle * 37.0) + std::sin(currentAngle * 51.0)) * 0.33f;

                        if (padType == 0) // KICK
                        {
                            static double kickPhase = 0.0;
                            float freq = 42.0f + 135.0f * (float)pitchEnv;
                            kickPhase += (double)freq / getSampleRate();
                            raw = (float)(std::sin(kickPhase * 2.0 * 3.14159265359) * drumEnv * 1.25);
                            pitchEnv *= 0.9972;
                            drumEnv *= 0.99935;
                        }
                        else if (padType == 1) // SNARE
                        {
                            raw = (float)(std::sin(currentAngle * 185.0 * 2.0 * 3.14159265359) * 0.45 + white * 0.85) * (float)drumEnv;
                            drumEnv *= 0.9991;
                        }
                        else if (padType == 2) // CLOSED HI-HAT
                        {
                            raw = metallic * (float)drumEnv * 0.75f;
                            drumEnv *= 0.9968;
                        }
                        else if (padType == 3) // OPEN HI-HAT
                        {
                            raw = (metallic * 0.7f + white * 0.3f) * (float)drumEnv * 0.75f;
                            drumEnv *= 0.99945;
                        }
                        else if (padType == 4 || padType == 5) // TOMS
                        {
                            float baseF = (padType == 4) ? 75.0f : 125.0f;
                            float freq = baseF + 70.0f * (float)pitchEnv;
                            currentAngle += (double)freq / getSampleRate();
                            raw = (float)(std::sin(currentAngle * 2.0 * 3.14159265359) * drumEnv * 1.1);
                            pitchEnv *= 0.998;
                            drumEnv *= 0.9992;
                        }
                        else if (padType == 6) // CLAP / RIM
                        {
                            raw = (white * 0.9f + metallic * 0.2f) * (float)drumEnv * 0.95f;
                            drumEnv *= 0.9985;
                        }
                        else // CRASH / RIDE
                        {
                            raw = (metallic * 0.6f + white * 0.4f) * (float)drumEnv * 0.8f;
                            drumEnv *= 0.99965;
                        }
                    }

                    float currentSample = raw * (float)level * (tailOff > 0.0 ? (float)tailOff : 1.0f);

                    for (int i = outputBuffer.getNumChannels(); --i >= 0;)
                        outputBuffer.addSample(i, startSample, currentSample);

                    currentAngle += angleDelta;
                    ++startSample;
                    if (tailOff > 0.0)
                    {
                        tailOff *= 0.99;
                        if (tailOff <= 0.005)
                        {
                            clearCurrentNote();
                            angleDelta = 0.0;
                            baseAngleDelta = 0.0;
                            break;
                        }
                    }
                }
            }
        }
        int presetType = 2;
    private:
        double lfoPhase = 0.0;



        static int getDrumPieceType(int note)
        {
            if (note == 35 || note == 36) return 0; // Kick
            if (note == 38 || note == 40) return 1; // Snare
            if (note == 42 || note == 44) return 2; // Closed Hat
            if (note == 46)               return 3; // Open Hat
            if (note == 41 || note == 43 || note == 45) return 4; // Low Tom
            if (note == 47 || note == 48 || note == 50) return 5; // High Tom
            if (note == 37 || note == 39) return 6; // Clap / Rim
            if (note == 49 || note == 51 || note == 57) return 7; // Crash / Ride
            return note % 8;
        }

        double currentAngle = 0.0, angleDelta = 0.0, level = 0.0, tailOff = 0.0;
        double baseAngleDelta = 0.0;
        double pitchBendFactor = 1.0;
        float modWheelDepth = 0.0f;
        int currentMidiNote = 36;
        double drumEnv = 0.0;
        double pitchEnv = 0.0;
        float lastNoiseSample = 0.0f;
        juce::Random drumRng;
    };
    class MidiChannelStrip : public juce::Component
    {
    public:
        MidiChannelStrip(int midiNumber, juce::AudioDeviceManager& dm)
            : midiIndex(midiNumber), deviceManager(dm)
        {
            addAndMakeVisible(inSelector);
            addAndMakeVisible(outSelector);
            addChildComponent(instSelector);
            addAndMakeVisible(inSelector);
            addAndMakeVisible(outSelector);
            addChildComponent(instSelector);
            addMouseListener(this, true);

            for (int v = 0; v < 8; ++v)
                channelSynth.addVoice(new MidawSynthVoice());
            channelSynth.addSound(new MidawSynthSound());
            channelSynth.setCurrentPlaybackSampleRate(48000.0);

            instSelector.addItem("None (Off)", 1);
            instSelector.addItem("Sine Pad", 2);
            instSelector.addItem("Harmonic Keys", 3);
            instSelector.addItem("Saw Lead", 4);
            instSelector.addItem("Drum Kit", 5);
            instSelector.addItem("Bajo Electrico", 6);
            instSelector.addItem("Guitarra Acustica", 7);
            instSelector.addItem("Guitarra Electrica", 8);

            instSelector.setSelectedId(1, juce::dontSendNotification);
            selectedInstrumentMode = 0;
            assignedInstrumentName = "None (Off)";
            updateSynthPreset();

            instSelector.onChange = [this]()
                {
                    selectedInstrumentMode = instSelector.getSelectedId() - 1;
                    assignedInstrumentName = instSelector.getText();
                    if (selectedInstrumentMode == 0)
                        channelSynth.allNotesOff(0, false);
                    else
                        updateSynthPreset();
                    repaint();
                };

            inSelector.onChange = [this]()
                {
                    int sel = inSelector.getSelectedId();
                    if (sel == 1)
                    {
                        assignedInId = "";
                        channelSynth.allNotesOff(0, false);
                    }
                    else if (sel == 2)
                    {
                        assignedInId = "ALL";
                    }
                    else
                    {
                        int idx = sel - 3;

                                        auto inputs = juce::MidiInput::getAvailableDevices();
                                        if (idx >= 0 && idx < inputs.size())
                                        {
                                            assignedInId = inputs[idx].identifier;
                                            deviceManager.setMidiInputDeviceEnabled(assignedInId, true);
                                        }
                                        else
                                        {
                                            assignedInId = "";
                                            channelSynth.allNotesOff(0, false);
                                        }
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

                            assignedInId = "ALL";
                            refreshDevices();
                        }

                        void setAssignedInstrument(int modeId, const juce::String& displayName)
                        {
                            if (modeId >= 1 && modeId <= instSelector.getNumItems())
                                instSelector.setSelectedId(modeId, juce::dontSendNotification);
                            selectedInstrumentMode = modeId - 1;
                            assignedInstrumentName = displayName;
                            if (selectedInstrumentMode <= 0)
                                channelSynth.allNotesOff(0, false);
                            else
                                updateSynthPreset();
                            repaint();
                        }

                        void updateSynthPreset()
                        {
                            for (int v = 0; v < channelSynth.getNumVoices(); ++v)
                            {
                                if (auto* voice = dynamic_cast<MidawSynthVoice*>(channelSynth.getVoice(v)))
                                    voice->presetType = juce::jlimit(1, 7, selectedInstrumentMode);

                            }
                        }

                        void pushIncomingMidi(const juce::MidiMessage& msg)
                        {
                            hasActivity = true;
                            juce::MidiMessage unified(msg);
                            unified.setChannel(1);
                            const juce::ScopedLock sl(channelMidiLock);
                            pendingChannelMidi.addEvent(unified, 0);
                        }

                        void renderChannelAudio(juce::AudioBuffer<float>& mixBuffer, int startSample, int numSamples, double sampleRate, bool anySoloActive)
                        {
                            if (sampleRate > 0.0 && std::abs(channelSynth.getSampleRate() - sampleRate) > 1.0)
                                channelSynth.setCurrentPlaybackSampleRate(sampleRate);

                            juce::MidiBuffer localMidi;
                            {
                                const juce::ScopedLock sl(channelMidiLock);
                                localMidi.swapWith(pendingChannelMidi);
                            }

                            bool isAudible = (selectedInstrumentMode > 0) && !muteButton.getToggleState() && (!anySoloActive || soloButton.getToggleState());

                            if (!isAudible)
                            {
                                channelSynth.allNotesOff(0, true);
                                return;
                            }

                            if (tempChannelBuffer.getNumChannels() != mixBuffer.getNumChannels() || tempChannelBuffer.getNumSamples() < numSamples)
                                tempChannelBuffer.setSize(mixBuffer.getNumChannels(), numSamples, false, false, true);

                            tempChannelBuffer.clear();
                            channelSynth.renderNextBlock(tempChannelBuffer, localMidi, 0, numSamples);

                            float gain = (float)velSlider.getValue();
                            for (int ch = 0; ch < mixBuffer.getNumChannels(); ++ch)
                                mixBuffer.addFrom(ch, startSample, tempChannelBuffer, ch, 0, numSamples, gain);
                        }

                        void refreshDevices()
                        {
                            auto inputs = juce::MidiInput::getAvailableDevices();
                            auto outputs = juce::MidiOutput::getAvailableDevices();

                            int prevIn = inSelector.getSelectedId();
                            inSelector.clear(juce::dontSendNotification);
                            inSelector.addItem("All MIDI In", 1);
                            inSelector.addItem("None (Off)", 2);
                            for (int i = 0; i < inputs.size(); ++i)
                                inSelector.addItem(inputs[i].name, i + 3);
                            inSelector.setSelectedId(prevIn > 0 ? prevIn : 1, juce::dontSendNotification);

                            int prevOut = outSelector.getSelectedId();
                            outSelector.clear(juce::dontSendNotification);
                            outSelector.addItem("None", 1);
                            for (int i = 0; i < outputs.size(); ++i)
                                outSelector.addItem(outputs[i].name, i + 2);
                            outSelector.setSelectedId(prevOut > 0 ? prevOut : 1, juce::dontSendNotification);
                        }
                        void mouseDown(const juce::MouseEvent&) override
                        {
                            if (onSelectChannel)
                                onSelectChannel(midiIndex - 1);
                        }

                        bool isSelectedTrack = false;
                        std::function<void(int)> onSelectChannel;
                        
                        void paint(juce::Graphics& g) override

                        {
                            int w = getWidth();
                            int h = getHeight();

                            g.setColour(juce::Colour(0xff182026));
                            g.fillRect(0, 0, w, h);
                            g.setColour(juce::Colours::deepskyblue.withAlpha(0.8f));
                            g.setColour(isSelectedTrack ? juce::Colours::gold : juce::Colours::deepskyblue.withAlpha(0.8f));
                            g.drawRect(0, 0, w, h, isSelectedTrack ? 2 : 1);


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
                            g.drawText("MIDI IN", 0, 26, w, 12, juce::Justification::centred);
                            g.drawText("MIDI OUT", 0, 66, w, 12, juce::Justification::centred);
                            g.drawText("ASSIGNED SYNTH", 0, 106, w, 12, juce::Justification::centred);
                            g.drawText("CLIP LOOPER", 0, 146, w, 12, juce::Justification::centred);
                            g.drawText("LEVEL", 0, 260, w, 11, juce::Justification::centred);

                            g.setColour(juce::Colour(0xff0d151c));
                            g.fillRoundedRectangle(6.0f, 119.0f, (float)(w - 12), 22.0f, 4.0f);
                            g.setColour(selectedInstrumentMode > 0 ? juce::Colours::cyan.withAlpha(0.85f) : juce::Colours::grey);
                            g.drawRoundedRectangle(6.0f, 119.0f, (float)(w - 12), 22.0f, 4.0f, 1.0f);
                            g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
                            g.drawText(assignedInstrumentName, 8, 119, w - 16, 22, juce::Justification::centred, true);

                            g.setColour(juce::Colour(0xff0f1418));
                            g.fillRect(8, 186, w - 16, 16);
                            g.setColour(isRecording ? juce::Colours::crimson : (isLooping ? juce::Colours::limegreen : juce::Colours::grey));
                            g.drawRect(8, 186, w - 16, 16, 1);
                            g.setFont(9.0f);
                            juce::String statusTxt = isRecording ? "REC..." : (isLooping ? "PLAYING" : (recordedEvents.empty() ? "EMPTY" : "READY"));
                            g.drawText(statusTxt, 8, 186, w - 16, 16, juce::Justification::centred);

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

                            inSelector.setBounds(6, 39, w - 12, 22);
                            outSelector.setBounds(6, 79, w - 12, 22);
                            recButton.setBounds(8, 160, 38, 22);
                            loopButton.setBounds(50, 160, 38, 22);
                            muteButton.setBounds(10, 232, 35, 22);
                            soloButton.setBounds(50, 232, 35, 22);
                            velSlider.setBounds(15, 273, w - 30, h - 297);
                        }

                        int midiIndex;
                        juce::AudioDeviceManager& deviceManager;
                        juce::ComboBox inSelector, outSelector, instSelector;
                        juce::String assignedInstrumentName{ "None (Off)" };
                        juce::TextButton recButton{ "REC" }, loopButton{ "LOOP" };
                        juce::TextButton muteButton{ "M" }, soloButton{ "S" };
                        juce::Slider velSlider;

                        juce::String assignedInId;
                        std::unique_ptr<juce::MidiOutput> midiOut;
                        bool hasActivity = false;
                        int selectedInstrumentMode = 0;

                        juce::Synthesiser channelSynth;
                        juce::CriticalSection channelMidiLock;
                        juce::MidiBuffer pendingChannelMidi;
                        juce::AudioBuffer<float> tempChannelBuffer;

                        bool isRecording = false;
                        bool isLooping = false;
                        double recordStartTime = 0.0;
                        double loopDuration = 2.0;
                        double loopStartTime = 0.0;
                        double lastLoopCheckTime = 0.0;
                        std::vector<RecordedMidiEvent> recordedEvents;
};


//==============================================================================
// Ventana flotante para la interfaz grafica original de Plugins VST3 externos
class PluginEditorWindow : public juce::DocumentWindow
{
public:
    PluginEditorWindow(const juce::String& name, juce::AudioProcessorEditor* editor)
        : DocumentWindow(name, juce::Colour(0xff1e1e1e), DocumentWindow::closeButton)
    {
        setUsingNativeTitleBar(true);
        setContentOwned(editor, true);
        centreWithSize(getWidth(), getHeight());
        setVisible(true);
    }

    void closeButtonPressed() override
    {
        setVisible(false);
    }
};
struct TimelineClip
{
    int trackIndex = 0;          // 0..N-1 (Audio primero, luego MIDI)
    bool isMidi = false;
    juce::String name;
    juce::Colour colour = juce::Colours::deepskyblue;
    double startSec = 0.0;       // Posicion en el timeline
    double durationSec = 4.0;    // Duracion visible actual
    double sourceOffsetSec = 0.0;// Recorte desde el inicio
    double maxDurationSec = 4.0; // Duracion total grabada
    std::vector<float> waveformPeaks; // Picos de onda (0.0 .. 1.0) para Audio
    struct NoteVisual { double startRel; double dur; int note; };
    std::vector<NoteVisual> midiNotes; // Notas visuales para MIDI
};

class ArrangementTimeline : public juce::Component
{
public:
    std::vector<TimelineClip> clips;
    juce::StringArray trackNames;
    std::vector<bool> trackIsMidi;
    double playheadSec = 0.0;
    double pixelsPerSecond = 45.0;
    double bpm = 120.0;

    void paint(juce::Graphics& g) override
    {
        int w = getWidth();
        int h = getHeight();
        g.fillAll(juce::Colour(0xff12171d));
        g.setColour(juce::Colours::deepskyblue.withAlpha(0.6f));
        g.drawRect(0, 0, w, h, 1);

        int headerW = 92;
        int rulerH = 22;

        // Regla superior de compases/tiempo
        g.setColour(juce::Colour(0xff1b242e));
        g.fillRect(0, 0, w, rulerH);
        g.setColour(juce::Colours::lightgrey);
        g.setFont(10.0f);
        g.drawText("TIMELINE", 6, 0, headerW - 8, rulerH, juce::Justification::centredLeft);

        double secPerBar = (60.0 / juce::jmax(30.0, bpm)) * 4.0;
        for (int bar = 0; bar < 64; ++bar)
        {
            int x = headerW + (int)(bar * secPerBar * pixelsPerSecond);
            if (x > w) break;
            g.setColour(juce::Colours::white.withAlpha(0.25f));
            g.drawVerticalLine(x, 0.0f, (float)h);
            g.setColour(juce::Colours::silver);
            g.drawText(juce::String(bar + 1), x + 3, 2, 28, rulerH - 4, juce::Justification::centredLeft);
        }

        int numTracks = juce::jmax(1, trackNames.size());
        int trackH = juce::jlimit(26, 54, (h - rulerH) / numTracks);

        // Cabeceras y carriles de pistas
        for (int t = 0; t < trackNames.size(); ++t)
        {
            int y = rulerH + t * trackH;
            if (y >= h) break;

            g.setColour(t % 2 == 0 ? juce::Colour(0xff161d24) : juce::Colour(0xff1a222b));
            g.fillRect(headerW, y, w - headerW, trackH);
            g.setColour(juce::Colours::white.withAlpha(0.08f));
            g.drawHorizontalLine(y + trackH - 1, 0.0f, (float)w);

            g.setColour(trackIsMidi[t] ? juce::Colour(0xff183142) : juce::Colour(0xff1d2b36));
            g.fillRect(1, y + 1, headerW - 2, trackH - 2);
            g.setColour(trackIsMidi[t] ? juce::Colours::lightskyblue : juce::Colours::aquamarine);
            g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
            g.drawText(trackNames[t], 6, y, headerW - 10, trackH, juce::Justification::centredLeft);
        }

        // Dibujar cada clip con su Forma de Onda (Audio) o Notas (MIDI)
        for (int i = 0; i < (int)clips.size(); ++i)
        {
            const auto& c = clips[i];
            if (c.trackIndex < 0 || c.trackIndex >= trackNames.size()) continue;

            int y = rulerH + c.trackIndex * trackH + 2;
            int ch = trackH - 4;
            int x = headerW + (int)(c.startSec * pixelsPerSecond);
            int cw = juce::jmax(8, (int)(c.durationSec * pixelsPerSecond));

            g.setColour(c.colour.withAlpha(0.32f));
            g.fillRoundedRectangle((float)x, (float)y, (float)cw, (float)ch, 4.0f);
            g.setColour(i == selectedClipIndex ? juce::Colours::gold : c.colour);
            g.drawRoundedRectangle((float)x, (float)y, (float)cw, (float)ch, 4.0f, i == selectedClipIndex ? 1.8f : 1.1f);

            // Tiradores visuales de recorte en ambos bordes
            g.setColour(juce::Colours::white.withAlpha(0.65f));
            g.fillRect(x + 1, y + 4, 2, ch - 8);
            g.fillRect(x + cw - 3, y + 4, 2, ch - 8);

            g.saveState();
            g.reduceClipRegion(x + 2, y + 1, juce::jmax(1, cw - 4), juce::jmax(1, ch - 2));

            if (!c.isMidi && !c.waveformPeaks.empty() && c.maxDurationSec > 0.0)
            {
                // Forma de onda real respetando el recorte (sourceOffsetSec .. sourceOffsetSec + durationSec)
                g.setColour(juce::Colours::aquamarine.withAlpha(0.9f));
                float midY = y + ch * 0.5f;
                for (int px = 0; px < cw; ++px)
                {
                    double tRel = c.sourceOffsetSec + ((double)px / pixelsPerSecond);
                    int peakIdx = (int)((tRel / c.maxDurationSec) * (double)c.waveformPeaks.size());
                    if (peakIdx >= 0 && peakIdx < (int)c.waveformPeaks.size())
                    {
                        float amp = juce::jlimit(0.05f, 1.0f, c.waveformPeaks[peakIdx]) * (ch * 0.44f);
                        g.drawVerticalLine(x + px, midY - amp, midY + amp);
                    }
                }
            }
            else if (c.isMidi && !c.midiNotes.empty())
            {
                // Mini Piano Roll con notas MIDI respetando el recorte
                g.setColour(juce::Colours::gold.withAlpha(0.92f));
                for (const auto& n : c.midiNotes)
                {
                    double nStart = n.startRel - c.sourceOffsetSec;
                    if (nStart + n.dur < 0.0 || nStart > c.durationSec) continue;
                    int nx = x + (int)(nStart * pixelsPerSecond);
                    int nw = juce::jmax(3, (int)(n.dur * pixelsPerSecond));
                    float normPitch = juce::jlimit(0.0f, 1.0f, (n.note - 36) / 48.0f);
                    int ny = y + ch - 4 - (int)(normPitch * (ch - 8));
                    g.fillRect(nx, ny, nw, 3);
                }
            }

            g.setColour(juce::Colours::white);
            g.setFont(9.5f);
            g.drawText(c.name, x + 5, y + 1, cw - 8, 12, juce::Justification::centredLeft);
            g.restoreState();
        }

        // Cabezal de reproduccion (Playhead)
        int phX = headerW + (int)(playheadSec * pixelsPerSecond);
        if (phX >= headerW && phX <= w)
        {
            g.setColour(juce::Colours::orangered);
            g.drawVerticalLine(phX, 0.0f, (float)h);
        }
    }

    void mouseMove(const juce::MouseEvent& e) override
    {
        int hitClip = -1, edge = 0;
        hitTestClipEdge(e.x, e.y, hitClip, edge);
        setMouseCursor(edge != 0 ? juce::MouseCursor::LeftRightResizeCursor : juce::MouseCursor::NormalCursor);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        int hitClip = -1, edge = 0;
        hitTestClipEdge(e.x, e.y, hitClip, edge);
        selectedClipIndex = hitClip;
        dragClipIndex = hitClip;
        dragEdgeMode = edge; // -1 = borde izq, 1 = borde der, 0 = mover clip
        dragStartX = e.x;
        if (dragClipIndex >= 0)
            dragInitialClip = clips[dragClipIndex];
        repaint();
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (dragClipIndex < 0 || dragClipIndex >= (int)clips.size()) return;
        double deltaSec = (e.x - dragStartX) / pixelsPerSecond;
        auto& c = clips[dragClipIndex];

        if (dragEdgeMode == -1) // Recorte por borde izquierdo
        {
            double maxTrim = dragInitialClip.durationSec - 0.15;
            double minTrim = -dragInitialClip.sourceOffsetSec;
            double trim = juce::jlimit(minTrim, maxTrim, deltaSec);
            c.startSec = dragInitialClip.startSec + trim;
            c.sourceOffsetSec = dragInitialClip.sourceOffsetSec + trim;
            c.durationSec = dragInitialClip.durationSec - trim;
        }
        else if (dragEdgeMode == 1) // Recorte por borde derecho
        {
            double maxDur = dragInitialClip.maxDurationSec - dragInitialClip.sourceOffsetSec;
            c.durationSec = juce::jlimit(0.15, juce::jmax(0.15, maxDur), dragInitialClip.durationSec + deltaSec);
        }
        else // Mover clip en el tiempo
        {
            c.startSec = juce::jmax(0.0, dragInitialClip.startSec + deltaSec);
        }
        repaint();
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        dragClipIndex = -1;
        dragEdgeMode = 0;
    }

private:
    void hitTestClipEdge(int mx, int my, int& outClip, int& outEdge)
    {
        outClip = -1; outEdge = 0;
        int headerW = 92, rulerH = 22;
        int numTracks = juce::jmax(1, trackNames.size());
        int trackH = juce::jlimit(26, 54, (getHeight() - rulerH) / numTracks);

        for (int i = (int)clips.size() - 1; i >= 0; --i)
        {
            const auto& c = clips[i];
            int y = rulerH + c.trackIndex * trackH + 2;
            int ch = trackH - 4;
            int x = headerW + (int)(c.startSec * pixelsPerSecond);
            int cw = juce::jmax(8, (int)(c.durationSec * pixelsPerSecond));

            if (mx >= x - 4 && mx <= x + cw + 4 && my >= y && my <= y + ch)
            {
                outClip = i;
                if (std::abs(mx - x) <= 6) outEdge = -1;
                else if (std::abs(mx - (x + cw)) <= 6) outEdge = 1;
                else outEdge = 0;
                return;
            }
        }
    }

    int selectedClipIndex = -1;
    int dragClipIndex = -1;
    int dragEdgeMode = 0;
    int dragStartX = 0;
    TimelineClip dragInitialClip;
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
    int metroBeatIndex = 0;

    int metroBeepSamplesLeft = 0;
    // --- SISTEMA DE TEMAS (CLARO / OSCURO / MIDNIGHT) ---
    struct DawTheme {
        juce::Colour bgMain;
        juce::Colour bgBar;
        juce::Colour bgCapsule;
        juce::Colour bgPanel;
        juce::Colour borderSubtle;
        juce::Colour textPrimary;
        juce::Colour accentPrimary;
    };

    DawTheme currentTheme;
    juce::ComboBox themeSelector;
    void applyTheme(int themeId);

    double metroPhase = 0.0;
    // Instrumento Virtual Nativo + Teclado en vivo
    juce::Synthesiser nativeSynth;
    juce::CriticalSection midiControlLock;
    juce::MidiBuffer pendingControlMidi;

    juce::MidiKeyboardState keyboardState;
    juce::MidiKeyboardComponent midiKeyboard{ keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard };
    juce::ComboBox synthPresetBox;
    // Browser estilo Ableton (Izquierda) + Host de Plugins Externos VST3
    juce::TextEditor browserSearchBox;
    juce::TextButton catAllBtn{ "Todos" };
    juce::TextButton catPianoBtn{ "Pianos" };
    juce::TextButton catSynthBtn{ "Synths" };
    juce::TextButton catDrumBtn{ "Drums" };
    juce::TextButton catStringsBtn{ "Cuerdas" };
    
    juce::TextButton catVstBtn{ "VST3" };
    juce::TextButton scanVstBtn{ "Scan VST3" };
    juce::TextButton openGuiBtn{ "Abrir UI" };
    juce::TextButton unloadVstBtn{ "X" };

    int browserCategory = 0; // 0 = Todos, 1 = Pianos, 2 = Synths, 3 = Drums, 4 = VST3
    int openFolder = 3; // Carpeta desplegada por defecto (3 = Bajo y Guitarras; -1 = todas cerradas)


    juce::StringArray browserItemNames;
    juce::Array<int>  browserItemTypes; // 1..3 = Nativo, 100+ = Indice en knownPlugins
    int selectedBrowserRow = -1;

    struct BrowserListModel : public juce::ListBoxModel
    {
        MainComponent* owner = nullptr;
        int getNumRows() override;
        void paintListBoxItem(int row, juce::Graphics& g, int width, int height, bool selected) override;
        void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override;
        void selectedRowsChanged(int lastRowSelected) override;
    };
    BrowserListModel browserModel;
    juce::ListBox browserListBox{ "BrowserList", &browserModel };

    juce::AudioPluginFormatManager pluginFormatManager;
    juce::KnownPluginList knownPlugins;
    std::unique_ptr<juce::AudioPluginInstance> activeVstPlugin;
    std::unique_ptr<PluginEditorWindow> activePluginWindow;
    juce::String loadedPluginName{ "Ninguno" };

    void rebuildBrowserList();
    void loadBrowserItem(int row);
    void scanForVst3Plugins();






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
    class AudioSetupOverlay : public juce::Component
    {
    public:
        AudioSetupOverlay(juce::AudioDeviceManager& dm)
        {
            selector = std::make_unique<juce::AudioDeviceSelectorComponent>(dm, 1, 8, 1, 8, true, true, true, false);


            addAndMakeVisible(*selector);

            addAndMakeVisible(closeBtn);
            closeBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::crimson);
            closeBtn.onClick = [this]()
                {
                    if (onClose) onClose();
                };
        }

        void paint(juce::Graphics& g) override
        {
            g.fillAll(juce::Colour(0xff141b22));

            g.setColour(juce::Colour(0xff1e2933));
            g.fillRect(0, 0, getWidth(), 34);

            g.setColour(juce::Colours::deepskyblue);
            g.drawRect(0, 0, getWidth(), getHeight(), 2);
            g.drawHorizontalLine(34, 0.0f, (float)getWidth());

            g.setColour(juce::Colours::white);
            g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
            g.drawText("CONFIGURACION DE AUDIO Y MIDI", 14, 0, getWidth() - 60, 34, juce::Justification::centredLeft);
        }

        void resized() override
        {
            closeBtn.setBounds(getWidth() - 34, 5, 26, 24);
            selector->setBounds(24, 42, getWidth() - 38, getHeight() - 50);
        }

        std::unique_ptr<juce::AudioDeviceSelectorComponent> selector;
        juce::TextButton closeBtn{ "X" };
        std::function<void()> onClose;
    };

    std::unique_ptr<AudioSetupOverlay> audioSetupComp;


    bool isPlaying = false;
    bool showAudioSetup = false;
    float generateWave(double phase, int waveType);
    // === MOTOR UNIVERSAL DE FADERS MOTORIZADOS (MCU 14-bit + CC Universal) ===
    std::unique_ptr<juce::MidiOutput> motorizedMidiOut;
    juce::String motorizedOutDeviceId;
    int motorizedFaderMode = 0; // 0: Dual Universal (MCU + CC), 1: Solo MCU 14-bit, 2: Solo CC 7-bit
    bool faderTouched[16] = { false };
    int lastSentFaderValue[16] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };

    void openMotorizedOutput(const juce::String& deviceIdentifier);
    void sendMotorizedFaderFeedback(int faderIndex, float normalizedValue);
    void sendAllMotorizedFaders();
    bool processMotorizedSurfaceInput(const juce::MidiMessage& message);
    // === MOTOR UNIVERSAL DE LUCES DMX POR MIDI (512 CANALES + 16 ESCENAS) ===
    std::unique_ptr<juce::MidiOutput> dmxMidiOut;
    juce::String dmxOutDeviceId;
    int dmxMidiChannel = 16; // Canal MIDI dedicado por defecto para luces (Ch 16)
    uint8_t dmxUniverse[512] = { 0 };
    uint8_t dmxScenes[16][512] = { { 0 } };
    int activeDmxScene = -1;
    bool dmxBlackout = false;

    void openDmxOutput(const juce::String& deviceIdentifier);
    void setDmxChannelValue(int dmxChannel1To512, uint8_t value0To255);
    void saveDmxScene(int sceneIndex);
    void recallDmxScene(int sceneIndex);
    void triggerDmxBlackout(bool enableBlackout);
    bool processDmxMidiTrigger(const juce::MidiMessage& message);
    ArrangementTimeline arrangementTimeline;
    juce::TextButton timelineToggleBtn{ "TIMELINE" };
    bool showTimelineView = true;

    int selectedMidiTrackIndex = 0;

}
;


  