target_sources(
  firmware
  PRIVATE ../../../lib/CrashCatcher/Core/src/CrashCatcher.c
          ../../../lib/CrashCatcher/Core/src/CrashCatcher_armv6m.S
          Cheese.cpp
          crash_catcher_integration.cpp
          fifo_encoder.cpp
          interrupts.cpp
          led.cpp
          loadcell.cpp
          logging.cpp
          main.c
          module_marlin.cpp
          task_startup.cpp
          timing_sys.cpp
          tool_filament_sensor.cpp
          trigger_crash_dump.c
          # these two CrashCatcher files need to be here otherwise HardFault_Handler will be handled
          # by Default_Handler (no compiler error)
  )

target_include_directories(firmware PUBLIC .)

add_subdirectory(modbus)
