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

#include "audio/automation_track.h"
#include "audio/audio_bus_track.h"
#include "audio/channel.h"
#include "audio/instrument_track.h"
#include "audio/track.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/automation_point.h"
#include "gui/widgets/ruler.h"
#include "project.h"
#include "utils/ui.h"

G_DEFINE_TYPE (
  AutomationPointWidget,
  automation_point_widget,
  ARRANGER_OBJECT_WIDGET_TYPE)

static gboolean
draw_cb (
  GtkWidget * widget,
  cairo_t *   cr,
  AutomationPointWidget * self)
{
  GtkStyleContext *context;

  context =
    gtk_widget_get_style_context (widget);

  int width =
    gtk_widget_get_allocated_width (widget);
  int height =
    gtk_widget_get_allocated_height (widget);

  gtk_render_background (
    context, cr, 0, 0, width, height);

  Track * track =
    self->automation_point->region->at->track;

  /* get color */
  GdkRGBA color = track->color;
  ui_get_arranger_object_color (
    &color,
    gtk_widget_get_state_flags (
      GTK_WIDGET (self)) &
      GTK_STATE_FLAG_PRELIGHT,
    automation_point_is_selected (
      self->automation_point),
    automation_point_is_transient (
      self->automation_point));
  gdk_cairo_set_source_rgba (
    cr, &color);

  /* draw circle */
  cairo_arc (
    cr,
    width / 2, height / 2,
    width / 2 - AP_WIDGET_PADDING, 0,
    2 * G_PI);
  cairo_stroke_preserve(cr);
  cairo_fill(cr);

 return FALSE;
}

AutomationPointWidget *
automation_point_widget_new (
  AutomationPoint * ap)
{
  AutomationPointWidget * self =
    g_object_new (
      AUTOMATION_POINT_WIDGET_TYPE,
      "visible", 1,
      NULL);

  arranger_object_widget_setup (
    Z_ARRANGER_OBJECT_WIDGET (self),
    (ArrangerObject *) ap);

  self->automation_point = ap;

  return self;
}

static void
automation_point_widget_class_init (
  AutomationPointWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (
    klass, "automation-point");
}

static void
automation_point_widget_init (
  AutomationPointWidget * self)
{
  ARRANGER_OBJECT_WIDGET_GET_PRIVATE (self);

  /* connect signals */
  g_signal_connect (
    G_OBJECT (ao_prv->drawing_area), "draw",
    G_CALLBACK (draw_cb), self);
}
