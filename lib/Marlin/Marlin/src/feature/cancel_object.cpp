/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2020 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "../inc/MarlinConfig.h"

#if ENABLED(CANCEL_OBJECTS)

#include "cancel_object.h"
#include "../gcode/gcode.h"

CancelObject cancelable;

int8_t CancelObject::object_count, // = 0
       CancelObject::active_object = -1;
uint32_t CancelObject::canceled; // = 0x0000
bool CancelObject::skipping; // = false
CancelObject::ObjectNameItem *CancelObject::object_name_head;


void CancelObject::set_active_object(const int8_t obj) {
  active_object = obj;
  if (WITHIN(obj, 0, 31)) {
    if (obj >= object_count) object_count = obj + 1;
    skipping = TEST(canceled, obj);
  }
  else
    skipping = false;

  #if BOTH(HAS_STATUS_MESSAGE, CANCEL_OBJECTS_REPORTING)
    if (active_object >= 0)
      ui.status_printf(0, F(S_FMT " %i"), GET_TEXT(MSG_PRINTING_OBJECT), int(active_object));
    else
      ui.reset_status();
  #endif
}

void CancelObject::cancel_object(const int8_t obj) {
  if (WITHIN(obj, 0, 31)) {
    SBI(canceled, obj);
    if (obj == active_object) skipping = true;
  }
}

void CancelObject::uncancel_object(const int8_t obj) {
  if (WITHIN(obj, 0, 31)) {
    CBI(canceled, obj);
    if (obj == active_object) skipping = false;
  }
}

void CancelObject::save_object_name(int8_t obj, const char* name) {
  if (get_object_name(obj)) {
    return;
  }

  int name_len = _MIN(static_cast<int>(strlen(name)), 16);
  ObjectNameItem *item = (ObjectNameItem*)malloc(sizeof(ObjectNameItem) + name_len + 1);
  snprintf(item->name, name_len + 1, "%s", name);

  item->obj = obj;
  item->next = object_name_head;

  object_name_head = item;
}

const char* CancelObject::get_object_name(const int8_t obj) {
  for (ObjectNameItem *item = object_name_head; item != nullptr; item = item->next) {
    if (item->obj == obj) {
      return item->name;
    }
  }
  return nullptr;
}

void CancelObject::reset() {
  canceled = 0x0000;
  object_count = 0;
  clear_active_object();

  ObjectNameItem *item = object_name_head;
  while (item) {
    ObjectNameItem *next = item->next;
    free(item);
    item = next;
  }
  object_name_head = nullptr;
}

void CancelObject::report() {
  if (active_object >= 0) {
    SERIAL_ECHO_START();
    SERIAL_ECHOPGM("Active Object: ");
    SERIAL_ECHO(active_object);
    SERIAL_EOL();
  }

  if (canceled) {
    SERIAL_ECHO_START();
    SERIAL_ECHOPGM("Canceled:");
    for (int i = 0; i < object_count; i++)
      if (TEST(canceled, i)) { SERIAL_CHAR(' '); SERIAL_ECHO(i); }
    SERIAL_EOL();
  }
}

#endif // CANCEL_OBJECTS
