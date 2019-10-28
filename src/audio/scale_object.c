/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/scale_object.h"
#include "audio/chord_track.h"
#include "audio/position.h"
#include "gui/backend/arranger_object.h"
#include "project.h"
#include "utils/flags.h"

/**
 * Creates a ScaleObject.
 */
ScaleObject *
scale_object_new (
  MusicalScale * descr,
  int               is_main)
{
  ScaleObject * self =
    calloc (1, sizeof (ScaleObject));

  ArrangerObject * obj =
    (ArrangerObject *) self;
  obj->type = ARRANGER_OBJECT_TYPE_SCALE_OBJECT;

  self->scale = descr;

  if (is_main)
    {
      arranger_object_set_as_main (obj);
    }

  return self;
}

int
scale_object_is_equal (
  ScaleObject * a,
  ScaleObject * b)
{
  ArrangerObject * obj_a =
    (ArrangerObject *) a;
  ArrangerObject * obj_b =
    (ArrangerObject *) b;
  return
    position_is_equal (&obj_a->pos, &obj_b->pos) &&
    musical_scale_is_equal (a->scale, b->scale);
}

/**
 * Sets the Track of the scale.
 */
void
scale_object_set_track (
  ScaleObject * self,
  Track *  track)
{
  self->track = track;
  self->track_pos = track->pos;
}

