#include "EffectChainPanel.h"

EffectChainPanel::EffectChainPanel(EffectChain& chain)
    : effectChain(chain)
{
    // Title label
    titleLabel.setText("EFFECT CHAIN", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, ProgFlowColours::textPrimary());
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(titleLabel);

    // Add effect selector
    populateEffectSelector();
    addEffectSelector.onChange = [this]() { onAddEffectSelected(); };
    addAndMakeVisible(addEffectSelector);

    // Clear button
    clearButton.setButtonText("Clear All");
    clearButton.setColour(juce::TextButton::buttonColourId, ProgFlowColours::bgTertiary());
    clearButton.setColour(juce::TextButton::textColourOffId, ProgFlowColours::accentRed());
    clearButton.onClick = [this]()
    {
        effectChain.clearAll();
        refreshFromChain();
    };
    addAndMakeVisible(clearButton);

    // Create effect slots
    for (int i = 0; i < EffectChain::MAX_EFFECTS; ++i)
    {
        slots[i] = std::make_unique<EffectSlot>(i);
        slots[i]->onRemoveClicked = [this](int slot) { onRemoveEffect(slot); };
        slots[i]->onBypassToggled = [this](int slot, bool bypassed) { onBypassToggled(slot, bypassed); };
        slots[i]->onEffectDropped = [this](int from, int to) { onEffectDropped(from, to); };
        slotsContainer.addAndMakeVisible(*slots[i]);
    }

    // Viewport for scrolling
    viewport.setViewedComponent(&slotsContainer, false);
    viewport.setScrollBarsShown(false, true);
    addAndMakeVisible(viewport);

    refreshFromChain();
}

EffectChainPanel::~EffectChainPanel() = default;

void EffectChainPanel::populateEffectSelector()
{
    addEffectSelector.clear();
    addEffectSelector.addItem("Add Effect...", 1);
    addEffectSelector.addSeparator();

    // Dynamics
    addEffectSelector.addSectionHeading("Dynamics");
    addEffectSelector.addItem("Compressor", 10);
    addEffectSelector.addItem("Sidechain Compressor", 13);
    addEffectSelector.addItem("Limiter", 11);
    addEffectSelector.addItem("Gate", 12);

    // Modulation
    addEffectSelector.addSectionHeading("Modulation");
    addEffectSelector.addItem("Chorus", 20);
    addEffectSelector.addItem("Phaser", 21);
    addEffectSelector.addItem("Flanger", 22);
    addEffectSelector.addItem("Tremolo", 23);

    // Time-Based
    addEffectSelector.addSectionHeading("Time-Based");
    addEffectSelector.addItem("Reverb", 30);
    addEffectSelector.addItem("Delay", 31);

    // Distortion
    addEffectSelector.addSectionHeading("Distortion");
    addEffectSelector.addItem("Distortion", 40);
    addEffectSelector.addItem("Bitcrusher", 41);

    // EQ & Filter
    addEffectSelector.addSectionHeading("EQ & Filter");
    addEffectSelector.addItem("EQ", 50);
    addEffectSelector.addItem("Filter", 51);

    // Amp Simulation
    addEffectSelector.addSectionHeading("Amp Simulation");
    addEffectSelector.addItem("Amp Simulator", 60);
    addEffectSelector.addItem("Cabinet", 61);

    addEffectSelector.setSelectedId(1, juce::dontSendNotification);
}

std::unique_ptr<EffectBase> EffectChainPanel::createEffect(const juce::String& name)
{
    if (name == "Reverb") return std::make_unique<ReverbEffect>();
    if (name == "Delay") return std::make_unique<DelayEffect>();
    if (name == "Chorus") return std::make_unique<ChorusEffect>();
    if (name == "Phaser") return std::make_unique<PhaserEffect>();
    if (name == "Flanger") return std::make_unique<FlangerEffect>();
    if (name == "Tremolo") return std::make_unique<TremoloEffect>();
    if (name == "Distortion") return std::make_unique<DistortionEffect>();
    if (name == "Bitcrusher") return std::make_unique<BitcrusherEffect>();
    if (name == "Compressor") return std::make_unique<CompressorEffect>();
    if (name == "Sidechain Compressor") return std::make_unique<SidechainCompressorEffect>();
    if (name == "Limiter") return std::make_unique<LimiterEffect>();
    if (name == "Gate") return std::make_unique<GateEffect>();
    if (name == "EQ") return std::make_unique<EQEffect>();
    if (name == "Filter") return std::make_unique<FilterEffect>();
    if (name == "Amp Simulator") return std::make_unique<AmpSimulatorEffect>();
    if (name == "Cabinet") return std::make_unique<CabinetEffect>();

    return nullptr;
}

void EffectChainPanel::onAddEffectSelected()
{
    auto selectedText = addEffectSelector.getText();
    if (selectedText == "Add Effect..." || selectedText.isEmpty())
        return;

    auto effect = createEffect(selectedText);
    if (effect)
    {
        effectChain.addEffect(std::move(effect));
        refreshFromChain();
    }

    // Reset selector
    addEffectSelector.setSelectedId(1, juce::dontSendNotification);
}

void EffectChainPanel::onRemoveEffect(int slot)
{
    effectChain.removeEffect(slot);
    refreshFromChain();
}

void EffectChainPanel::onBypassToggled(int slot, bool bypassed)
{
    effectChain.setSlotBypass(slot, bypassed);
}

void EffectChainPanel::onEffectDropped(int fromSlot, int toSlot)
{
    effectChain.swapEffects(fromSlot, toSlot);
    refreshFromChain();
}

void EffectChainPanel::refreshFromChain()
{
    for (int i = 0; i < EffectChain::MAX_EFFECTS; ++i)
    {
        slots[i]->setEffect(effectChain.getEffect(i));
        slots[i]->setBypass(effectChain.isSlotBypassed(i));
    }
    resized();
}

void EffectChainPanel::paint(juce::Graphics& g)
{
    g.fillAll(ProgFlowColours::bgSecondary());

    // Top bar background
    g.setColour(ProgFlowColours::bgPrimary());
    g.fillRect(0, 0, getWidth(), 40);

    // Border
    g.setColour(ProgFlowColours::border());
    g.drawHorizontalLine(40, 0.0f, static_cast<float>(getWidth()));
}

void EffectChainPanel::resized()
{
    const int margin = 8;
    const int topBarHeight = 40;
    const int slotWidth = 200;
    const int slotHeight = 140;

    auto bounds = getLocalBounds();

    // Top bar
    auto topBar = bounds.removeFromTop(topBarHeight).reduced(margin, 4);
    titleLabel.setBounds(topBar.removeFromLeft(150));
    clearButton.setBounds(topBar.removeFromRight(80).withHeight(28));
    topBar.removeFromRight(margin);
    addEffectSelector.setBounds(topBar.removeFromRight(150).withHeight(28));

    // Slots container
    bounds.reduce(margin, margin);
    int numSlots = EffectChain::MAX_EFFECTS;
    int containerWidth = numSlots * (slotWidth + margin);
    slotsContainer.setBounds(0, 0, containerWidth, slotHeight);

    int x = 0;
    for (int i = 0; i < numSlots; ++i)
    {
        slots[i]->setBounds(x, 0, slotWidth, slotHeight);
        x += slotWidth + margin;
    }

    viewport.setBounds(bounds.withHeight(slotHeight + 20));  // Extra for scrollbar
}
