target_sources(
  firmware
  PRIVATE display_ex.cpp
          display_helper.cpp
          gui.cpp
          gui_timer.cpp
          guitypes.cpp
          i_window_menu_container.cpp
          i_window_menu_item.cpp
          i_window_text.cpp
          Jogwheel.cpp
          knob_event.cpp
          menu_item_event_dispatcher.cpp
          menu_opener.cpp
          menu_spin_config_types.cpp
          png_measure.cpp
          Rect16.cpp
          screen.cpp
          ScreenShot.cpp
          scroll_bar.cpp
          term.cpp
          text_roll.cpp
          window.cpp
          window_arrows.cpp
          window_event.cpp
          window_frame.cpp
          window_icon.cpp
          window_menu.cpp
          window_menu_adv.cpp
          window_menu_bar.cpp
          window_msgbox.cpp
          window_numb.cpp
          window_progress.cpp
          window_qr.cpp
          window_roll_text.cpp
          window_term.cpp
          window_text.cpp
          WindowItemFormatableSpin.cpp
          WindowMenuInfo.cpp
          WindowMenuItems.cpp
          WindowMenuSpin.cpp
          WindowMenuSpinExponential.cpp
          WindowMenuSwitch.cpp
  )

# has LEDS
if(HAS_LEDS)
  target_sources(firmware PRIVATE gui_leds.cpp led_lcd_cs_selector.cpp)
endif()

if(HAS_TOOLCHANGER)
  target_sources(firmware PRIVATE window_tool_action_box.cpp)
endif()

if(BOARD IN_LIST BOARDS_WITH_ST7789V)
  target_sources(firmware PRIVATE menu_spin_config_basic.cpp st7789v.cpp)
elseif(BOARD IN_LIST BOARDS_WITH_ILI9488)
  target_sources(firmware PRIVATE ili9488.cpp menu_spin_config_with_units.cpp)
endif()
