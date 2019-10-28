/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Velocities for MidiNote's.
 */

#ifndef __AUDIO_VELOCITY_H__
#define __AUDIO_VELOCITY_H__

#include <stdint.h>

#include "gui/backend/arranger_object.h"

#include <cyaml/cyaml.h>

typedef struct MidiNote MidiNote;
typedef struct _VelocityWidget VelocityWidget;

/**
 * @addtogroup audio
 *
 * @{
 */

#define velocity_is_main(r) \
  arranger_object_is_main ( \
    (ArrangerObject *) r)
#define velocity_is_transient(r) \
  arranger_object_is_transient ( \
    (ArrangerObject *) r)
#define velocity_get_main(r) \
  ((Velocity *) \
   arranger_object_get_main ( \
     (ArrangerObject *) r))
#define velocity_get_main_trans(r) \
  ((Velocity *) \
   arranger_object_get_main_trans ( \
     (ArrangerObject *) r))
#define velocity_is_selected(r) \
  arranger_object_is_selected ( \
    (ArrangerObject *) r)

/**
 * Default velocity.
 */
#define VELOCITY_DEFAULT 90

/**
 * The MidiNote velocity.
 */
typedef struct Velocity
{
  /** Base struct. */
  ArrangerObject  base;

  /** Velocity value (0-127). */
  uint8_t          vel;

  /** Cache velocity, used to save the values at
   * the start of actions. */
  uint8_t          cache_vel;

  /**
   * Owner.
   *
   * For convenience only.
   */
  MidiNote *       midi_note;
} Velocity;

static const cyaml_schema_field_t
velocity_fields_schema[] =
{
  CYAML_FIELD_MAPPING (
    "base", CYAML_FLAG_DEFAULT,
    Velocity, base,
    arranger_object_fields_schema),
  CYAML_FIELD_UINT (
    "vel", CYAML_FLAG_DEFAULT,
    Velocity, vel),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
velocity_schema = {
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
    Velocity, velocity_fields_schema),
};

/**
 * Creates a new Velocity with the given value.
 */
Velocity *
velocity_new (
  MidiNote *    midi_note,
  const uint8_t vel,
  const int     is_main);

/**
 * Sets the MidiNote the Velocity belongs to.
 */
void
velocity_set_midi_note (
  Velocity * velocity,
  MidiNote * midi_note);

/**
 * Sets the cached value for use in live actions.
 */
void
velocity_set_cache_vel (
  Velocity * velocity,
  const uint8_t vel);

/**
 * Returns 1 if the Velocity's match, 0 if not.
 */
int
velocity_is_equal (
  Velocity * src,
  Velocity * dest);

/**
 * Changes the Velocity by the given amount of
 * values (delta).
 */
void
velocity_shift (
  Velocity * self,
  const int  delta);

/**
 * Sets the velocity to the given value.
 *
 * The given value may exceed the bounds 0-127,
 * and will be clamped.
 */
void
velocity_set_val (
  Velocity *    self,
  const int     val,
  ArrangerObjectUpdateFlag update_flag);

/**
 * @}
 */

#endif
