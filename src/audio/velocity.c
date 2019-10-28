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

#include <stdlib.h>

#include "audio/midi_note.h"
#include "audio/velocity.h"
#include "gui/widgets/velocity.h"
#include "utils/types.h"

#include <gtk/gtk.h>

/**
 * Creates a new Velocity with the given value.
 */
Velocity *
velocity_new (
  MidiNote *    midi_note,
  const uint8_t vel,
  const int     is_main)
{
  Velocity * self =
    calloc (1, sizeof (Velocity));

  ArrangerObject * obj =
    (ArrangerObject *) self;
  obj->type = ARRANGER_OBJECT_TYPE_VELOCITY;

  self->vel = vel;
  self->midi_note = midi_note;

  if (is_main)
    {
      arranger_object_set_as_main (obj);
    }

  return self;
}

/**
 * Sets the MidiNote the Velocity belongs to.
 */
void
velocity_set_midi_note (
  Velocity * velocity,
  MidiNote * midi_note)
{
  Velocity * vel;
  for (int i = 0; i < 2; i++)
    {
      if (i == AOI_COUNTERPART_MAIN)
        vel =
          velocity_get_main (velocity);
      else if (i == AOI_COUNTERPART_MAIN_TRANSIENT)
        vel =
          velocity_get_main_trans (velocity);

      vel->midi_note = midi_note;
    }
}

/**
 * Returns 1 if the Velocity's match, 0 if not.
 */
int
velocity_is_equal (
  Velocity * src,
  Velocity * dest)
{
  return
    src->vel == dest->vel &&
    midi_note_is_equal (
      src->midi_note, dest->midi_note);
}

/**
 * Sets the cached value for use in live actions.
 */
void
velocity_set_cache_vel (
  Velocity *    velocity,
  const uint8_t vel)
{
  /* see ARRANGER_OBJ_SET_POS */
  velocity_get_main (velocity)->
    cache_vel = vel;
  velocity_get_main_trans (velocity)->
    cache_vel = vel;
}

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
  ArrangerObjectUpdateFlag update_flag)
{
  arranger_object_set_primitive (
    Velocity, self, vel,
    (uint8_t) CLAMP (val, 0, 127),
    update_flag);

  /* re-set the midi note value to set a note off
   * event */
  midi_note_set_val (
    self->midi_note,
    self->midi_note->val,
    update_flag);
}

/**
 * Changes the Velocity by the given amount of
 * values (delta).
 */
void
velocity_shift (
  Velocity * self,
  const int  delta)
{
  self->vel =
    (midi_byte_t)
    ((int) self->vel + delta);
}
