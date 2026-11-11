#include "MainComponent.h"

MainComponent::MainComponent() : AudioAppComponent()
{
    startTimer(20);
    menuBar = std::make_unique<juce::MenuBarComponent>(this);
    addAndMakeVisible(*menuBar);
    formatManager.registerBasicFormats();

    setAudioChannels(0, 2);
    for (const auto& input : juce::MidiInput::getAvailableDevices())
    {
        deviceManager.setMidiInputDeviceEnabled(input.identifier, true);
        deviceManager.addMidiInputDeviceCallback(input.identifier, this);
    }


    delayBufferLeft.resize(96000, 0.0f);
    delayBufferRight.resize(96000, 0.0f);

    addAndMakeVisible(channelViewport);
    channelViewport.setViewedComponent(&channelContainer, false);
    channelViewport.setScrollBarsShown(false, true);

    addAudioChannel(440.0);
    addAudioChannel(554.0);
    addMidiChannel();
    addMidiChannel();

    auto setupFxKnob = [this](juce::Slider& knob, double minVal, double maxVal, double defVal, juce::Colour color)
        {
            addAndMakeVisible(knob);
            knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            knob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 48, 15);
            knob.setRange(minVal, maxVal, 0.01);
            knob.setValue(defVal);
            knob.setColour(juce::Slider::rotarySliderFillColourId, color);
            knob.setColour(juce::Slider::thumbColourId, juce::Colours::white);
        };

    setupFxKnob(reverbSizeSlider, 0.0, 1.0, 0.5, juce::Colours::mediumpurple);
    setupFxKnob(reverbMixSlider, 0.0, 1.0, 0.0, juce::Colours::mediumpurple);
    setupFxKnob(delayTimeSlider, 0.05, 1.0, 0.35, juce::Colours::mediumspringgreen);
    setupFxKnob(delayFeedbackSlider, 0.0, 0.85, 0.0, juce::Colours::mediumspringgreen);

    addAndMakeVisible(masterVolumeSlider);
    masterVolumeSlider.setSliderStyle(juce::Slider::LinearVertical);
    masterVolumeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 16);
    masterVolumeSlider.setRange(0.0, 1.0, 0.01);
    masterVolumeSlider.setValue(0.8);
    masterVolumeSlider.setColour(juce::Slider::thumbColourId, juce::Colours::gold);
    masterVolumeSlider.setColour(juce::Slider::trackColourId, juce::Colours::white);

    addAndMakeVisible(playButton);
    addAndMakeVisible(stopButton);
    addAndMakeVisible(recWavButton);
    addAndMakeVisible(addAudioBtn);
    addAndMakeVisible(addMidiBtn);
    addAndMakeVisible(saveBtn);
    addAndMakeVisible(loadBtn);
    

    playButton.setColour(juce::TextButton::buttonColourId, juce::Colours::forestgreen);
    stopButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkred);
    recWavButton.setButtonText("REC");
    recWavButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff4a1515));
    recWavButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::red);
    recWavButton.setClickingTogglesState(true);

    addAudioBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff3a3a3c));
    addMidiBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::deepskyblue.darker(0.4f));
    saveBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2b4c3f));
    loadBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2b3d4c));
    setupButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a2a2a));
    addAndMakeVisible(bpmSlider);
    bpmSlider.setSliderStyle(juce::Slider::LinearBar);
    bpmSlider.setRange(60.0, 200.0, 1.0);
    bpmSlider.setValue(120.0);
    bpmSlider.setTextValueSuffix(" BPM");
    bpmSlider.setColour(juce::Slider::trackColourId, juce::Colour(0xff2b4c3f));

    addAndMakeVisible(metronomeBtn);
    metronomeBtn.setClickingTogglesState(true);
    metronomeBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a2a2a));
    metronomeBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colours::darkorange);

    for (int i = 0; i < 8; ++i)
        nativeSynth.addVoice(new MidawSynthVoice());
    nativeSynth.addSound(new MidawSynthSound());

    addAndMakeVisible(synthPresetBox);
    synthPresetBox.addItem("Pad Suave", 1);
    synthPresetBox.addItem("E-Piano", 2);
    synthPresetBox.addItem("Synth Lead", 3);
    synthPresetBox.setSelectedId(2);
    synthPresetBox.onChange = [this]
        {
            int id = synthPresetBox.getSelectedId();
            for (int i = 0; i < nativeSynth.getNumVoices(); ++i)
                if (auto* v = dynamic_cast<MidawSynthVoice*>(nativeSynth.getVoice(i)))
                    v->presetType = id;
        };

    addAndMakeVisible(midiKeyboard);

    synthPresetBox.setVisible(false);

    

    browserModel.owner = this;

    addAndMakeVisible(browserSearchBox);
    browserSearchBox.setTextToShowWhenEmpty("Buscar sonido o VST3...", juce::Colours::grey);
    browserSearchBox.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff181818));
    browserSearchBox.onTextChange = [this] { rebuildBrowserList(); };

    auto setupCatBtn = [this](juce::TextButton& b, int catId)
        {
            addAndMakeVisible(b);
            b.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff232323));
            b.onClick = [this, catId]
                {
                    browserCategory = catId;
                    rebuildBrowserList();
                };
        };
    setupCatBtn(catAllBtn, 0);
    setupCatBtn(catNativeBtn, 1);
    setupCatBtn(catPluginsBtn, 2);

    addAndMakeVisible(scanVstBtn);
    scanVstBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2b4c3f));
    scanVstBtn.onClick = [this] { scanForVst3Plugins(); };

    addAndMakeVisible(openGuiBtn);
    openGuiBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2d4a53));
    openGuiBtn.onClick = [this]
        {
            if (activeVstPlugin != nullptr && activeVstPlugin->hasEditor())
            {
                if (activePluginWindow == nullptr)
                    activePluginWindow = std::make_unique<PluginEditorWindow>(loadedPluginName, activeVstPlugin->createEditorIfNeeded());
                activePluginWindow->setVisible(true);
                activePluginWindow->toFront(true);
            }
        };

    addAndMakeVisible(unloadVstBtn);
    unloadVstBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff4a1c1c));
    unloadVstBtn.onClick = [this]
        {
            activePluginWindow.reset();
            activeVstPlugin.reset();
            loadedPluginName = "Ninguno";
            repaint();
        };

    addAndMakeVisible(browserListBox);
    browserListBox.setRowHeight(24);
    browserListBox.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff161616));

    rebuildBrowserList();




    playButton.onClick = [this] { isPlaying = true; };
    stopButton.onClick = [this] { isPlaying = false; };
    addAudioBtn.onClick = [this] { addAudioChannel(659.0); };
    addMidiBtn.onClick = [this] { addMidiChannel(); };
    saveBtn.onClick = [this] { saveSessionToFile(); };
    loadBtn.onClick = [this] { loadSessionFromFile(); };

    recWavButton.onClick = [this]
        {
            if (recWavButton.getToggleState())
                startMasterRecording();
            else
                stopMasterRecording();
        };

    setupButton.onClick = [this]
        {
            showAudioSetup = !showAudioSetup;
            audioSetupComp->setVisible(showAudioSetup);
            for (auto* m : midiChannels) m->refreshDevices();
            resized();
        };

    audioSetupComp = std::make_unique<juce::AudioDeviceSelectorComponent>(deviceManager, 0, 0, 0, 2, true, true, true, false);
    addChildComponent(*audioSetupComp);

    setSize(1180, 680); 
}

MainComponent::~MainComponent()
{
    menuBar.reset();
    deviceManager.removeMidiInputDeviceCallback({}, this);
    shutdownAudio();
}

void MainComponent::startMasterRecording()
{
    recordedAudioBuffer.setSize(2, 48000 * 60 * 3);
    recordedAudioBuffer.clear();
    recordedSamplesCount = 0;

    recordedMidiSeq.clear();
    masterRecStartTime = juce::Time::getMillisecondCounterHiRes() * 0.001;

    isRecordingMaster = true;
    recWavButton.setButtonText("REC...");
}

void MainComponent::stopMasterRecording()
{
    if (!isRecordingMaster) return;
    isRecordingMaster = false;
    recWavButton.setButtonText("REC");
    recWavButton.setToggleState(false, juce::dontSendNotification);

    auto docsDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);

    if (recordedSamplesCount > 0)
    {
        auto outWav = docsDir.getNonexistentChildFile("Midawmixer_Session", ".wav");
        if (auto fileStream = std::unique_ptr<juce::FileOutputStream>(outWav.createOutputStream()))
        {
            juce::WavAudioFormat wavFormat;
            if (auto writer = std::unique_ptr<juce::AudioFormatWriter>(wavFormat.createWriterFor(fileStream.get(), 48000.0, 2, 24, {}, 0)))
            {
                fileStream.release();
                writer->writeFromAudioSampleBuffer(recordedAudioBuffer, 0, recordedSamplesCount);
            }
        }
    }

    if (recordedMidiSeq.getNumEvents() > 0)
    {
        recordedMidiSeq.updateMatchedPairs();
        juce::MidiFile midiFile;
        midiFile.setTicksPerQuarterNote(960);
        midiFile.addTrack(recordedMidiSeq);

        auto outMid = docsDir.getNonexistentChildFile("Midawmixer_Session", ".mid");
        if (auto midiStream = std::unique_ptr<juce::FileOutputStream>(outMid.createOutputStream()))
        {
            midiFile.writeTo(*midiStream);
        }
    }
}

void MainComponent::saveSessionToFile()
{
    fileChooser = std::make_unique<juce::FileChooser>("Guardar Proyecto Midawmixer",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
        "*.xml");
    auto flags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles;
    fileChooser->launchAsync(flags, [this](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file == juce::File{}) return;

            juce::XmlElement root("MidawmixerSession");
            root.setAttribute("masterVol", masterVolumeSlider.getValue());
            root.setAttribute("revSize", reverbSizeSlider.getValue());
            root.setAttribute("revMix", reverbMixSlider.getValue());
            root.setAttribute("delTime", delayTimeSlider.getValue());
            root.setAttribute("delFeed", delayFeedbackSlider.getValue());

            for (auto* ch : audioChannels)
            {
                auto* chXml = root.createNewChildElement("AudioChannel");
                chXml->setAttribute("wave", ch->waveSelector.getSelectedId());
                chXml->setAttribute("freq", ch->freqSlider.getValue());
                chXml->setAttribute("pan", ch->panSlider.getValue());
                chXml->setAttribute("vol", ch->volumeSlider.getValue());
                chXml->setAttribute("low", ch->eqLowSlider.getValue());
                chXml->setAttribute("mid", ch->eqMidSlider.getValue());
                chXml->setAttribute("high", ch->eqHighSlider.getValue());
            }

            root.writeTo(file.withFileExtension(".xml"));
        });
}

void MainComponent::loadSessionFromFile()
{
    fileChooser = std::make_unique<juce::FileChooser>("Cargar Proyecto Midawmixer",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
        "*.xml");
    auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
    fileChooser->launchAsync(flags, [this](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file == juce::File{}) return;

            if (auto xml = juce::XmlDocument::parse(file))
            {
                masterVolumeSlider.setValue(xml->getDoubleAttribute("masterVol", 0.8));
                reverbSizeSlider.setValue(xml->getDoubleAttribute("revSize", 0.5));
                reverbMixSlider.setValue(xml->getDoubleAttribute("revMix", 0.0));
                delayTimeSlider.setValue(xml->getDoubleAttribute("delTime", 0.35));
                delayFeedbackSlider.setValue(xml->getDoubleAttribute("delFeed", 0.0));

                audioChannels.clear();
                for (auto* chXml : xml->getChildWithTagNameIterator("AudioChannel"))
                {
                    addAudioChannel(chXml->getDoubleAttribute("freq", 440.0));
                    auto* ch = audioChannels.getLast();
                    ch->waveSelector.setSelectedId(chXml->getIntAttribute("wave", 1));
                    ch->panSlider.setValue(chXml->getDoubleAttribute("pan", 0.5));
                    ch->volumeSlider.setValue(chXml->getDoubleAttribute("vol", 0.6));
                    ch->eqLowSlider.setValue(chXml->getDoubleAttribute("low", 0.0));
                    ch->eqMidSlider.setValue(chXml->getDoubleAttribute("mid", 0.0));
                    ch->eqHighSlider.setValue(chXml->getDoubleAttribute("high", 0.0));
                }
                layoutDynamicChannels();
                

            }
        });
}
void MainComponent::newProject()
{
    isPlaying = false;
    stopMasterRecording();
    masterVolumeSlider.setValue(0.8);
    reverbSizeSlider.setValue(0.5);
    reverbMixSlider.setValue(0.0);
    delayTimeSlider.setValue(0.35);
    delayFeedbackSlider.setValue(0.0);

    audioChannels.clear();
    midiChannels.clear();
    addAudioChannel(440.0);
    addAudioChannel(554.0);
    addMidiChannel();
    addMidiChannel();
    layoutDynamicChannels();
}

juce::StringArray MainComponent::getMenuBarNames()
{
    return { "Archivo", "Canales", "Configuracion", "Ayuda" };
}

juce::PopupMenu MainComponent::getMenuForIndex(int topLevelMenuIndex, const juce::String&)
{
    juce::PopupMenu menu;

    if (topLevelMenuIndex == 0)
    {
        menu.addItem(1, "Nuevo Proyecto");
        menu.addSeparator();
        menu.addItem(2, "Abrir Proyecto (.xml)...");
        menu.addItem(3, "Guardar Proyecto (.xml)...");
        menu.addSeparator();
        menu.addItem(4, isRecordingMaster ? "Detener Grabacion (REC)" : "Grabar Master (WAV + MIDI)");
        menu.addSeparator();
        menu.addItem(6, "Abrir Carpeta de Grabaciones...");

        menu.addItem(5, "Salir");
    }
    else if (topLevelMenuIndex == 1)
    {
        menu.addItem(10, "Agregar Canal de Audio (+ AUDIO)");
        menu.addItem(11, "Agregar Canal MIDI (+ MIDI)");
    }
    else if (topLevelMenuIndex == 2)
    {
        menu.addItem(20, "Configuracion de Audio y MIDI...");
    }
    else if (topLevelMenuIndex == 3)
    {
        menu.addItem(30, "Soporte y Guia Rapida");
        menu.addSeparator();
        menu.addItem(31, "Licencia y Terminos de Uso (Dual)...");

    }

    return menu;
}

void MainComponent::menuItemSelected(int menuItemID, int)
{
    switch (menuItemID)
    {
    case 1:  newProject(); break;
    case 2:  loadSessionFromFile(); break;
    case 3:  saveSessionToFile(); break;
    case 4:  if (isRecordingMaster) stopMasterRecording(); else { recWavButton.setToggleState(true, juce::dontSendNotification); startMasterRecording(); } break;
    case 5:  juce::JUCEApplication::getInstance()->systemRequestedQuit(); break;
#if ! JUCE_ANDROID && ! JUCE_IOS
    case 6:  juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).revealToUser(); break;
#endif

    case 10: addAudioChannel(659.0); break;
    case 11: addMidiChannel(); break;

    case 20:
        showAudioSetup = !showAudioSetup;
        audioSetupComp->setVisible(showAudioSetup);
        for (auto* m : midiChannels) m->refreshDevices();
        resized();
        break;

    case 30:
        juce::AlertWindow::showMessageBoxAsync(
            juce::MessageBoxIconType::InfoIcon,
            "Soporte Midawmixer v1.0",
            "Midawmixer Studio Console\n\n"
            "- Transport: PLAY / STOP / REC (exporta WAV + MIDI en Documentos).\n"
            "- Canales Audio: Oscilador + EQ 3 Bandas (HI/MID/LOW) + Pan + M/S.\n"
            "- Canales MIDI: Enrutamiento IN/OUT + Clip Looper en vivo.\n"
            "- Proyectos: Guarda y carga tus sesiones en formato XML desde Archivo.");
        break;
    case 31:
        juce::AlertWindow::showMessageBoxAsync(
            juce::MessageBoxIconType::InfoIcon,
            "Midawmixer - Terminos y Doble Licencia",
            "Copyright (c) 2026 Maxi - Todos los derechos reservados.\n\n"
            "Este software se distribuye bajo un esquema de DOBLE LICENCIA:\n\n"
            "1. LICENCIA SOFTWARE LIBRE (GNU AGPLv3):\n"
            "Libre para uso personal, educativo y proyectos de codigo abierto. "
            "Toda modificacion, distribucion o uso en red debe mantener esta misma "
            "licencia y publicar su codigo fuente completo.\n\n"
            "2. LICENCIA COMERCIAL EMPRESARIAL:\n"
            "Requerida para empresas o desarrolladores que deseen modificar, "
            "integrar o distribuir Midawmixer en productos comerciales de codigo "
            "cerrado sin las obligaciones de la AGPLv3.\n\n"
            "GARANTIA: El software se entrega 'TAL CUAL', sin garantias explicitas "
            "ni implicitas. El autor no se responsabiliza por danos o perdida de datos.");
        break;

    default: break;
    }
}

void MainComponent::addAudioChannel(double defaultFreq)
{
    int nextNum = audioChannels.size() + 1;
    auto* strip = audioChannels.add(new AudioChannelStrip(nextNum, defaultFreq));
    strip->loadAudioBtn.onClick = [this, strip] { loadSampleIntoChannel(strip); };
    channelContainer.addAndMakeVisible(strip);
    layoutDynamicChannels();
   
}

void MainComponent::loadSampleIntoChannel(AudioChannelStrip* strip)
{
    fileChooser = std::make_unique<juce::FileChooser>("Cargar Audio / Loop en Canal",
        juce::File::getSpecialLocation(juce::File::userMusicDirectory),
        "*.wav;*.mp3;*.aiff");
    auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
    fileChooser->launchAsync(flags, [this, strip](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file == juce::File{}) return;

            if (auto reader = std::unique_ptr<juce::AudioFormatReader>(formatManager.createReaderFor(file)))
            {
                strip->sampleBuffer.setSize((int)reader->numChannels, (int)reader->lengthInSamples);
                reader->read(&strip->sampleBuffer, 0, (int)reader->lengthInSamples, 0, true, true);
                strip->samplePlayPos = 0;
                strip->hasLoadedSample = true;
                strip->loadAudioBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::limegreen.darker());
            }
        });
}

void MainComponent::addMidiChannel()
{
    int nextNum = midiChannels.size() + 1;
    auto* strip = midiChannels.add(new MidiChannelStrip(nextNum, deviceManager));
    channelContainer.addAndMakeVisible(strip);
    layoutDynamicChannels();
}

void MainComponent::layoutDynamicChannels()
{
    int chW = 118;

    int chH = channelViewport.getHeight() - 15;
    if (chH < 400) chH = 580;

    int xPos = 0;
    for (auto* aStrip : audioChannels)
    {
        aStrip->setBounds(xPos, 0, chW, chH);
        xPos += chW + 8;
    }
    for (auto* mStrip : midiChannels)
    {
        mStrip->setBounds(xPos, 0, chW, chH);
        xPos += chW + 8;
    }
    channelContainer.setSize(std::max(xPos, channelViewport.getWidth()), chH);
}

float MainComponent::generateWave(double phase, int waveType)
{
    if (waveType == 2) return (phase < 0.5) ? 0.8f : -0.8f;
    if (waveType == 3) return (float)(2.0 * phase - 1.0) * 0.8f;
    return (float)std::sin(phase * 2.0 * 3.14159265359);
}

void MainComponent::triggerInternalSynths(const juce::MidiMessage& message, float velocityScale)
{
    juce::MessageManager::callAsync([this, message, velocityScale]()
        {
            if (message.isNoteOn())
            {
                double baseFreq = message.getMidiNoteInHertz(message.getNoteNumber());
                for (int i = 0; i < audioChannels.size(); ++i)
                {
                    double harmonicMult = (i == 0) ? 1.0 : (i == 1 ? 1.5 : 2.0);
                    audioChannels[i]->freqSlider.setValue(baseFreq * harmonicMult, juce::dontSendNotification);
                }
                isPlaying = true;
            }
            else if (message.isNoteOff())
            {
                isPlaying = false;
            }
            else if (message.isController())
            {
                int cc = message.getControllerNumber();
                float val = message.getControllerValue() / 127.0f;
                int faderIdx = cc - 48;
                if (faderIdx >= 0 && faderIdx < audioChannels.size())
                    audioChannels[faderIdx]->volumeSlider.setValue(val, juce::dontSendNotification);
                else if (cc == 56)
                    masterVolumeSlider.setValue(val, juce::dontSendNotification);
            }
        });
}

void MainComponent::handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message)
{
    if (source == nullptr) return;
    juce::String sourceId = source->getIdentifier();
    keyboardState.processNextMidiEvent(message);
    bool anyMidiSolo = false;
    for (auto* m : midiChannels)
        if (m->soloButton.getToggleState()) anyMidiSolo = true;

    for (auto* m : midiChannels)
    {
        bool isActive = anyMidiSolo ? m->soloButton.getToggleState() : !m->muteButton.getToggleState();

        if (m->assignedInId == sourceId && isActive)
        {
            m->hasActivity = true;

            if (isRecordingMaster && (message.isNoteOn() || message.isNoteOff()))
            {
                double ticks = ((juce::Time::getMillisecondCounterHiRes() * 0.001) - masterRecStartTime) * 1920.0;
                recordedMidiSeq.addEvent(message, ticks);
            }

            if (m->isRecording && (message.isNoteOn() || message.isNoteOff()))
            {
                double relTime = (juce::Time::getMillisecondCounterHiRes() * 0.001) - m->recordStartTime;
                m->recordedEvents.push_back({ relTime, message });
            }

            if (m->midiOut != nullptr)
            {
                juce::MidiMessage outMsg = message;
                if (outMsg.isNoteOn())
                    outMsg.setVelocity(outMsg.getFloatVelocity() * (float)m->velSlider.getValue());
                m->midiOut->sendMessageNow(outMsg);
            }

            triggerInternalSynths(message, (float)m->velSlider.getValue());
        }
    }
}

void MainComponent::timerCallback()
{
    double now = juce::Time::getMillisecondCounterHiRes() * 0.001;

    for (auto* m : midiChannels)
    {
        if (m->isLooping && !m->recordedEvents.empty() && !m->muteButton.getToggleState())
        {
            double elapsed = std::fmod(now - m->loopStartTime, m->loopDuration);
            if (elapsed < m->lastLoopCheckTime)
                m->lastLoopCheckTime = 0.0;

            for (const auto& ev : m->recordedEvents)
            {
                if (ev.timestampSeconds >= m->lastLoopCheckTime && ev.timestampSeconds < elapsed)
                {
                    m->hasActivity = true;
                    if (m->midiOut != nullptr)
                        m->midiOut->sendMessageNow(ev.message);
                    triggerInternalSynths(ev.message, (float)m->velSlider.getValue());
                }
            }
            m->lastLoopCheckTime = elapsed;
        }
    }

    repaint();
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    bufferToFill.clearActiveBufferRegion();
    juce::MidiBuffer incomingMidi;
    keyboardState.processNextMidiBuffer(incomingMidi, bufferToFill.startSample, bufferToFill.numSamples, true);
    nativeSynth.renderNextBlock(*bufferToFill.buffer, incomingMidi, bufferToFill.startSample, bufferToFill.numSamples);
    if (activeVstPlugin != nullptr)
        activeVstPlugin->processBlock(*bufferToFill.buffer, incomingMidi);




    float masterVol = (float)masterVolumeSlider.getValue();

    bool anySolo = false;
    for (auto* ch : audioChannels)
    {
        ch->updateEqFilters(48000.0);
        if (ch->soloButton.getToggleState()) anySolo = true;
    }

    auto* leftChannel = bufferToFill.buffer->getWritePointer(0, bufferToFill.startSample);
    auto* rightChannel = bufferToFill.buffer->getWritePointer(1, bufferToFill.startSample);

    double blindRate = 48000.0;
    int delaySamples = (int)(delayTimeSlider.getValue() * blindRate);
    float feedback = (float)delayFeedbackSlider.getValue();
    int bufferSize = (int)delayBufferLeft.size();

    int numCh = audioChannels.size();
    float mixScale = (numCh > 0) ? (1.0f / std::sqrt((float)numCh)) : 1.0f;

    for (auto sampleIndex = 0; sampleIndex < bufferToFill.numSamples; ++sampleIndex)
    {
        float dryLeft = leftChannel[sampleIndex];
        float dryRight = rightChannel[sampleIndex];


        if (isPlaying)
        {
            for (auto* ch : audioChannels)
            {
                bool active = anySolo ? ch->soloButton.getToggleState() : !ch->muteButton.getToggleState();
                float vol = active ? (float)ch->volumeSlider.getValue() : 0.0f;

                if (vol > 0.001f)
                {
                    float w = 0.0f;
                    if (ch->hasLoadedSample && ch->sampleBuffer.getNumSamples() > 0)
                    {
                        w = ch->sampleBuffer.getSample(0, ch->samplePlayPos);
                        ch->samplePlayPos = (ch->samplePlayPos + 1) % ch->sampleBuffer.getNumSamples();
                    }

                    w = ch->processSampleWithEq(w);
                    float pan = (float)ch->panSlider.getValue();
                    dryLeft += w * vol * (1.0f - pan);
                    dryRight += w * vol * pan;
                }

                

                

     
        

        float clickSample = 0.0f;
        if (isPlaying && metronomeBtn.getToggleState())
        {
            int samplesPerBeat = (int)((60.0 / bpmSlider.getValue()) * blindRate);
            if (metroSampleCounter == 0)
            {
                metroBeepSamplesLeft = (int)(0.035 * blindRate);
                metroPhase = 0.0;
            }
            metroSampleCounter = (metroSampleCounter + 1) % std::max(1, samplesPerBeat);

            if (metroBeepSamplesLeft > 0)
            {
                clickSample = (float)std::sin(metroPhase * 2.0 * 3.14159265359) * 0.25f;
                metroPhase += 1000.0 / blindRate;
                --metroBeepSamplesLeft;
            }
        }
        else
        {
            metroSampleCounter = 0;
        }
        float delayedLeft = 0.0f;
        float delayedRight = 0.0f;

        if (bufferSize > 0)
        {
            int readPos = (delayWritePosition - delaySamples + bufferSize) % bufferSize;
            delayedLeft = delayBufferLeft[readPos];
            delayedRight = delayBufferRight[readPos];

            delayBufferLeft[delayWritePosition] = dryLeft + (delayedLeft * feedback);
            delayBufferRight[delayWritePosition] = dryRight + (delayedRight * feedback);
            delayWritePosition = (delayWritePosition + 1) % bufferSize;
        }
            
        if (leftChannel != nullptr) leftChannel[sampleIndex] = dryLeft + (delayedLeft * 0.6f) + clickSample;
        if (rightChannel != nullptr) rightChannel[sampleIndex] = dryRight + (delayedRight * 0.6f) + clickSample;

    }

    reverbParams.roomSize = (float)reverbSizeSlider.getValue();
    reverbParams.wetLevel = (float)reverbMixSlider.getValue();
    reverbParams.dryLevel = 1.0f - (reverbParams.wetLevel * 0.4f);
    masterReverb.setParameters(reverbParams);

    if (leftChannel != nullptr && rightChannel != nullptr && reverbParams.wetLevel > 0.01f)
        masterReverb.processStereo(leftChannel, rightChannel, bufferToFill.numSamples);

    float maxLeft = 0.0f, maxRight = 0.0f;
    for (auto sampleIndex = 0; sampleIndex < bufferToFill.numSamples; ++sampleIndex)
    {
        if (leftChannel != nullptr)
        {
            leftChannel[sampleIndex] *= masterVol;
            maxLeft = std::max(maxLeft, std::abs(leftChannel[sampleIndex]));
        }
        if (rightChannel != nullptr)
        {
            rightChannel[sampleIndex] *= masterVol;
            maxRight = std::max(maxRight, std::abs(rightChannel[sampleIndex]));
        }
    }

    if (isRecordingMaster && leftChannel != nullptr && rightChannel != nullptr)
    {
        int spaceLeft = recordedAudioBuffer.getNumSamples() - recordedSamplesCount;
        int samplesToCopy = std::min(bufferToFill.numSamples, spaceLeft);
        if (samplesToCopy > 0)
        {
            recordedAudioBuffer.copyFrom(0, recordedSamplesCount, leftChannel, samplesToCopy);
            recordedAudioBuffer.copyFrom(1, recordedSamplesCount, rightChannel, samplesToCopy);
            recordedSamplesCount += samplesToCopy;
        }
    }

    currentLevelLeft = std::max(maxLeft, currentLevelLeft * 0.85f);
    currentLevelRight = std::max(maxRight, currentLevelRight * 0.85f);
    }

}
    }


void MainComponent::releaseResources() { masterReverb.reset(); }



void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff141414));


g.setColour(juce::Colour(0xff1f1f1f));
g.fillRect(0, 28, getWidth(), 50);
g.setColour(juce::Colours::white.withAlpha(0.2f));
g.drawLine(0, 78, (float)getWidth(), 78, 1.0f);
// Pantalla de Osciloscopio en tiempo real (Barra superior extendida)
int scopeX = 765;
int scopeW = getWidth() - 785;

if (scopeW > 80 && !scopeBuffer.empty())
{
    g.setColour(juce::Colour(0xff0b1315));
    g.fillRoundedRectangle((float)scopeX, 36.0f, (float)scopeW, 34.0f, 4.0f);
    g.setColour(juce::Colours::cyan.withAlpha(0.5f));
    g.drawRoundedRectangle((float)scopeX, 36.0f, (float)scopeW, 34.0f, 4.0f, 1.0f);

    juce::Path wavePath;
    float midY = 53.0f;
    for (int i = 0; i < scopeW; ++i)
    {
        int idx = (scopeWritePos + (i * (int)scopeBuffer.size() / scopeW)) % (int)scopeBuffer.size();
        float y = midY - (scopeBuffer[idx] * 14.0f);
        if (i == 0) wavePath.startNewSubPath((float)(scopeX + i), y);
        else        wavePath.lineTo((float)(scopeX + i), y);
    }
    g.setColour(juce::Colours::mediumspringgreen);
    g.strokePath(wavePath, juce::PathStrokeType(1.5f));
}

int chY = 93;
int chH = getHeight() - 176;

// Fondo del Browser estilo Ableton (Izquierda)
g.setColour(juce::Colour(0xff161616));
g.fillRoundedRectangle(15.0f, (float)chY, 250.0f, (float)chH, 8.0f);
g.setColour(juce::Colours::white.withAlpha(0.12f));
g.drawRoundedRectangle(15.0f, (float)chY, 250.0f, (float)chH, 8.0f, 1.0f);
g.setColour(juce::Colours::cyan);
g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
g.drawText("BROWSER | ACTIVO: " + loadedPluginName, 22, chY + 4, 236, 18, juce::Justification::centredLeft, true);

// Fondo de los Canales (Centro)
g.setColour(juce::Colour(0xff1a1a1a));
g.fillRoundedRectangle(275.0f, (float)chY, (float)(getWidth() - 535), (float)chH, 8.0f);



    int fxX = getWidth() - 245;
    int fxW = 100;
    g.setColour(juce::Colour(0xff1d1824));
    g.fillRect(fxX, chY, fxW, chH);
    g.setColour(juce::Colours::mediumpurple);
    g.drawRect(fxX, chY, fxW, chH, 1);

    g.setColour(juce::Colours::mediumpurple);
    g.fillRect(fxX, chY, fxW, 22);
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    g.drawText("MASTER FX", fxX, chY, fxW, 22, juce::Justification::centred);

    g.setColour(juce::Colours::plum);
    g.setFont(9.5f);
    g.drawText("REV SIZE", fxX, chY + 32, fxW, 12, juce::Justification::centred);
    g.drawText("REV MIX", fxX, chY + 125, fxW, 12, juce::Justification::centred);
    g.setColour(juce::Colours::lightgreen);
    g.drawText("DELAY TIME", fxX, chY + 235, fxW, 12, juce::Justification::centred);
    g.drawText("FEEDBACK", fxX, chY + 328, fxW, 12, juce::Justification::centred);

    g.setColour(juce::Colours::mediumpurple);
    g.fillRect(fxX, chY + chH - 20, fxW, 20);
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    g.drawText("FX BUS", fxX, chY + chH - 20, fxW, 20, juce::Justification::centred);

    int masterX = getWidth() - 135;
    int masterW = 115;
    g.setColour(juce::Colour(0xff221c1c));
    g.fillRect(masterX, chY, masterW, chH);
    g.setColour(juce::Colours::orangered);
    g.drawRect(masterX, chY, masterW, chH, 2);

    g.setColour(juce::Colours::orangered);
    g.fillRect(masterX, chY, masterW, 22);
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    g.drawText("MASTER OUT", masterX, chY, masterW, 22, juce::Justification::centred);

    g.setColour(juce::Colours::lightgrey);
    g.setFont(10.0f);
    g.drawText("MAIN VOL", masterX + 5, chY + 30, 55, 14, juce::Justification::centred);
    g.drawText("L   R", masterX + 65, chY + 30, 42, 14, juce::Justification::centred);

    int numLeds = 24;
    int ledAreaTop = chY + 50;
    int ledAreaHeight = chH - 80;
    int ledHeight = (ledAreaHeight / numLeds) - 2;

    for (int i = 0; i < numLeds; ++i)
    {
        float threshold = (float)(i + 1) / (float)numLeds;
        int ledY = ledAreaTop + ledAreaHeight - ((i + 1) * (ledAreaHeight / numLeds));

        juce::Colour ledOnColor = juce::Colours::limegreen;
        if (i >= 20) ledOnColor = juce::Colours::red;
        else if (i >= 15) ledOnColor = juce::Colours::yellow;

        bool leftLit = (currentLevelLeft * 1.8f) >= threshold;
        g.setColour(leftLit ? ledOnColor : ledOnColor.withAlpha(0.15f));
        g.fillRect(masterX + 70, ledY, 13, ledHeight);

        bool rightLit = (currentLevelRight * 1.8f) >= threshold;
        g.setColour(rightLit ? ledOnColor : ledOnColor.withAlpha(0.15f));
        g.fillRect(masterX + 88, ledY, 13, ledHeight);
    }

    g.setColour(juce::Colours::orangered);
    g.fillRect(masterX, chY + chH - 20, masterW, 20);
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    g.drawText("MAIN", masterX, chY + chH - 20, masterW, 20, juce::Justification::centred);

}

void MainComponent::resized()
{
    int menuH = 28;
    if (menuBar != nullptr)
        menuBar->setBounds(0, 0, getWidth(), menuH);

    playButton.setBounds(20, menuH + 10, 62, 30);
    stopButton.setBounds(88, menuH + 10, 62, 30);
    recWavButton.setBounds(156, menuH + 10, 80, 30);

    addAudioBtn.setBounds(255, menuH + 10, 85, 30);
    addMidiBtn.setBounds(346, menuH + 10, 85, 30);

    saveBtn.setBounds(450, menuH + 10, 65, 30);
    loadBtn.setBounds(521, menuH + 10, 65, 30);
    bpmSlider.setBounds(600, menuH + 10, 100, 30);
    metronomeBtn.setBounds(710, menuH + 10, 65, 30);
    bpmSlider.setBounds(595, menuH + 10, 95, 30);
    metronomeBtn.setBounds(696, menuH + 10, 58, 30);



    int chY = menuH + 65;
    int keyH = 68;
    int chH = getHeight() - (menuH + 80) - keyH;

    int brX = 15, brW = 250;
    browserSearchBox.setBounds(brX + 6, chY + 26, brW - 12, 24);
    catAllBtn.setBounds(brX + 6, chY + 54, 70, 24);
    catNativeBtn.setBounds(brX + 6, chY + 82, 70, 24);
    catPluginsBtn.setBounds(brX + 6, chY + 110, 70, 24);
    scanVstBtn.setBounds(brX + 6, chY + 142, 70, 26);
    openGuiBtn.setBounds(brX + 6, chY + 174, 70, 24);
    unloadVstBtn.setBounds(brX + 6, chY + 202, 70, 24);
    browserListBox.setBounds(brX + 80, chY + 54, brW - 86, chH - 60);

    channelViewport.setBounds(275, chY, getWidth() - 535, chH + 15);
    midiKeyboard.setBounds(15, getHeight() - keyH - 5, getWidth() - 30, keyH);

    int fxX = getWidth() - 245;
    int fxW = 100;
    reverbSizeSlider.setBounds(fxX + 14, chY + 46, fxW - 28, 72);
    reverbMixSlider.setBounds(fxX + 14, chY + 139, fxW - 28, 72);
    delayTimeSlider.setBounds(fxX + 14, chY + 249, fxW - 28, 72);
    delayFeedbackSlider.setBounds(fxX + 14, chY + 342, fxW - 28, 72);

    int masterX = getWidth() - 135;
    masterVolumeSlider.setBounds(masterX + 10, chY + 50, 50, chH - 75);

    if (audioSetupComp != nullptr && showAudioSetup)
        audioSetupComp->setBounds(440, chY, getWidth() - 700, chH);
}

void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    masterReverb.setSampleRate(sampleRate);
    nativeSynth.setCurrentPlaybackSampleRate(sampleRate);

}

//==============================================================================
// Logica del Browser estilo Ableton + Escaneo y Carga de Plugins VST3
int MainComponent::BrowserListModel::getNumRows()
{
    return owner != nullptr ? owner->browserItemNames.size() : 0;
}

void MainComponent::BrowserListModel::paintListBoxItem(int row, juce::Graphics& g, int width, int height, bool selected)
{
    if (owner == nullptr || row < 0 || row >= owner->browserItemNames.size())
        return;

    if (selected)
        g.fillAll(juce::Colour(0xff2d5a68));
    else if (row % 2 == 1)
        g.fillAll(juce::Colour(0xff1b1b1b));

    int type = owner->browserItemTypes[row];
    g.setColour(type < 100 ? juce::Colours::mediumspringgreen : juce::Colours::orange);
    g.fillRoundedRectangle(6.0f, (height - 8.0f) * 0.5f, 8.0f, 8.0f, 4.0f);

    g.setColour(juce::Colours::white);
    g.setFont(13.0f);
    g.drawText(owner->browserItemNames[row], 20, 0, width - 24, height, juce::Justification::centredLeft, true);
}

void MainComponent::BrowserListModel::selectedRowsChanged(int lastRowSelected)
{
    if (owner != nullptr)
        owner->selectedBrowserRow = lastRowSelected;
}

void MainComponent::BrowserListModel::listBoxItemDoubleClicked(int row, const juce::MouseEvent&)
{
    if (owner != nullptr)
        owner->loadBrowserItem(row);
}

void MainComponent::rebuildBrowserList()
{
    browserItemNames.clear();
    browserItemTypes.clear();
    auto query = browserSearchBox.getText().trim().toLowerCase();

    auto tryAdd = [&](const juce::String& label, int typeId)
        {
            if (query.isEmpty() || label.toLowerCase().contains(query))
            {
                browserItemNames.add(label);
                browserItemTypes.add(typeId);
            }
        };

    if (browserCategory == 0 || browserCategory == 1)
    {
        tryAdd("[Nativo] Pad Suave", 1);
        tryAdd("[Nativo] E-Piano", 2);
        tryAdd("[Nativo] Synth Lead", 3);
    }

    if (browserCategory == 0 || browserCategory == 2)
    {
        auto types = knownPlugins.getTypes();
        for (int i = 0; i < types.size(); ++i)
            tryAdd(types[i].manufacturerName + " - " + types[i].name, 100 + i);
    }

    browserListBox.updateContent();
    repaint();
}

void MainComponent::loadBrowserItem(int row)
{
    if (row < 0 || row >= browserItemTypes.size())
        return;

    int typeId = browserItemTypes[row];
    if (typeId < 100)
    {
        activePluginWindow.reset();
        activeVstPlugin.reset();
        loadedPluginName = browserItemNames[row];
        for (int i = 0; i < nativeSynth.getNumVoices(); ++i)
            if (auto* v = dynamic_cast<MidawSynthVoice*>(nativeSynth.getVoice(i)))
                v->presetType = typeId;
        repaint();
        return;
    }

    int pluginIdx = typeId - 100;
    auto types = knownPlugins.getTypes();
    if (pluginIdx >= 0 && pluginIdx < types.size())
    {
        juce::String errorMsg;
        double sr = deviceManager.getCurrentAudioDevice() != nullptr ? deviceManager.getCurrentAudioDevice()->getCurrentSampleRate() : 44100.0;
        int bs = deviceManager.getCurrentAudioDevice() != nullptr ? deviceManager.getCurrentAudioDevice()->getCurrentBufferSizeSamples() : 512;

        if (auto instance = pluginFormatManager.createPluginInstance(types[pluginIdx], sr, bs, errorMsg))
        {
            activePluginWindow.reset();
            instance->prepareToPlay(sr, bs);
            activeVstPlugin = std::move(instance);
            loadedPluginName = types[pluginIdx].name;

            if (activeVstPlugin->hasEditor())
                activePluginWindow = std::make_unique<PluginEditorWindow>(loadedPluginName, activeVstPlugin->createEditorIfNeeded());

            repaint();
        }
        else
        {
            juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "Error cargando VST3", errorMsg);
        }
    }
}

void MainComponent::scanForVst3Plugins()
{
    for (auto* format : pluginFormatManager.getFormats())
    {
        juce::PluginDirectoryScanner scanner(knownPlugins, *format, format->getDefaultLocationsToSearch(), true, juce::File());
        juce::String pluginName;
        while (scanner.scanNextFile(true, pluginName)) {}
    }
    browserCategory = 2;
    rebuildBrowserList();
}
