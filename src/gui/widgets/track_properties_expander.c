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

#include "audio/engine.h"
#include "audio/mixer.h"
#include "audio/track.h"
#include "gui/widgets/editable_label.h"
#include "gui/widgets/track_properties_expander.h"
#include "project.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (TrackPropertiesExpanderWidget,
               track_properties_expander_widget,
               TWO_COL_EXPANDER_BOX_WIDGET_TYPE)

/**
 * Refreshes each field.
 */
void
track_properties_expander_widget_refresh (
  TrackPropertiesExpanderWidget * self)
{
  /* TODO */

}

/**
 * Sets up the TrackPropertiesExpanderWidget.
 */
void
track_properties_expander_widget_setup (
  TrackPropertiesExpanderWidget * self,
  Track *                             track)
{
  g_warn_if_fail (track);
  self->track = track;

  editable_label_widget_setup (
    self->name,
    track, track_get_name, track_set_name);
  track_properties_expander_widget_refresh (
    self);
}

static void
track_properties_expander_widget_class_init (
  TrackPropertiesExpanderWidgetClass * klass)
{
}

static void
track_properties_expander_widget_init (
  TrackPropertiesExpanderWidget * self)
{
  self->name =
    editable_label_widget_new (
      NULL, NULL, NULL, 11);
  two_col_expander_box_widget_add_single (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self),
    GTK_WIDGET (self->name));

  /* set name and icon */
  expander_box_widget_set_label (
    Z_EXPANDER_BOX_WIDGET (self),
    _("Track Info"));
  expander_box_widget_set_icon_resource (
    Z_EXPANDER_BOX_WIDGET (self),
    ICON_TYPE_ZRYTHM,
    "instrument.svg");
  expander_box_widget_set_orientation (
    Z_EXPANDER_BOX_WIDGET (self),
    GTK_ORIENTATION_VERTICAL);
}