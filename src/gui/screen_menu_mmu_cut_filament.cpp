/**
 * @file screen_menu_mmu_cut_filament.cpp
 */

#include "screen_menu_mmu_cut_filament.hpp"
#include "png_resources.hpp"

ScreenMenuMMUCutFilament::ScreenMenuMMUCutFilament()
    : ScreenMenuMMUCutFilament__(_(label)) {
    header.SetIcon(&png::info_16x16);
}
