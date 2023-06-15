#include "TopPanel.h"

TopPanel::TopPanel(juce::AudioProcessorValueTreeState& parameters) {
    // init combobox
    std::array comboboxList { &modeBox, &loudnessBox, &periodBox, &sideBox };
    std::array comboboxID { ZLDsp::mode::ID, ZLDsp::loudness::ID, ZLDsp::period::ID, ZLDsp::side::ID };
    for (size_t i = 0; i < comboboxList.size(); ++i) {
        *comboboxList[i] = std::make_unique<ComboboxComponent> (parameters.getParameter (comboboxID[i])->name,
            parameters.getParameter (comboboxID[i])->getAllValueStrings());
        addAndMakeVisible (*(*comboboxList[i]));
        comboboxAttachments.add (
            std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (parameters, comboboxID[i], (*comboboxList[i])->getComboBox()));
    }
}

TopPanel::~TopPanel() = default;

void TopPanel::paint (juce::Graphics&) {}

void TopPanel::resized() {
    juce::Grid grid;
    using Track = juce::Grid::TrackInfo;
    using Fr = juce::Grid::Fr;

    grid.templateRows = {Track(Fr(1))};
    grid.templateColumns = {Track(Fr(1)), Track(Fr(1)), Track(Fr(1)), Track(Fr(1))};

    juce::Array<juce::GridItem> items;
    items.add(*modeBox);
    items.add(*loudnessBox);
    items.add(*periodBox);
    items.add(*sideBox);
    grid.items = items;

    grid.performLayout(getLocalBounds());
}

void TopPanel::setFontSize (float size) {
    std::array comboboxList { &modeBox, &loudnessBox, &periodBox, &sideBox };
    for (auto const& c : comboboxList) {
        (*c)->setFontSize (size);
    }
}