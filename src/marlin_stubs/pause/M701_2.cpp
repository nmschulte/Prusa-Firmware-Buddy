#include "config_features.h"

// clang-format off
#if (!ENABLED(FILAMENT_LOAD_UNLOAD_GCODES)) || \
    HAS_LCD_MENU || \
    ENABLED(MIXING_EXTRUDER) || \
    ENABLED(NO_MOTION_BEFORE_HOMING)
    #error unsupported
#endif
// clang-format on

#include "../../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "../../../lib/Marlin/Marlin/src/Marlin.h"
#include "../../../lib/Marlin/Marlin/src/module/motion.h"
#include "../../../lib/Marlin/Marlin/src/module/planner.h"
#include "../../../lib/Marlin/Marlin/src/module/temperature.h"

#include "marlin_server.hpp"
#include "pause_stubbed.hpp"
#include <functional> // std::invoke
#include <cmath>
#include "task.h" //critical sections
#include "filament_sensors_handler.hpp"
#include "eeprom_function_api.h"
#include "RAII.hpp"
#include "M70X.hpp"
#include "fs_event_autolock.hpp"

uint filament_gcodes::InProgress::lock = 0;

using namespace filament_gcodes;

/**
 * Shared code for load/unload filament
 */
bool filament_gcodes::load_unload([[maybe_unused]] LoadUnloadMode type, filament_gcodes::Func f_load_unload, pause::Settings &rSettings) {
    float disp_temp = marlin_vars()->active_hotend().display_nozzle;
    float targ_temp = Temperature::degTargetHotend(rSettings.GetExtruder());

    if (disp_temp > targ_temp) {
        thermalManager.setTargetHotend(disp_temp, rSettings.GetExtruder());
    }

    // Load/Unload filament
    bool res = std::invoke(f_load_unload, Pause::Instance(), rSettings);

    if (disp_temp > targ_temp) {
        thermalManager.setTargetHotend(targ_temp, rSettings.GetExtruder());
    }
    return res;
}

void filament_gcodes::M701_no_parser(filament::Type filament_to_be_loaded, const std::optional<float> &fast_load_length, float z_min_pos, std::optional<RetAndCool_t> op_preheat, uint8_t target_extruder, int8_t mmu_slot) {
    InProgress progress;

    if (op_preheat) {
        if (filament_to_be_loaded == filament::Type::NONE) {
            PreheatData data(!fast_load_length.has_value() || fast_load_length > 0.F ? PreheatMode::Load : PreheatMode::Purge, *op_preheat);
            auto preheat_ret = data.Mode() == PreheatMode::Load ? preheat_for_change_load(data, target_extruder) : preheat(data, target_extruder);
            if (preheat_ret.first) {
                // canceled
                M70X_process_user_response(*preheat_ret.first, target_extruder);
                return;
            }

            filament_to_be_loaded = preheat_ret.second;
        } else {
            preheat_to(filament_to_be_loaded, target_extruder);
        }
    }
    filament::set_type_to_load(filament_to_be_loaded);

    pause::Settings settings;
    settings.SetExtruder(target_extruder);
    settings.SetFastLoadLength(fast_load_length);
    settings.SetRetractLength(0.f);
    settings.SetMmuFilamentToLoad(mmu_slot);
    xyz_pos_t park_position = { X_AXIS_LOAD_POS, NAN, z_min_pos > 0 ? std::max(current_position.z, z_min_pos) : NAN };
#ifndef DO_NOT_RESTORE_Z_AXIS
    settings.SetResumePoint(current_position);
#endif
    settings.SetParkPoint(park_position);

    // Pick the right tool
    if (!Pause::Instance().ToolChange(target_extruder, LoadUnloadMode::Load, settings)) {
        return;
    }

    // Load
    if (load_unload(LoadUnloadMode::Load, PRINTER_TYPE == PRINTER_PRUSA_IXL ? &Pause::FilamentLoadNotBlocking : &Pause::FilamentLoad, settings)) {
        M70X_process_user_response(PreheatStatus::Result::DoneHasFilament, target_extruder);
    } else {
        M70X_process_user_response(PreheatStatus::Result::DidNotFinish, target_extruder);
    }
}

void filament_gcodes::M702_no_parser(std::optional<float> unload_length, float z_min_pos, std::optional<RetAndCool_t> op_preheat, uint8_t target_extruder, bool ask_unloaded) {
    InProgress progress;

    if (op_preheat) {
        PreheatData data(PreheatMode::Unload, *op_preheat); // TODO do I need PreheatMode::Unload_askUnloaded
        auto preheat_ret = preheat(data, target_extruder);
        if (preheat_ret.first) {
            // canceled
            M70X_process_user_response(*preheat_ret.first, target_extruder);
            return;
        }
    }

    pause::Settings settings;
    settings.SetExtruder(target_extruder);
    settings.SetUnloadLength(unload_length);
    xyz_pos_t park_position = { X_AXIS_UNLOAD_POS, NAN, z_min_pos > 0 ? std::max(current_position.z, z_min_pos) : NAN };
#ifndef DO_NOT_RESTORE_Z_AXIS
    settings.SetResumePoint(current_position);
#endif
    settings.SetParkPoint(park_position);

    // Pick the right tool
    if (!Pause::Instance().ToolChange(target_extruder, LoadUnloadMode::Unload, settings)) {
        return;
    }

    // Unload
    if (load_unload(LoadUnloadMode::Unload, ask_unloaded ? &Pause::FilamentUnload_AskUnloaded : &Pause::FilamentUnload, settings)) {
        M70X_process_user_response(PreheatStatus::Result::DoneNoFilament, target_extruder);
    } else {
        M70X_process_user_response(PreheatStatus::Result::DidNotFinish, target_extruder);
    }
}

namespace PreheatStatus {

volatile static Result preheatResult = Result::DidNotFinish;

Result ConsumeResult() {
    taskENTER_CRITICAL();
    Result ret = preheatResult;
    preheatResult = Result::DidNotFinish;
    taskEXIT_CRITICAL();
    return ret;
}

void SetResult(Result res) {
    preheatResult = res;
}

}

void filament_gcodes::M70X_process_user_response(PreheatStatus::Result res, uint8_t target_extruder) {
    // modify temperatures
    switch (res) {
    case PreheatStatus::Result::DoneHasFilament: {
        auto filament = filament::get_type_in_extruder(target_extruder);
        auto preheat_temp = filament::get_description(filament).nozzle_preheat;
        thermalManager.setTargetHotend(preheat_temp, 0);
        break;
    }
    case PreheatStatus::Result::CooledDown:
        // set temperatures to zero
        thermalManager.setTargetHotend(0, 0);
        thermalManager.setTargetBed(0);
        marlin_server::set_temp_to_display(0, 0);
        thermalManager.set_fan_speed(0, 0);
        break;
    case PreheatStatus::Result::DoneNoFilament:
    case PreheatStatus::Result::Aborted:
    case PreheatStatus::Result::Error:
    case PreheatStatus::Result::DidNotFinish: // cannot happen
    default:
        break; // do not alter temp
    }

    // store result, so other threads can see it
    PreheatStatus::SetResult(res);
}

void filament_gcodes::M1701_no_parser(const std::optional<float> &fast_load_length, float z_min_pos, uint8_t target_extruder) {
    InProgress progress;
    if constexpr (HAS_BOWDEN) {
        filament::set_type_in_extruder(filament::Type::NONE, target_extruder);
        M701_no_parser(filament::Type::NONE, fast_load_length, z_min_pos, RetAndCool_t::Return, target_extruder, 0);
    } else {

        pause::Settings settings;
        settings.SetExtruder(target_extruder);
        settings.SetFastLoadLength(fast_load_length);
        settings.SetRetractLength(0.f);

        // catch filament in gear and then ask for temp
        if (!Pause::Instance().LoadToGear(settings) || FSensors_instance().GetCurrentExtruder() == fsensor_t::NoFilament) {
            // do not ask for filament type after stop was pressed or filament was removed from FS
            Pause::Instance().UnloadFromGear();
            M70X_process_user_response(PreheatStatus::Result::DoneNoFilament, target_extruder);
            FSensors_instance().ClrAutoloadSent();
            return;
        }

        PreheatData data(PreheatMode::Autoload, RetAndCool_t::Return);
        auto preheat_ret = preheat_for_change_load(data, target_extruder);

        if (preheat_ret.first) {
            // canceled
            Pause::Instance().UnloadFromGear();
            M70X_process_user_response(PreheatStatus::Result::DoneNoFilament, target_extruder);
            FSensors_instance().ClrAutoloadSent();
            return;
        }

        filament::Type filament = preheat_ret.second;
        filament::set_type_to_load(filament);

        if (z_min_pos > 0 && z_min_pos > current_position.z + 0.1F) {
            xyz_pos_t park_position = { NAN, NAN, z_min_pos };
            // Returning to previous position is unwanted outside of printing (M1701 should be used only outside of printing)
            settings.SetParkPoint(park_position);
        }

        if (load_unload(LoadUnloadMode::Load, &Pause::FilamentAutoload, settings)) {
            M70X_process_user_response(PreheatStatus::Result::DoneHasFilament, target_extruder);
        } else {
            M70X_process_user_response(PreheatStatus::Result::DidNotFinish, target_extruder);
        }
    }

    FSensors_instance().ClrAutoloadSent();
}

void filament_gcodes::M1600_no_parser(filament::Type filament_to_be_loaded, uint8_t target_extruder, RetAndCool_t preheat, AskFilament_t ask_filament) {
    FS_EventAutolock autoload_lock;
    InProgress progress;

    filament::Type filament = filament::get_type_in_extruder(target_extruder);
    if (filament == filament::Type::NONE && ask_filament == AskFilament_t::Never) {
        PreheatStatus::SetResult(PreheatStatus::Result::DoneNoFilament);
        return;
    }

    if (ask_filament == AskFilament_t::Always || (filament == filament::Type::NONE && ask_filament == AskFilament_t::IfUnknown)) {
        M1700_no_parser(preheat, target_extruder, true, true, eeprom_get_bool(EEVAR_HEATUP_BED)); // need to save filament to check if operation went well
        filament = filament::get_type_in_extruder(target_extruder);
        if (filament == filament::Type::NONE)
            return; // no need to set PreheatStatus::Result::DoneNoFilament, M1700 did that
    }

    PreheatStatus::SetResult(PreheatStatus::Result::DoneHasFilament);

    preheat_to(filament, target_extruder);
    xyze_pos_t current_position_tmp = current_position;

    pause::Settings settings;
    xyz_pos_t park_position = { X_AXIS_UNLOAD_POS, NAN, std::max(current_position.z, (float)Z_AXIS_LOAD_POS) };
    settings.SetParkPoint(park_position);
    settings.SetExtruder(target_extruder);
    settings.SetRetractLength(0.f);

    // Pick the right tool
    if (!Pause::Instance().ToolChange(target_extruder, LoadUnloadMode::Unload, settings)) {
        return;
    }

    // Unload
    if (load_unload(LoadUnloadMode::Unload, PRINTER_TYPE == PRINTER_PRUSA_IXL ? &Pause::FilamentUnload : &Pause::FilamentUnload_AskUnloaded, settings)) {
        M70X_process_user_response(PreheatStatus::Result::DoneNoFilament, target_extruder);
    } else {
        M70X_process_user_response(PreheatStatus::Result::DidNotFinish, target_extruder);
        return;
    }

    // LOAD
    // cannot do normal preheat, since printer is already preheated from unload
    if (filament_to_be_loaded == filament::Type::NONE) {
        PreheatData data(PreheatMode::Change_phase2, preheat);
        auto preheat_ret = preheat_for_change_load(data, target_extruder);
        if (preheat_ret.first) {
            // canceled
            M70X_process_user_response(*preheat_ret.first, target_extruder);
            return;
        }

        filament_to_be_loaded = preheat_ret.second;
    } else {
        preheat_to(filament_to_be_loaded, target_extruder);
    }
    filament::set_type_to_load(filament_to_be_loaded);

#ifndef DO_NOT_RESTORE_Z_AXIS
    settings.SetResumePoint(current_position_tmp);
#endif

    if (load_unload(LoadUnloadMode::Load, PRINTER_TYPE == PRINTER_PRUSA_IXL ? &Pause::FilamentLoadNotBlocking : &Pause::FilamentLoad, settings)) {
        M70X_process_user_response(PreheatStatus::Result::DoneHasFilament, target_extruder);
    } else {
        M70X_process_user_response(PreheatStatus::Result::DidNotFinish, target_extruder);
    }
}
