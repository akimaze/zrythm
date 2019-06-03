/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "audio/midi.h"
#include "gui/backend/clip_editor.h"
#include "gui/backend/piano_roll.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/piano_roll_key.h"
#include "gui/widgets/piano_roll.h"
#include "project.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (PianoRollKeyWidget,
               piano_roll_key_widget,
               GTK_TYPE_DRAWING_AREA)

/* 1 = black */
static int notes[12] = {
    0,
    1,
    0,
    1,
    0,
    0,
    1,
    0,
    1,
    0,
    1,
    0 };

static gboolean
piano_roll_key_draw_cb (
  PianoRollKeyWidget * self,
  cairo_t *cr,
  gpointer data)
{
  guint width, height;
  GtkStyleContext *context;

  context =
    gtk_widget_get_style_context (GTK_WIDGET (self));

  width =
    gtk_widget_get_allocated_width (
      GTK_WIDGET (self));
  height =
    gtk_widget_get_allocated_height (
      GTK_WIDGET (self));

  gtk_render_background (
    context, cr, 0, 0, width, height);

  /* draw note */
  int black_note =
    notes[self->descr->value % 12] == 1;

  if (black_note)
    cairo_set_source_rgb (cr, 0, 0, 0);
  else
    cairo_set_source_rgb (cr, 1, 1, 1);
  cairo_rectangle (
    cr, 0, 0,
    width, height);
  cairo_fill (cr);

  /* add shade if currently pressed note */
  if (PIANO_ROLL->current_note &&
      PIANO_ROLL->current_note->value ==
        self->descr->value)
    {
      if (black_note)
        cairo_set_source_rgba (cr, 1, 1, 1, 0.1);
      else
        cairo_set_source_rgba (cr, 0, 0, 0, 0.3);
      cairo_rectangle (
        cr, 0, 0,
        width, height);
      cairo_fill (cr);
    }

 return FALSE;
}

/**
 * Send a note off.
 *
 * TODO Can be moved to a global function?
 *
 * @param on 1 if on, 0 if off.
 */
static void
send_note_event (
  PianoRollKeyWidget * self,
  int                  on)
{
#ifdef HAVE_JACK
  jack_midi_event_t * ev;
#endif

  /* add note off event */
#ifdef HAVE_JACK
  ev =
    &MANUAL_PRESS_QUEUE->jack_midi_events[
      MANUAL_PRESS_QUEUE->num_events];
  ev->time = 600;
  ev->size = 3;
  if (!ev->buffer)
    ev->buffer =
      calloc (3, sizeof (jack_midi_data_t));

  /* status byte */
  ev->buffer[0] =
    on ? MIDI_CH1_NOTE_ON : MIDI_CH1_NOTE_OFF;

  /* note number 0-127 */
  ev->buffer[1] = self->descr->value;

  /* velocity 0-127 */
  ev->buffer[2] = 90;
#endif

  MANUAL_PRESS_QUEUE->num_events++;

  gtk_widget_queue_draw (GTK_WIDGET (self));
}

static gboolean
on_leave (GtkWidget *widget,
          GdkEvent  *event,
          PianoRollKeyWidget * self)
{
  if (MW_PIANO_ROLL->note_pressed &&
      !MW_PIANO_ROLL->note_released)
    {
      send_note_event (self, 0);
    }
  return FALSE;
}

static gboolean
on_enter (GtkWidget *widget,
          GdkEvent  *event,
          PianoRollKeyWidget * self)
{
  if (MW_PIANO_ROLL->note_pressed &&
      !MW_PIANO_ROLL->note_released)
    {
      send_note_event (self, 1);
    }
  return FALSE;
}

void
on_released (
  GtkGestureMultiPress *gesture,
  gint                  n_press,
  gdouble               x,
  gdouble               y,
  PianoRollKeyWidget *  self)
{
  MW_PIANO_ROLL->note_pressed = 0;
  MW_PIANO_ROLL->note_released = 1;
  send_note_event (self, 0);
}

void
on_pressed (
  GtkGestureMultiPress *gesture,
  gint                  n_press,
  gdouble               x,
  gdouble               y,
  PianoRollKeyWidget *  self)
{
  MW_PIANO_ROLL->note_pressed = 1;
  MW_PIANO_ROLL->note_released = 0;
  send_note_event (self, 1);
}

/**
 * Creates a PianoRollKeyWidget for the given
 * MIDI note descriptor.
 */
PianoRollKeyWidget *
piano_roll_key_widget_new (
  MidiNoteDescriptor * descr)
{
  PianoRollKeyWidget * self =
    g_object_new (PIANO_ROLL_KEY_WIDGET_TYPE,
                  NULL);

  self->descr = descr;

  gtk_widget_set_visible (
    GTK_WIDGET (self), 1);

  gtk_widget_set_size_request (
    GTK_WIDGET (self), 48, MW_PIANO_ROLL->px_per_key);

  return self;
}

static void
piano_roll_key_widget_class_init (
  PianoRollKeyWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (
    klass, "piano-roll-key");
}

static void
piano_roll_key_widget_init (
  PianoRollKeyWidget * self)
{
  /* make it able to notify */
  gtk_widget_add_events (
    GTK_WIDGET (self),
    GDK_ALL_EVENTS_MASK);

  self->multipress =
    GTK_GESTURE_MULTI_PRESS (
      gtk_gesture_multi_press_new (
        GTK_WIDGET (self)));

  g_signal_connect (
    G_OBJECT (self), "draw",
    G_CALLBACK (piano_roll_key_draw_cb), NULL);
  g_signal_connect (
    G_OBJECT (self), "enter-notify-event",
    G_CALLBACK (on_enter), NULL);
  g_signal_connect (
    G_OBJECT (self), "leave-notify-event",
    G_CALLBACK (on_leave), NULL);
  g_signal_connect (
    G_OBJECT(self->multipress), "pressed",
    G_CALLBACK (on_pressed),  self);
  g_signal_connect (
    G_OBJECT(self->multipress), "released",
    G_CALLBACK (on_released),  self);
}