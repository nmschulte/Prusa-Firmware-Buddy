#include "radio_buttons.hpp"

//define counts of individual radio buttons here
const uint8_t RadioButtons::LoadUnloadCounts[RadioBtnCount<RadioBtnLoadUnload>()] = {
    1, 2
};

const uint8_t RadioButtons::TestCounts[RadioBtnCount<RadioBtnTest>()] = {
    1, 2
};
