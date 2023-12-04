#pragma once

#include "common.hpp"

#include <functional>
#include <tuple>
#include <optional>
#include <vector>
#include <atomic>

#include <module/prusa/accelerometer.h>
#include <core/types.h>

namespace phase_stepping {

/**
 * Assuming phase stepping is enabled, measure resonance data for given axis.
 * Returns measured accelerometer samples via a callback, the function returns
 * measured frequency.
 *
 * Speed is assumed to be always positive in rotations per seconds, revolutions
 * to made can be negative.
 *
 * On accelerometer error returns 0 as frequency. The error reason is silently
 * ignored.
 */
float capture_samples(AxisEnum axis, float speed, float revs,
    const std::function<void(const PrusaAccelerometer::Acceleration &)> &yield_sample);

/**
 * Assuming phase stepping is enabled, measure resonance and return requested
 * harmonics.
 *
 * Returns the measurement measurement.
 */
std::vector<float> analyze_resonance(AxisEnum axis,
    float speed, float revs, const std::vector<int> &requested_harmonics);

/**
 * Calibration routine notifies about the progress made via this class. Subclass
 * it and pass it to the calibration routine.
 */
class CalibrationReporterBase {
protected:
    int _calibration_phases_count = -1;
    int _current_calibration_phase = 0;

public:
    virtual ~CalibrationReporterBase() = default;

    /**
     * Report initial movement is in progress
     */
    virtual void on_initial_movement() = 0;

    /**
     * Set number of calibration phases
     */
    virtual void set_calibration_phases_count(int phases) {
        _calibration_phases_count = phases;
    }

    /**
     * Report beginning of the new phase
     */
    virtual void on_enter_calibration_phase(int phase) {
        _current_calibration_phase = phase;
    }

    /**
     * Report progress in percent for given phase
     */
    virtual void on_calibration_phase_progress(int progress) = 0;

    /**
     * Report result in percents for given calibration phase. Lower = better.
     */
    virtual void on_calibration_phase_result(float forward_score, float backward_score) = 0;

    /**
     * Report calibration termination
     */
    virtual void on_termination() = 0;
};

/**
 * Assuming the printer is homed, calibrate given axis. The progress is reported
 * via reporter. The routine is blocking.
 *
 * Returns a tuple with forward and backward calibration
 */
std::optional<std::tuple<MotorPhaseCorrection, MotorPhaseCorrection>>
calibrate_axis(AxisEnum axis, CalibrationReporterBase &reporter);

enum class CalibrationResult {
    Ok, // Last calibration successful
    Unknown, // Calibration not run
    Error, // Last calibration failed
};

/**
 * Global error state of the last axis calibration. Re/set by M977.
 */
extern std::atomic<CalibrationResult> last_calibration_result;

} // namespace phase_stepping
