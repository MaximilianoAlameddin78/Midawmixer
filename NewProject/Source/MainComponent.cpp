#include "MainComponent.h"

MainComponent::MainComponent() : AudioAppComponent()
{
    startTimer(16);
    setAudioChannels(0, 2);

    // Registrar este componente como receptor global de mensajes MIDI
    deviceManager.addMidiInputDeviceCallback({}, this);

    // ================= CANAL 1 =================
    addAndMakeVisible(volumeSlider1);
    volumeSlider1.setSliderStyle(juce::Slider::LinearVertical);
    volumeSlider1.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 18);
    volumeSlider1.setRange(0.0, 1.0, 0.01);
    volumeSlider1.setValue(0.5);
    volumeSlider1.setColour(juce::Slider::thumbColourId, juce::Colours::orangered);
    volumeSlider1.setColour(juce::Slider::trackColourId, juce::Colours::white);

    addAndMakeVisible(freqSlider1);
    freqSlider1.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    freqSlider1.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 16);
    freqSlider1.setRange(50.0, 2000.0, 1.0); // Rango ampliado para cubrir todas las octavas del KeyStep
    freqSlider1.setValue(440.0);
    freqSlider1.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::crimson);
    freqSlider1.setColour(juce::Slider::thumbColourId, juce::Colours::white);

    addAndMakeVisible(panSlider1);
    panSlider1.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    panSlider1.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 16);
    panSlider1.setRange(0.0, 1.0, 0.01);
    panSlider1.setValue(0.5);
    panSlider1.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::dodgerblue);
    panSlider1.setColour(juce::Slider::thumbColourId, juce::Colours::white);

    // ================= CANAL 2 =================
    addAndMakeVisible(volumeSlider2);
    volumeSlider2.setSliderStyle(juce::Slider::LinearVertical);
    volumeSlider2.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 18);
    volumeSlider2.setRange(0.0, 1.0, 0.01);
    volumeSlider2.setValue(0.5);
    volumeSlider2.setColour(juce::Slider::thumbColourId, juce::Colours::orangered);
    volumeSlider2.setColour(juce::Slider::trackColourId, juce::Colours::white);

    addAndMakeVisible(freqSlider2);
    freqSlider2.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    freqSlider2.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 16);
    freqSlider2.setRange(50.0, 2000.0, 1.0);
    freqSlider2.setValue(554.0);
    freqSlider2.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::crimson);
    freqSlider2.setColour(juce::Slider::thumbColourId, juce::Colours::white);

    addAndMakeVisible(panSlider2);
    panSlider2.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    panSlider2.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 16);
    panSlider2.setRange(0.0, 1.0, 0.01);
    panSlider2.setValue(0.5);
    panSlider2.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::dodgerblue);
    panSlider2.setColour(juce::Slider::thumbColourId, juce::Colours::white);

    // ================= TRANSPORTE Y SETUP =================
    addAndMakeVisible(playButton);
    addAndMakeVisible(stopButton);
    addAndMakeVisible(setupButton);

    playButton.setButtonText("PLAY");
    stopButton.setButtonText("STOP");
    setupButton.setButtonText("AUDIO SETUP");

    playButton.setColour(juce::TextButton::buttonColourId, juce::Colours::forestgreen);
    stopButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkred);
    setupButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a2a2a));

    playButton.onClick = [this] { isPlaying = true; };
    stopButton.onClick = [this] { isPlaying = false; };
    setupButton.onClick = [this]
        {
            showAudioSetup = !showAudioSetup;
            audioSetupComp->setVisible(showAudioSetup);
            resized();
        };

    isPlaying = false;

    audioSetupComp = std::make_unique<juce::AudioDeviceSelectorComponent>(deviceManager, 0, 0, 0, 2, true, false, true, false);
    addChildComponent(*audioSetupComp);

    setSize(900, 620);
}

MainComponent::~MainComponent()
{
    deviceManager.removeMidiInputDeviceCallback({}, this);
    shutdownAudio();
}

void MainComponent::handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message)
{
    // Pasamos los mensajes al hilo gráfico de forma segura para mover perillas y faders
    juce::MessageManager::callAsync([this, message]()
        {
            // 1. TECLADO (Arturia KeyStep 32): Tocar una nota cambia la frecuencia y enciende el sonido
            if (message.isNoteOn())
            {
                double noteFreq = message.getMidiNoteInHertz(message.getNoteNumber());
                freqSlider1.setValue(noteFreq, juce::dontSendNotification);
                freqSlider2.setValue(noteFreq * 1.5, juce::dontSendNotification); // El Canal 2 armoniza una 5ta arriba
                isPlaying = true;
            }
            else if (message.isNoteOff())
            {
                isPlaying = false;
            }
            // 2. FADERS FÍSICOS (Akai APC mini mk2 envía CC 48 en Fader 1 y CC 49 en Fader 2)
            else if (message.isController())
            {
                int ccNumber = message.getControllerNumber();
                float normalizedValue = message.getControllerValue() / 127.0f;

                if (ccNumber == 48)
                    volumeSlider1.setValue(normalizedValue, juce::dontSendNotification);
                else if (ccNumber == 49)
                    volumeSlider2.setValue(normalizedValue, juce::dontSendNotification);
            }
        });
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    bufferToFill.clearActiveBufferRegion();

    float vol1 = (float)volumeSlider1.getValue();
    double freq1 = freqSlider1.getValue();
    float pan1 = (float)panSlider1.getValue();

    float vol2 = (float)volumeSlider2.getValue();
    double freq2 = freqSlider2.getValue();
    float pan2 = (float)panSlider2.getValue();

    auto* leftChannel = bufferToFill.buffer->getWritePointer(0, bufferToFill.startSample);
    auto* rightChannel = bufferToFill.buffer->getWritePointer(1, bufferToFill.startSample);

    double blindRate = 48000.0;
    phaseIncrement1 = freq1 / blindRate;
    phaseIncrement2 = freq2 / blindRate;

    float leftGain1 = vol1 * (1.0f - pan1);
    float rightGain1 = vol1 * pan1;

    float leftGain2 = vol2 * (1.0f - pan2);
    float rightGain2 = vol2 * pan2;

    float maxLeft = 0.0f;
    float maxRight = 0.0f;

    for (auto sampleIndex = 0; sampleIndex < bufferToFill.numSamples; ++sampleIndex)
    {
        float outLeft = 0.0f;
        float outRight = 0.0f;

        if (isPlaying)
        {
            float wave1 = (float)std::sin(phase1 * 2.0 * 3.14159265359);
            float wave2 = (float)std::sin(phase2 * 2.0 * 3.14159265359);

            outLeft = ((wave1 * leftGain1) + (wave2 * leftGain2)) * 0.5f;
            outRight = ((wave1 * rightGain1) + (wave2 * rightGain2)) * 0.5f;
        }

        if (leftChannel != nullptr) leftChannel[sampleIndex] = outLeft;
        if (rightChannel != nullptr) rightChannel[sampleIndex] = outRight;

        // Medir picos máximos para alimentar el Vúmetro LED
        maxLeft = std::max(maxLeft, std::abs(outLeft));
        maxRight = std::max(maxRight, std::abs(outRight));

        phase1 += phaseIncrement1;
        if (phase1 >= 1.0) phase1 -= 1.0;

        phase2 += phaseIncrement2;
        if (phase2 >= 1.0) phase2 -= 1.0;
    }

    // Suavizar la caída de los LEDs del Vúmetro
    currentLevelLeft = std::max(maxLeft, currentLevelLeft * 0.85f);
    currentLevelRight = std::max(maxRight, currentLevelRight * 0.85f);
}

void MainComponent::releaseResources()
{}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff141414));

    g.setColour(juce::Colour(0xff1f1f1f));
    g.fillRect(0, 0, getWidth(), 50);
    g.setColour(juce::Colours::white.withAlpha(0.2f));
    g.drawLine(0, 50, (float)getWidth(), 50, 1.0f);

    int chY = 65;
    int chW = 85;
    int chH = getHeight() - 80;

    // 1. Dibujar los 2 Canales
    for (int ch = 0; ch < 2; ++ch)
    {
        int chX = 20 + (ch * (chW + 8));

        g.setColour(juce::Colour(0xff1c1c1e));
        g.fillRect(chX, chY, chW, chH);

        g.setColour(juce::Colours::white.withAlpha(0.75f));
        g.drawRect(chX, chY, chW, chH, 1);

        g.setColour(juce::Colours::white);
        g.fillRect(chX, chY, chW, 22);
        g.setColour(juce::Colours::black);
        g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
        g.drawText("CH " + juce::String(ch + 1) + " - OSC", chX, chY, chW, 22, juce::Justification::centred);

        g.setColour(juce::Colours::lightgrey);
        g.setFont(10.0f);
        g.drawText("FREQ (Hz)", chX, chY + 28, chW, 14, juce::Justification::centred);
        g.drawText("PAN (L/R)", chX, chY + 123, chW, 14, juce::Justification::centred);
        g.drawText("LEVEL", chX, chY + 218, chW, 14, juce::Justification::centred);

        g.setColour(juce::Colours::white.withAlpha(0.4f));
        int faderTop = chY + 245;
        int faderBottom = chY + chH - 35;
        for (int i = 0; i <= 10; ++i)
        {
            int yPos = faderTop + (i * (faderBottom - faderTop) / 10);
            g.drawLine((float)(chX + 12), (float)yPos, (float)(chX + 22), (float)yPos, 1.0f);
            g.drawLine((float)(chX + chW - 22), (float)yPos, (float)(chX + chW - 12), (float)yPos, 1.0f);
        }

        g.setColour(juce::Colours::white);
        g.fillRect(chX, chY + chH - 20, chW, 20);
        g.setColour(juce::Colours::black);
        g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
        g.drawText(juce::String(ch + 1), chX, chY + chH - 20, chW, 20, juce::Justification::centred);
    }

    // 2. Dibujar la Sección Master con el Vúmetro LED Estéreo (Estilo Apogee)
    int vuX = 215;
    int vuW = 65;
    g.setColour(juce::Colour(0xff1c1c1e));
    g.setColour(juce::Colours::white.withAlpha(0.75f));
    g.drawRect(vuX, chY, vuW, chH, 1);

    g.setColour(juce::Colours::orangered);
    g.fillRect(vuX, chY, vuW, 22);
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    g.drawText("MASTER", vuX, chY, vuW, 22, juce::Justification::centred);

    g.setColour(juce::Colours::lightgrey);
    g.setFont(10.0f);
    g.drawText("L      R", vuX, chY + 28, vuW, 14, juce::Justification::centred);

    // Dibujar los 20 segmentos LED por canal (Verde, Amarillo y Rojo)
    int numLeds = 20;
    int ledAreaTop = chY + 48;
    int ledAreaHeight = chH - 80;
    int ledHeight = (ledAreaHeight / numLeds) - 3;

    for (int i = 0; i < numLeds; ++i)
    {
        float threshold = (float)(i + 1) / (float)numLeds;
        int ledY = ledAreaTop + ledAreaHeight - ((i + 1) * (ledAreaHeight / numLeds));

        juce::Colour ledOnColor = juce::Colours::limegreen;
        if (i >= 16) ledOnColor = juce::Colours::red;
        else if (i >= 12) ledOnColor = juce::Colours::yellow;

        bool leftLit = (currentLevelLeft * 1.8f) >= threshold;
        g.setColour(leftLit ? ledOnColor : ledOnColor.withAlpha(0.15f));
        g.fillRect(vuX + 14, ledY, 14, ledHeight);

        bool rightLit = (currentLevelRight * 1.8f) >= threshold;
        g.setColour(rightLit ? ledOnColor : ledOnColor.withAlpha(0.15f));
        g.fillRect(vuX + 37, ledY, 14, ledHeight);
    }
}

void MainComponent::resized()
{
    playButton.setBounds(20, 10, 75, 30);
    stopButton.setBounds(105, 10, 75, 30);
    setupButton.setBounds(getWidth() - 130, 10, 110, 30);

    int chY = 65;
    int chW = 85;
    int chH = getHeight() - 80;

    int ch1X = 20;
    freqSlider1.setBounds(ch1X + 7, chY + 42, chW - 14, 75);
    panSlider1.setBounds(ch1X + 7, chY + 137, chW - 14, 75);
    volumeSlider1.setBounds(ch1X + 15, chY + 235, chW - 30, chH - 260);

    int ch2X = 20 + (chW + 8);
    freqSlider2.setBounds(ch2X + 7, chY + 42, chW - 14, 75);
    panSlider2.setBounds(ch2X + 7, chY + 137, chW - 14, 75);
    volumeSlider2.setBounds(ch2X + 15, chY + 235, chW - 30, chH - 260);

    if (audioSetupComp != nullptr && showAudioSetup)
        audioSetupComp->setBounds(295, 65, getWidth() - 315, getHeight() - 80);
}

void MainComponent::timerCallback()
{
    repaint();
}

void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{}

