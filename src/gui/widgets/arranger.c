/*
 * gui/widgets/arranger.c - The timeline containing regions
 *
 * Copyright (C) 2018 Alexandros Theodotou
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

#include "zrythm_app.h"
#include "project.h"
#include "settings_manager.h"
#include "gui/widgets/region.h"
#include "audio/automation_track.h"
#include "audio/channel.h"
#include "audio/mixer.h"
#include "audio/track.h"
#include "audio/transport.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/automation_curve.h"
#include "gui/widgets/automation_point.h"
#include "gui/widgets/automation_track.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/inspector.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger_bg.h"
#include "gui/widgets/midi_editor.h"
#include "gui/widgets/midi_note.h"
#include "gui/widgets/piano_roll_labels.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_bg.h"
#include "gui/widgets/track.h"
#include "gui/widgets/tracklist.h"
#include "utils/arrays.h"
#include "utils/ui.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (ArrangerWidget, arranger_widget, GTK_TYPE_OVERLAY)

#define MW_RULER MAIN_WINDOW->ruler
#define T_MIDI self->type == ARRANGER_TYPE_MIDI
#define T_TIMELINE self->type == ARRANGER_TYPE_TIMELINE

/**
 * Sets up the MIDI editor for the given region.
 */
void
arranger_widget_set_channel (ArrangerWidget * self, Channel * channel)
{
  if (self->type == ARRANGER_TYPE_MIDI)
    {
      if (self->me_selected_channel)
        {
          channel_reattach_midi_editor_manual_press_port (
            self->me_selected_channel,
            0);
        }
      channel_reattach_midi_editor_manual_press_port (
        channel,
        1);
      self->me_selected_channel = channel;

      gtk_notebook_set_current_page (MAIN_WINDOW->bot_notebook, 0);

      char * label = g_strdup_printf ("%s",
                                      channel->name);
      gtk_label_set_text (MIDI_EDITOR->midi_name_label,
                          label);
      g_free (label);

      color_area_widget_set_color (MIDI_EDITOR->midi_track_color,
                                   &channel->color);

      /* remove all previous children and add new */
      GList *children, *iter;
      children = gtk_container_get_children(GTK_CONTAINER(self));
      for(iter = children; iter != NULL; iter = g_list_next(iter))
        {
          if (iter->data !=  self->bg)
            gtk_container_remove (GTK_CONTAINER (self),
                                  GTK_WIDGET (iter->data));
        }
      g_list_free(children);
      for (int i = 0; i < channel->track->num_regions; i++)
        {
          Region * region = channel->track->regions[i];
          for (int j = 0; j < region->num_midi_notes; j++)
            {
              gtk_overlay_add_overlay (GTK_OVERLAY (self),
                                       GTK_WIDGET (region->midi_notes[j]->widget));
            }
        }
      gtk_widget_queue_allocate (GTK_WIDGET (self));
      gtk_widget_show_all (GTK_WIDGET (self));

      GtkAdjustment * adj = gtk_scrolled_window_get_vadjustment (
                                      MIDI_EDITOR->piano_roll_arranger_scroll);
      gtk_adjustment_set_value (adj,
                                gtk_adjustment_get_upper (adj) / 2);
      gtk_scrolled_window_set_vadjustment (MIDI_EDITOR->piano_roll_arranger_scroll,
                                           adj);
    }
}

/**
 * Gets x position in pixels
 */
int
arranger_get_x_pos_in_px (Position * pos)
{
  return (pos->bars - 1) * MW_RULER->px_per_bar +
  (pos->beats - 1) * MW_RULER->px_per_beat +
  (pos->quarter_beats - 1) * MW_RULER->px_per_quarter_beat +
  pos->ticks * MW_RULER->px_per_tick +
  SPACE_BEFORE_START;
}

/**
 * Gets called to set the position/size of each overlayed widget.
 */
static gboolean
get_child_position (GtkOverlay   *overlay,
                    GtkWidget    *widget,
                    GdkRectangle *allocation,
                    gpointer      user_data)
{
  ArrangerWidget * self = (ArrangerWidget *) overlay;

  if (T_MIDI)
    {
      if (IS_MIDI_NOTE_WIDGET (widget))
        {
          MidiNoteWidget * midi_note_widget = MIDI_NOTE_WIDGET (widget);

          allocation->x = arranger_get_x_pos_in_px (&midi_note_widget->midi_note->start_pos);
          allocation->y = MAIN_WINDOW->midi_editor->piano_roll_labels->px_per_note *
            (127 - midi_note_widget->midi_note->val);
          allocation->width =
            arranger_get_x_pos_in_px (&midi_note_widget->midi_note->end_pos) -
            allocation->x;
          allocation->height = MAIN_WINDOW->midi_editor->piano_roll_labels->px_per_note;
        }
    }
  else if (T_TIMELINE)
    {
      if (IS_REGION_WIDGET (widget))
        {
          RegionWidget * rw = REGION_WIDGET (widget);

          gint wx, wy;
          gtk_widget_translate_coordinates(
                    GTK_WIDGET (rw->region->track->widget->track_box),
                    GTK_WIDGET (overlay),
                    0,
                    0,
                    &wx,
                    &wy);

          allocation->x = arranger_get_x_pos_in_px (&rw->region->start_pos);
          allocation->y = wy;
          allocation->width = arranger_get_x_pos_in_px (&rw->region->end_pos) -
            allocation->x;
          allocation->height =
            gtk_widget_get_allocated_height (
              GTK_WIDGET (rw->region->track->widget->track_box));
        }
      else if (IS_AUTOMATION_POINT_WIDGET (widget))
        {
          AutomationPointWidget * ap_widget = AUTOMATION_POINT_WIDGET (widget);
          AutomationPoint * ap = ap_widget->ap;
          /*Automatable * a = ap->at->automatable;*/

          gint wx, wy;
          gtk_widget_translate_coordinates(
                    GTK_WIDGET (ap->at->widget->at_grid),
                    GTK_WIDGET (overlay),
                    0,
                    0,
                    &wx,
                    &wy);

          allocation->x = arranger_get_x_pos_in_px (&ap->pos) -
            AP_WIDGET_POINT_SIZE / 2;
          allocation->y = (wy + automation_point_get_y_in_px (ap)) -
            AP_WIDGET_POINT_SIZE / 2;
          allocation->width = AP_WIDGET_POINT_SIZE;
          allocation->height = AP_WIDGET_POINT_SIZE;
        }
      else if (IS_AUTOMATION_CURVE_WIDGET (widget))
        {
          AutomationCurveWidget * acw = AUTOMATION_CURVE_WIDGET (widget);
          AutomationCurve * ac = acw->ac;
          /*Automatable * a = ap->at->automatable;*/

          gint wx, wy;
          gtk_widget_translate_coordinates(
                    GTK_WIDGET (ac->at->widget->at_grid),
                    GTK_WIDGET (overlay),
                    0,
                    0,
                    &wx,
                    &wy);
          AutomationPoint * prev_ap =
            automation_track_get_ap_before_curve (ac->at, ac);
          AutomationPoint * next_ap =
            automation_track_get_ap_after_curve (ac->at, ac);

          allocation->x = arranger_get_x_pos_in_px (&prev_ap->pos);
          int prev_y = automation_point_get_y_in_px (prev_ap);
          int next_y = automation_point_get_y_in_px (next_ap);
          allocation->y = wy + (prev_y > next_y ? next_y : prev_y);
          allocation->width =
            arranger_get_x_pos_in_px (&next_ap->pos) - allocation->x;
          allocation->height = prev_y > next_y ? prev_y - next_y : next_y - prev_y;
        }
    }


  return TRUE;
}

/**
 * For timeline use only.
 */
static Track *
get_track_at_y (double y)
{
  for (int i = 0; i < MIXER->num_channels; i++)
    {
      Track * track = MIXER->channels[i]->track;

      GtkAllocation allocation;
      gtk_widget_get_allocation (GTK_WIDGET (track->widget->track_automation_paned),
                                 &allocation);

      gint wx, wy;
      gtk_widget_translate_coordinates(
                GTK_WIDGET (MW_TIMELINE),
                GTK_WIDGET (track->widget->track_automation_paned),
                0,
                0,
                &wx,
                &wy);

      if (y > -wy && y < ((-wy) + allocation.height))
        {
          return track;
        }
    }
  return NULL;
}

/**
 * For timeline use only.
 */
static AutomationTrack *
get_automation_track_at_y (double y)
{
  for (int i = 0; i < MIXER->num_channels; i++)
    {
      Track * track = MIXER->channels[i]->track;

      for (int j = 0; j < track->num_automation_tracks; j++)
        {
          /*g_message ("at %d of %d", j, i);*/
          AutomationTrack * at = track->automation_tracks[j];
          if (at->widget)
            {
              GtkAllocation allocation;
              gtk_widget_get_allocation (GTK_WIDGET (at->widget->at_grid),
                                         &allocation);

              gint wx, wy;
              gtk_widget_translate_coordinates(
                        GTK_WIDGET (MW_TIMELINE),
                        GTK_WIDGET (at->widget->at_grid),
                        0,
                        y,
                        &wx,
                        &wy);

              if (wy >= 0 && wy <= allocation.height)
                {
                  return at;
                }
            }
        }
    }

  return NULL;
}

static int
get_note_at_y (double y)
{
  return 128 - y / PIANO_ROLL_LABELS->px_per_note;
}


static void
get_hit_widgets_in_range (ArrangerWidget *  self,
                          ArrangerChildType type,
                          double            start_x,
                          double            start_y,
                          double            offset_x,
                          double            offset_y,
                          GtkWidget **      array, ///< array to fill
                          int *             array_size) ///< array_size to fill
{
  GList *children, *iter;

  /* go through each overlay child */
  children = gtk_container_get_children (GTK_CONTAINER (self));
  for(iter = children; iter != NULL; iter = g_list_next (iter))
    {
      GtkWidget * widget = GTK_WIDGET (iter->data);

      GtkAllocation allocation;
      gtk_widget_get_allocation (widget,
                                 &allocation);

      gint wx, wy;
      gtk_widget_translate_coordinates(
                GTK_WIDGET (self),
                GTK_WIDGET (widget),
                start_x,
                start_y,
                &wx,
                &wy);

      int x_hit, y_hit;
      if (offset_x < 0)
        {
          x_hit = wx + offset_x <= allocation.width &&
            wx > 0;
        }
      else
        {
          x_hit = wx <= allocation.width &&
            wx + offset_x > 0;
        }
      if (!x_hit) continue;
      if (offset_y < 0)
        {
          y_hit = wy + offset_y <= allocation.height &&
            wy > 0;
        }
      else
        {
          y_hit = wy <= allocation.height &&
            wy + offset_y > 0;
        }
      if (!y_hit) continue;

      if (type == ARRANGER_CHILD_TYPE_MIDI_NOTE &&
          IS_MIDI_NOTE_WIDGET (widget))
        {
          array[(*array_size)++] = widget;
        }
      else if (type == ARRANGER_CHILD_TYPE_REGION &&
               IS_REGION_WIDGET (widget))
        {
          array[(*array_size)++] = widget;
        }
      else if (type == ARRANGER_CHILD_TYPE_AP &&
               IS_AUTOMATION_POINT_WIDGET (widget))
        {
          array[(*array_size)++] = widget;
        }
    }
}

static GtkWidget *
get_hit_widget (ArrangerWidget *  self,
                ArrangerChildType type,
                double            x,
                double            y)
{
  GList *children, *iter;

  /* go through each overlay child */
  children = gtk_container_get_children (GTK_CONTAINER (self));
  for(iter = children; iter != NULL; iter = g_list_next (iter))
    {
      GtkWidget * widget = GTK_WIDGET (iter->data);

      GtkAllocation allocation;
      gtk_widget_get_allocation (widget,
                                 &allocation);

      gint wx, wy;
      gtk_widget_translate_coordinates(
                GTK_WIDGET (self),
                GTK_WIDGET (widget),
                x,
                y,
                &wx,
                &wy);

      /* if hit */
      if (wx >= 0 &&
          wx <= allocation.width &&
          wy >= 0 &&
          wy <= allocation.height)
        {
          if (type == ARRANGER_CHILD_TYPE_MIDI_NOTE &&
              IS_MIDI_NOTE_WIDGET (widget))
            {
              return widget;
            }
          else if (type == ARRANGER_CHILD_TYPE_REGION &&
                   IS_REGION_WIDGET (widget))
            {
              return widget;
            }
          else if (type == ARRANGER_CHILD_TYPE_AP &&
                   IS_AUTOMATION_POINT_WIDGET (widget))
            {
              g_message ("wx %d wy %d", wx, wy);
              return widget;
            }
          else if (type == ARRANGER_CHILD_TYPE_AC &&
                   IS_AUTOMATION_CURVE_WIDGET (widget))
            {
              g_message ("wx %d wy %d", wx, wy);
              return widget;
            }
        }
    }
  return NULL;
}

static RegionWidget *
get_hit_region (ArrangerWidget *  self,
                double            x,
                double            y)
{
  GtkWidget * widget = get_hit_widget (self,
                                       ARRANGER_CHILD_TYPE_REGION,
                                       x,
                                       y);
  if (widget)
    {
      return REGION_WIDGET (widget);
    }
  return NULL;
}

static MidiNoteWidget *
get_hit_midi_note (ArrangerWidget *  self,
                   double            x,
                   double            y)
{
  GtkWidget * widget = get_hit_widget (self,
                                       ARRANGER_CHILD_TYPE_MIDI_NOTE,
                                       x,
                                       y);
  if (widget)
    {
      return MIDI_NOTE_WIDGET (widget);
    }
  return NULL;
}

static AutomationPointWidget *
get_hit_automation_point (ArrangerWidget *  self,
                          double            x,
                          double            y)
{
  GtkWidget * widget = get_hit_widget (self,
                                       ARRANGER_CHILD_TYPE_AP,
                                       x,
                                       y);
  if (widget)
    {
      return AUTOMATION_POINT_WIDGET (widget);
    }
  return NULL;
}

static AutomationCurveWidget *
get_hit_curve (ArrangerWidget * self, double x, double y)
{
  GtkWidget * widget = get_hit_widget (self,
                                       ARRANGER_CHILD_TYPE_AC,
                                       x,
                                       y);
  if (widget)
    {
      return AUTOMATION_CURVE_WIDGET (widget);
    }
  return NULL;
}

static void
update_inspector (ArrangerWidget *self)
{
  if (T_MIDI)
    {
      inspector_widget_show_selections (INSPECTOR_CHILD_MIDI,
                                        (void **) self->me_midi_notes,
                                     self->num_me_midi_notes);
    }
  else if (T_TIMELINE)
    {
      inspector_widget_show_selections (INSPECTOR_CHILD_REGION,
                                        (void **) self->tl_regions,
                                     self->num_tl_regions);
      inspector_widget_show_selections (INSPECTOR_CHILD_AP,
                                        (void **) self->tl_automation_points,
                                     self->num_tl_automation_points);
    }
}

/**
 * Selects the region.
 */
static void
toggle_select (ArrangerWidget *  self,
               ArrangerChildType type,
               void *            child,
               int               append)
{
  void ** array;
  int * num;
  if (type == ARRANGER_CHILD_TYPE_REGION)
    {
      array = (void **) self->tl_regions;
      num = &self->num_tl_regions;
    }
  else if (type == ARRANGER_CHILD_TYPE_AP)
    {
      array = (void **) self->tl_automation_points;
      num = &self->num_tl_automation_points;
    }
  else if (type == ARRANGER_CHILD_TYPE_MIDI_NOTE)
    {
      array = (void **) self->me_midi_notes;
      num = &self->num_me_midi_notes;
    }

  if (!append)
    {
      /* deselect existing selections */
      for (int i = 0; i < (*num); i++)
        {
          void * r = array[i];
          if (type == ARRANGER_CHILD_TYPE_REGION)
            {
              region_widget_select (((Region *)r)->widget, 0);
            }
        }
      *num = 0;
    }

  /* if already selected */
  if (array_contains (array,
                      *num,
                      child))
    {
      /* deselect */
      array_delete (array,
                    num,
                    child);
      if (type == ARRANGER_CHILD_TYPE_REGION)
        {
          region_widget_select (((Region *)child)->widget, 0);
        }
    }
  else /* not selected */
    {
      /* select */
      array_append (array,
                    num,
                    child);
      if (type == ARRANGER_CHILD_TYPE_REGION)
        {
          region_widget_select (((Region *)child)->widget, 1);
        }
    }
}

void
arranger_widget_select_all (ArrangerWidget *  self,
                                   int               select)
{
  if (T_MIDI)
    {

    }
  else if (T_TIMELINE)
    {
      self->num_tl_regions = 0;
      self->num_tl_automation_points = 0;

      for (int i = 0; i < MIXER->num_channels; i++)
        {
          Channel * chan = MIXER->channels[i];

          if (chan->visible)
            {
              for (int j = 0; j < chan->track->num_regions; j++)
                {
                  Region * r = chan->track->regions[j];

                  region_widget_select (r->widget, select);

                  if (select)
                    {
                      /* select  */
                      array_append ((void **)self->tl_regions,
                                    &self->num_tl_regions,
                                    r);
                    }
                }
              if (chan->track->automations_visible)
                {
                  for (int j = 0; j < chan->track->num_automation_tracks; j++)
                    {
                      AutomationTrack * at = chan->track->automation_tracks[j];
                      if (at->visible)
                        {
                          for (int k = 0; k < at->num_automation_points; k++)
                            {
                              /*AutomationPoint * ap = at->automation_points[k];*/

                              /*ap_widget_select (ap->widget, select);*/

                              /*if (select)*/
                                /*{*/
                                  /*[> select  <]*/
                                  /*array_append ((void **)self->tl_automation_points,*/
                                                /*&self->num_tl_automation_points,*/
                                                /*ap);*/
                                /*}*/
                            }
                        }
                    }
                }
            }
        }
    }
  update_inspector (self);
}

/**
 * Selects the region.
 */
void
arranger_widget_toggle_select_region (ArrangerWidget * self,
                                      Region         * region,
                                      int              append)
{
  toggle_select (self,
                 ARRANGER_CHILD_TYPE_REGION,
                 (void *) region,
                 append);
}

void
arranger_widget_toggle_select_automation_point (ArrangerWidget * self,
                                      AutomationPoint         * ap,
                                      int              append)
{
  toggle_select (self,
                                 ARRANGER_CHILD_TYPE_AP,
                                 (void *) ap,
                                 append);
}

void
arranger_widget_toggle_select_midi_note (ArrangerWidget * self,
                                      MidiNote         * midi_note,
                                      int              append)
{
  toggle_select (self,
                                 ARRANGER_CHILD_TYPE_MIDI_NOTE,
                                 (void *) midi_note,
                                 append);
}

static void
show_context_menu ()
{
  GtkWidget *menu, *menuitem;

  menu = gtk_menu_new();

  menuitem = gtk_menu_item_new_with_label("Do something");

  /*g_signal_connect(menuitem, "activate",*/
                   /*(GCallback) view_popup_menu_onDoSomething, treeview);*/

  gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

  gtk_widget_show_all(menu);

  gtk_menu_popup_at_pointer (GTK_MENU(menu), NULL);
}

static void
on_right_click (GtkGestureMultiPress *gesture,
               gint                  n_press,
               gdouble               x,
               gdouble               y,
               gpointer              user_data)
{
  /*ArrangerWidget * self = (ArrangerWidget *) user_data;*/

  if (n_press == 1)
    {
      show_context_menu ();

    }
}

static gboolean
on_key_action (GtkWidget *widget,
               GdkEventKey  *event,
               gpointer   user_data)
{
  ArrangerWidget * self = (ArrangerWidget *) user_data;

  if (event->state & GDK_CONTROL_MASK &&
      event->type == GDK_KEY_PRESS &&
      event->keyval == GDK_KEY_a)
    {
      arranger_widget_select_all (self, 1);
    }

  return FALSE;
}

/**
 * On button press.
 *
 * This merely sets the number of clicks. It is always called
 * before drag_begin, so the logic is done in drag_begin.
 */
static void
multipress_pressed (GtkGestureMultiPress *gesture,
               gint                  n_press,
               gdouble               x,
               gdouble               y,
               gpointer              user_data)
{
  ArrangerWidget * self = (ArrangerWidget *) user_data;
  self->n_press = n_press;
}

static void
drag_begin (GtkGestureDrag * gesture,
               gdouble         start_x,
               gdouble         start_y,
               gpointer        user_data)
{
  ArrangerWidget * self = (ArrangerWidget *) user_data;
  self->start_x = start_x;
  self->start_y = start_y;

  if (!gtk_widget_has_focus (GTK_WIDGET (self)))
    gtk_widget_grab_focus (GTK_WIDGET (self));

  GdkEventSequence *sequence =
    gtk_gesture_single_get_current_sequence (GTK_GESTURE_SINGLE (gesture));
  /*guint button =*/
    /*gtk_gesture_single_get_current_button (GTK_GESTURE_SINGLE (gesture));*/
  const GdkEvent * event =
    gtk_gesture_get_last_event (GTK_GESTURE (gesture), sequence);
  GdkModifierType state_mask;
  gdk_event_get_state (event, &state_mask);

  MidiNoteWidget * midi_note_widget = T_MIDI ?
    get_hit_midi_note (self, start_x, start_y) :
    NULL;
  RegionWidget * rw = T_TIMELINE ?
    get_hit_region (self, start_x, start_y) :
    NULL;
  AutomationCurveWidget * ac_widget = T_TIMELINE ?
    get_hit_curve (self, start_x, start_y) :
    NULL;
  AutomationPointWidget * ap_widget = T_TIMELINE ?
    get_hit_automation_point (self, start_x, start_y) :
    NULL;
  int is_hit = midi_note_widget || rw || ac_widget || ap_widget;
  if (is_hit)
    {
      /* set selections, positions, actions, cursor */
      if (midi_note_widget)
        {
          MidiNote * midi_note = midi_note_widget->midi_note;
          self->start_pos_px = ruler_widget_pos_to_px (&midi_note->start_pos);
          position_set_to_pos (&self->start_pos, &midi_note->start_pos);
          position_set_to_pos (&self->end_pos, &midi_note->end_pos);

          /* if already in selected notes, prepare to do action on all of them,
           * otherwise change selection to just this note */
          self->me_start_midi_note = midi_note;
          if (!array_contains ((void **) self->me_midi_notes,
                               self->num_me_midi_notes,
                               (void *) midi_note))
            {
              self->me_midi_notes[0] = midi_note;
              self->num_me_midi_notes = 1;
            }
          switch (midi_note_widget->state)
            {
            case MNW_STATE_NONE:
              g_warning ("hitting midi note but midi note hover state is none, should be fixed");
              break;
            case MNW_STATE_RESIZE_L:
              self->action = ARRANGER_ACTION_RESIZING_L;
              break;
            case MNW_STATE_RESIZE_R:
              self->action = ARRANGER_ACTION_RESIZING_R;
              break;
            case MNW_STATE_HOVER:
            case MNW_STATE_SELECTED:
              self->action = ARRANGER_ACTION_STARTING_MOVING;
              ui_set_cursor (GTK_WIDGET (midi_note_widget), "grabbing");
              break;
            }
        }
      else if (rw)
        {
          /* open MIDI editor */
          if (self->n_press > 0)
            {
              arranger_widget_set_channel(
                          MIDI_EDITOR->midi_arranger,
                          rw->region->track->channel);
            }

          Region * region = rw->region;
          MIDI_ARRANGER->me_selected_region = region;
          self->tl_start_region = rw->region;

          /* update arranger action */
          if (!rw->hover)
            g_warning ("hitting region but region hover state is none, should be fixed");
          else if (rw->cursor_state == RWS_CURSOR_RESIZE_L)
            self->action = ARRANGER_ACTION_RESIZING_L;
          else if (rw->cursor_state == RWS_CURSOR_RESIZE_R)
            self->action = ARRANGER_ACTION_RESIZING_R;
          else if (rw->hover)
            {
              self->action = ARRANGER_ACTION_STARTING_MOVING;
              ui_set_cursor (GTK_WIDGET (rw), "grabbing");
            }

          /* select/ deselect regions */
          if (state_mask & GDK_SHIFT_MASK ||
              state_mask & GDK_CONTROL_MASK)
            {
              /* if ctrl pressed toggle on/off */
              arranger_widget_toggle_select_region (self, region, 1);

            }
          else if (!array_contains ((void **)self->tl_regions,
                              self->num_tl_regions,
                              region))
            {
              /* else if not already selected select only it */
              arranger_widget_select_all (self, 0);
              arranger_widget_toggle_select_region (self, region, 0);
            }

          /* find highest and lowest selected regions */
          self->tl_top_region = self->tl_regions[0];
          self->tl_bot_region = self->tl_regions[0];
          FOREACH_TL_R
            {
              Region * region = self->tl_regions[i];
              if (region->track->channel->id <
                  self->tl_top_region->track->channel->id)
                {
                  self->tl_top_region = region;
                }
              if (region->track->channel->id >
                  self->tl_bot_region->track->channel->id)
                {
                  self->tl_bot_region = region;
                }
            }
        }
      else if (ap_widget)
        {
          AutomationPoint * ap = ap_widget->ap;
          self->start_pos_px = start_x;
          self->tl_start_ap = ap;
          if (!array_contains ((void **) self->tl_automation_points,
                               self->num_tl_automation_points,
                               (void *) ap))
            {
              self->tl_automation_points[0] = ap;
              self->num_tl_automation_points = 1;
            }
          switch (ap_widget->state)
            {
            case APW_STATE_NONE:
              g_warning ("hitting AP but AP hover state is none, should be fixed");
              break;
            case APW_STATE_HOVER:
            case APW_STATE_SELECTED:
              self->action = ARRANGER_ACTION_STARTING_MOVING;
              ui_set_cursor (GTK_WIDGET (ap_widget), "grabbing");
              break;
            }

          /* update selection */
          if (state_mask & GDK_SHIFT_MASK ||
              state_mask & GDK_CONTROL_MASK)
            {
              arranger_widget_toggle_select_automation_point (self, ap, 1);
            }
          else
            {
              arranger_widget_select_all (self, 0);
              arranger_widget_toggle_select_automation_point (self, ap, 0);
            }
        }
      else if (ac_widget)
        {
          self->tl_start_ac = ac_widget->ac;
        }

      /* find start pos */
      position_init (&self->start_pos);
      position_set_bar (&self->start_pos, 2000);
      if (T_TIMELINE)
        {
          FOREACH_TL_R
            {
              Region * r = self->tl_regions[i];
              if (position_compare (&r->start_pos,
                                    &self->start_pos) <= 0)
                {
                  position_set_to_pos (&self->start_pos,
                                       &r->start_pos);
                }

              /* set start poses fo regions */
              position_set_to_pos (&self->tl_region_start_poses[i],
                                   &r->start_pos);
            }
          FOREACH_TL_AP
            {
              AutomationPoint * ap = self->tl_automation_points[i];
              if (position_compare (&ap->pos,
                                    &self->start_pos) <= 0)
                {
                  position_set_to_pos (&self->start_pos,
                                       &ap->pos);
                }

              /* set start poses for APs */
              position_set_to_pos (&self->tl_ap_poses[i],
                                   &ap->pos);
            }
        }


    }
  else /* no note hit */
    {
      if (self->n_press == 1)
        {
          /* area selection */
          self->action = ARRANGER_ACTION_STARTING_SELECTION;

          /* deselect all */
          arranger_widget_select_all (self, 0);
        }
      else if (self->n_press == 2)
        {
          Position pos;
          ruler_widget_px_to_pos (
                               &pos,
                               start_x - SPACE_BEFORE_START);

          Track * track = NULL;
          AutomationTrack * at = NULL;
          int note;
          Region * region = NULL;
          if (T_TIMELINE)
            {
              at = get_automation_track_at_y (start_y);
              if (!at)
                track = get_track_at_y (start_y);
            }
          else if (T_MIDI)
            {
              note = (MIDI_EDITOR->piano_roll_labels->total_px - start_y) /
                MIDI_EDITOR->piano_roll_labels->px_per_note;

              /* if inside a region */
              if (self->me_selected_channel)
                {
                  region = region_at_position (
                    self->me_selected_channel->track,
                    &pos);
                }
            }
          if (((T_TIMELINE && track) || (T_TIMELINE && at)) ||
              (T_MIDI && region))
            {
              if (T_TIMELINE && at)
                {
                  position_snap (NULL,
                                 &pos,
                                 track,
                                 NULL,
                                 self->snap_grid);

                  /* if the automatable is float in this automation track */
                  if (automatable_is_float (at->automatable))
                    {
                      /* add automation point to automation track */
                      float value = automation_track_widget_get_fvalue_at_y (
                                                      at->widget,
                                                      start_y);

                      AutomationPoint * ap = automation_point_new_float (at,
                                                             value,
                                                             &pos);
                      automation_track_add_automation_point (at,
                                                             ap,
                                                             GENERATE_CURVE_POINTS);
                      gtk_overlay_add_overlay (GTK_OVERLAY (self),
                                               GTK_WIDGET (ap->widget));
                      gtk_widget_show (GTK_WIDGET (ap->widget));
                      self->tl_automation_points[0] = ap;
                      self->num_tl_automation_points = 1;
                    }
                }
              else if (T_TIMELINE && track)
                {
                  position_snap (NULL,
                                 &pos,
                                 track,
                                 NULL,
                                 self->snap_grid);
                  Region * region = region_new (track,
                                             &pos,
                                             &pos);
                  position_set_min_size (&region->start_pos,
                                         &region->end_pos,
                                         self->snap_grid);
                  track_add_region (track,
                                    region);
                  gtk_overlay_add_overlay (GTK_OVERLAY (self),
                                           GTK_WIDGET (region->widget));
                  gtk_widget_show (GTK_WIDGET (region->widget));
                  self->action = ARRANGER_ACTION_RESIZING_R;
                  self->tl_regions[0] = region;
                  self->num_tl_regions = 1;
                }
              else if (T_MIDI && region)
                {
                  position_snap (NULL,
                                 &pos,
                                 NULL,
                                 region,
                                 self->snap_grid);
                  MidiNote * midi_note = midi_note_new (region,
                                                   &pos,
                                                   &pos,
                                                   note,
                                                   -1);
                  position_set_min_size (&midi_note->start_pos,
                                         &midi_note->end_pos,
                                         self->snap_grid);
                  region_add_midi_note (region,
                                        midi_note);
                  gtk_overlay_add_overlay (GTK_OVERLAY (self),
                                           GTK_WIDGET (midi_note->widget));
                  gtk_widget_show (GTK_WIDGET (midi_note->widget));
                  self->action = ARRANGER_ACTION_RESIZING_R;
                  self->me_midi_notes[0] = midi_note;
                  self->num_me_midi_notes = 1;
                }
              gtk_widget_queue_allocate (GTK_WIDGET (self));
            }
        }
    }

  /* update inspector */
  update_inspector (self);
}

static void
drag_update (GtkGestureDrag * gesture,
               gdouble         offset_x,
               gdouble         offset_y,
               gpointer        user_data)
{
  ArrangerWidget * self = ARRANGER_WIDGET (user_data);

  /* set action to selecting if starting selection. this is because
   * drag_update never gets called if it's just a click, so we can check at
   * drag_end and see if anything was selected */
  if (self->action == ARRANGER_ACTION_STARTING_SELECTION)
    {
      self->action = ARRANGER_ACTION_SELECTING;
    }
  else if (self->action == ARRANGER_ACTION_STARTING_MOVING)
    {
      self->action = ARRANGER_ACTION_MOVING;
    }

  /* if drawing a selection */
  if (self->action == ARRANGER_ACTION_SELECTING)
    {
      /* deselect all */
      arranger_widget_select_all (self, 0);

      if (T_TIMELINE)
        {
          /* find enclosed regions */
          GtkWidget *    region_widgets[800];
          int            num_region_widgets = 0;
          get_hit_widgets_in_range (self,
                                    ARRANGER_CHILD_TYPE_REGION,
                                    self->start_x,
                                    self->start_y,
                                    offset_x,
                                    offset_y,
                                    region_widgets,
                                    &num_region_widgets);


          /* select the enclosed regions */
          for (int i = 0; i < num_region_widgets; i++)
            {
              RegionWidget * rw = REGION_WIDGET (region_widgets[i]);
              Region * region = rw->region;
              arranger_widget_toggle_select_region (self,
                                                    region,
                                                    1);
            }

          /* find enclosed automation_points */
          GtkWidget *    ap_widgets[800];
          int            num_ap_widgets = 0;
          get_hit_widgets_in_range (self,
                                    ARRANGER_CHILD_TYPE_AP,
                                    self->start_x,
                                    self->start_y,
                                    offset_x,
                                    offset_y,
                                    ap_widgets,
                                    &num_ap_widgets);

          /* select the enclosed automation_points */
          for (int i = 0; i < num_ap_widgets; i++)
            {
              AutomationPointWidget * ap_widget =
                AUTOMATION_POINT_WIDGET (ap_widgets[i]);
              AutomationPoint * ap = ap_widget->ap;
              arranger_widget_toggle_select_automation_point (self,
                                                              ap,
                                                              1);
            }
        }
      else if (T_MIDI)
        {
          /* find enclosed midi notes */
          GtkWidget *    midi_note_widgets[800];
          int            num_midi_note_widgets = 0;
          get_hit_widgets_in_range (self,
                                    ARRANGER_CHILD_TYPE_MIDI_NOTE,
                                    self->start_x,
                                    self->start_y,
                                    offset_x,
                                    offset_y,
                                    midi_note_widgets,
                                    &num_midi_note_widgets);

          /* select the enclosed midi_notes */
          for (int i = 0; i < num_midi_note_widgets; i++)
            {
              MidiNoteWidget * midi_note_widget =
                MIDI_NOTE_WIDGET (midi_note_widgets[i]);
              MidiNote * midi_note = midi_note_widget->midi_note;
              self->me_midi_notes[self->num_me_midi_notes++] = midi_note;
              arranger_widget_toggle_select_midi_note (self,
                                                       midi_note,
                                                       1);
            }
        }
    } /* endif ARRANGER_ACTION_SELECTING */

  /* handle x */
  else if (self->action == ARRANGER_ACTION_RESIZING_L)
    {
      Position pos;
      ruler_widget_px_to_pos (&pos,
                 (self->start_x + offset_x) - SPACE_BEFORE_START);
      if (T_TIMELINE)
        {
          for (int i = 0; i < self->num_tl_regions; i++)
            {
              Region * region = self->tl_regions[i];
              position_snap (NULL,
                             &pos,
                             region->track,
                             NULL,
                             self->snap_grid);
              region_set_start_pos (region,
                                    &pos,
                                    0);
            }
        }
      else if (T_MIDI)
        {
          for (int i = 0; i < self->num_me_midi_notes; i++)
            {
              MidiNote * midi_note = self->me_midi_notes[i];
              position_snap (NULL,
                             &pos,
                             NULL,
                             midi_note->region,
                             self->snap_grid);
              midi_note_set_start_pos (midi_note,
                                    &pos);
            }
        }
    } /* endif RESIZING_L */
  else if (self->action == ARRANGER_ACTION_RESIZING_R)
    {
      Position pos;
      ruler_widget_px_to_pos (&pos,
                 (self->start_x + offset_x) - SPACE_BEFORE_START);
      if (T_TIMELINE)
        {
          for (int i = 0; i < self->num_tl_regions; i++)
            {
              Region * region = self->tl_regions[i];
              position_snap (NULL,
                             &pos,
                             region->track,
                             NULL,
                             self->snap_grid);
              if (position_compare (&pos, &region->start_pos) > 0)
                {
                  region_set_end_pos (region,
                                      &pos);
                }
            }
        }
      else if (T_MIDI)
        {
          for (int i = 0; i < self->num_me_midi_notes; i++)
            {
              MidiNote * midi_note = self->me_midi_notes[i];
              position_snap (NULL,
                             &pos,
                             NULL,
                             midi_note->region,
                             self->snap_grid);
              if (position_compare (&pos, &midi_note->start_pos) > 0)
                {
                  midi_note_set_end_pos (midi_note,
                                        &pos);
                }
            }
        }
    } /*endif RESIZING_R */

  /* if moving the selection */
  else if (self->action == ARRANGER_ACTION_MOVING)
    {
      Position diff_pos;
      ruler_widget_px_to_pos (&diff_pos,
                              offset_x);
      int frames_diff = position_to_frames (&diff_pos);
      Position new_start_pos;
      position_set_to_pos (&new_start_pos, &self->start_pos);
      position_add_frames (&new_start_pos, frames_diff);
      position_snap (NULL,
                     &new_start_pos,
                     NULL,
                     NULL,
                     self->snap_grid);
      position_print (&new_start_pos);
      frames_diff = position_to_frames (&new_start_pos) -
        position_to_frames (&self->start_pos);

      if (T_TIMELINE)
        {
          /* update region positions */
          FOREACH_TL_R
            {
              Region * r = self->tl_regions[i];
              Position * prev_start_pos = &self->tl_region_start_poses[i];
              int length_frames = position_to_frames (&r->end_pos) -
                position_to_frames (&r->start_pos);
              Position tmp;
              position_set_to_pos (&tmp, prev_start_pos);
              position_add_frames (&tmp, frames_diff + length_frames);
              region_set_end_pos (r, &tmp);
              position_set_to_pos (&tmp, prev_start_pos);
              position_add_frames (&tmp, frames_diff);
              region_set_start_pos (r, &tmp, 1);
            }

          /* update ap positions */
          FOREACH_TL_AP
            {
              for (int i = 0; i < self->num_tl_automation_points; i++)
                {
                  AutomationPoint * ap = self->tl_automation_points[i];

                  /* get prev and next value APs */
                  AutomationPoint * prev_ap = automation_track_get_prev_ap (ap->at,
                                                                            ap);
                  AutomationPoint * next_ap = automation_track_get_next_ap (ap->at,
                                                                            ap);
                  /* get adjusted pos for this automation point */
                  Position ap_pos;
                  Position * prev_pos = &self->tl_ap_poses[i];
                  position_set_to_pos (&ap_pos,
                                       prev_pos);
                  position_add_frames (&ap_pos, frames_diff);

                  Position mid_pos;
                  AutomationCurve * ac;

                  /* update midway points */
                  if (prev_ap && position_compare (&ap_pos, &prev_ap->pos) >= 0)
                    {
                      /* set prev curve point to new midway pos */
                      position_get_midway_pos (&prev_ap->pos,
                                               &ap_pos,
                                               &mid_pos);
                      ac = automation_track_get_next_curve_ac (ap->at,
                                                               prev_ap);
                      position_set_to_pos (&ac->pos, &mid_pos);

                      /* set pos for ap */
                      if (!next_ap)
                        {
                          position_set_to_pos (&ap->pos, &ap_pos);
                        }
                    }
                  if (next_ap && position_compare (&ap_pos, &next_ap->pos) <= 0)
                    {
                      /* set next curve point to new midway pos */
                      position_get_midway_pos (&ap_pos,
                                               &next_ap->pos,
                                               &mid_pos);
                      ac = automation_track_get_next_curve_ac (ap->at,
                                                               ap);
                      position_set_to_pos (&ac->pos, &mid_pos);

                      /* set pos for ap - if no prev ap exists or if the position
                       * is also after the prev ap */
                      if ((prev_ap &&
                            position_compare (&ap_pos, &prev_ap->pos) >= 0) ||
                          (!prev_ap))
                        {
                          position_set_to_pos (&ap->pos, &ap_pos);
                        }
                    }
                  else if (!prev_ap && !next_ap)
                    {
                      /* set pos for ap */
                      position_set_to_pos (&ap->pos, &ap_pos);
                    }
                }
            }
        }
      else if (T_MIDI)
        {
          /* snap first selected midi note's pos */
          /*position_snap (NULL,*/
                         /*&pos,*/
                         /*NULL,*/
                         /*self->me_start_midi_note->region,*/
                         /*self->snap_grid);*/
          /*for (int i = 0; i < self->num_me_midi_notes; i++)*/
            /*{*/
              /*MidiNote * midi_note = self->me_midi_notes[i];*/

              /*[> get adjusted pos for this midi note <]*/
              /*Position midi_note_pos;*/
              /*position_set_to_pos (&midi_note_pos,*/
                                   /*&pos);*/
              /*int diff = position_to_frames (&midi_note->start_pos) -*/
                /*position_to_frames (&self->me_start_midi_note->start_pos);*/
              /*position_add_frames (&midi_note_pos, diff);*/

              /*midi_note_set_start_pos (midi_note,*/
                                    /*&midi_note_pos);*/
            /*}*/
        }
      /*int diff = position_to_frames (&pos) - position_to_frames (&self->start_pos);*/
      /*position_set_to_pos (&pos, &self->end_pos);*/
      /*position_add_frames (&pos,*/
                           /*diff);*/

      /* handle y */
      if (T_TIMELINE)
        {
          if (self->tl_start_region)
            {
              /* check if should be moved to new track */
              Track * track = get_track_at_y (self->start_y + offset_y);
              Track * old_track = self->tl_start_region->track;
              if (track)
                {
                  Track * pt = tracklist_widget_get_prev_visible_track (old_track);
                  Track * nt = tracklist_widget_get_next_visible_track (old_track);
                  Track * tt = tracklist_widget_get_top_track ();
                  Track * bt = tracklist_widget_get_bot_track ();
                  if (self->tl_start_region->track != track)
                    {
                      /* if new track is lower and bot region is not at the lowest track */
                      if (track == nt &&
                          self->tl_bot_region->track != bt)
                        {
                          /* shift all selected regions to their next track */
                          FOREACH_TL_R
                            {
                              Region * region = self->tl_regions[i];
                              nt = tracklist_widget_get_next_visible_track (region->track);
                              old_track = region->track;
                              track_remove_region (old_track, region);
                              track_add_region (nt, region);
                            }
                        }
                      else if (track == pt &&
                               self->tl_top_region->track != tt)
                        {
                          g_message ("track %s top region track %s tt %s",
                                     track->channel->name,
                                     self->tl_top_region->track->channel->name,
                                     tt->channel->name);
                          /* shift all selected regions to their prev track */
                          FOREACH_TL_R
                            {
                              Region * region = self->tl_regions[i];
                              pt = tracklist_widget_get_prev_visible_track (region->track);
                              old_track = region->track;
                              track_remove_region (old_track, region);
                              track_add_region (pt, region);
                            }
                        }
                    }
                }
            }
          else if (self->tl_start_ap)
            {
              for (int i = 0; i < self->num_tl_automation_points; i++)
                {
                  AutomationPoint * ap = self->tl_automation_points[i];

                  /* get adjusted y for this ap */
                  /*Position region_pos;*/
                  /*position_set_to_pos (&region_pos,*/
                                       /*&pos);*/
                  /*int diff = position_to_frames (&region->start_pos) -*/
                    /*position_to_frames (&self->tl_start_region->start_pos);*/
                  /*position_add_frames (&region_pos, diff);*/
                  int this_y =
                    automation_track_widget_get_y (ap->at->widget,
                                                   ap->widget);
                  int start_ap_y =
                    automation_track_widget_get_y (self->tl_start_ap->at->widget,
                                                   self->tl_start_ap->widget);
                  int diff = this_y - start_ap_y;

                  float fval =
                    automation_track_widget_get_fvalue_at_y (ap->at->widget,
                                                             self->start_y + offset_y + diff);
                  automation_point_update_fvalue (ap, fval);
                }
            }
        }
      else if (T_MIDI)
        {
          for (int i = 0; i < self->num_me_midi_notes; i++)
            {
              MidiNote * midi_note = self->me_midi_notes[i];
              /*midi_note_set_end_pos (midi_note,*/
                                     /*&pos);*/
              /* check if should be moved to new note  */
              midi_note->val = get_note_at_y (self->start_y + offset_y);
            }
        }
    } /* endif MOVING */

  gtk_widget_queue_allocate(GTK_WIDGET (self));
  if (T_MIDI)
    {
      gtk_widget_queue_draw (GTK_WIDGET (self));
      gtk_widget_show_all (GTK_WIDGET (self));
    }
  else if (T_TIMELINE)
    {
      gtk_widget_queue_allocate (GTK_WIDGET (MIDI_EDITOR->midi_arranger));
    }
  self->last_offset_x = offset_x;
  self->last_offset_y = offset_y;

  /* update inspector */
  update_inspector (self);
}

static void
drag_end (GtkGestureDrag *gesture,
               gdouble         offset_x,
               gdouble         offset_y,
               gpointer        user_data)
{
  ArrangerWidget * self = (ArrangerWidget *) user_data;
  self->start_x = 0;
  self->start_y = 0;
  self->last_offset_x = 0;
  self->last_offset_y = 0;
  for (int i = 0; i < self->num_me_midi_notes; i++)
    {
      MidiNote * midi_note = self->me_midi_notes[i];
      ui_set_cursor (GTK_WIDGET (midi_note->widget), "default");
    }
  self->me_start_midi_note = NULL;
  for (int i = 0; i < self->num_tl_regions; i++)
    {
      Region * region = self->tl_regions[i];
      ui_set_cursor (GTK_WIDGET (region->widget), "default");
    }

  /* if clicked on something, or clicked or nothing */
  if (self->action == ARRANGER_ACTION_STARTING_SELECTION ||
      self->action == ARRANGER_ACTION_STARTING_MOVING)
    {
      /* if clicked on something */
      if (self->action == ARRANGER_ACTION_STARTING_MOVING)
        {
          /*set_state_and_redraw_tl_regions (self, RW_STATE_SELECTED);*/
          /*set_state_and_redraw_tl_automation_points (self,*/
            /*APW_STATE_SELECTED);*/
          /*set_state_and_redraw_me_midi_notes (self,*/
            /*MNW_STATE_SELECTED);*/
        }

      /* else if clicked on nothing */
      else if (self->action == ARRANGER_ACTION_STARTING_SELECTION)
        {
          arranger_widget_select_all (self, 0);
        }
    }

  /* if didn't click on something */
  if (self->action != ARRANGER_ACTION_STARTING_MOVING)
    {
      self->tl_start_region = NULL;
      self->tl_start_ap = NULL;
      self->me_start_midi_note = NULL;
    }

  self->action = ARRANGER_ACTION_NONE;
  gtk_widget_queue_draw (GTK_WIDGET (self->bg));
}


ArrangerWidget *
arranger_widget_new (ArrangerWidgetType type, SnapGrid * snap_grid)
{
  g_message ("Creating arranger %d...", type);
  ArrangerWidget * self = g_object_new (ARRANGER_WIDGET_TYPE, NULL);
  self->snap_grid = snap_grid;
  self->type = type;
  if (T_MIDI)
    {
      MAIN_WINDOW->midi_editor->midi_arranger = self;
      self->bg = GTK_DRAWING_AREA (midi_arranger_bg_widget_new ());
    }
  else if (T_TIMELINE)
    {
      MAIN_WINDOW->timeline = self;
      self->bg = GTK_DRAWING_AREA (timeline_bg_widget_new ());
    }



  gtk_container_add (GTK_CONTAINER (self),
                     GTK_WIDGET (self->bg));

  /* make it able to notify */
  gtk_widget_add_events (GTK_WIDGET (self), GDK_ALL_EVENTS_MASK);
  gtk_widget_set_can_focus (GTK_WIDGET (self),
                           1);
  gtk_widget_set_focus_on_click (GTK_WIDGET (self),
                                 1);

  g_signal_connect (G_OBJECT (self), "get-child-position",
                    G_CALLBACK (get_child_position), NULL);
  g_signal_connect (G_OBJECT(self->drag), "drag-begin",
                    G_CALLBACK (drag_begin),  self);
  g_signal_connect (G_OBJECT(self->drag), "drag-update",
                    G_CALLBACK (drag_update),  self);
  g_signal_connect (G_OBJECT(self->drag), "drag-end",
                    G_CALLBACK (drag_end),  self);
  g_signal_connect (G_OBJECT (self->multipress), "pressed",
                    G_CALLBACK (multipress_pressed), self);
  g_signal_connect (G_OBJECT (self->right_mouse_mp), "pressed",
                    G_CALLBACK (on_right_click), self);
  g_signal_connect (G_OBJECT (self), "key-press-event",
                    G_CALLBACK (on_key_action), self);
  g_signal_connect (G_OBJECT (self), "key-release-event",
                    G_CALLBACK (on_key_action), self);

  return self;
}

static void
arranger_widget_class_init (ArrangerWidgetClass * klass)
{
}

static void
arranger_widget_init (ArrangerWidget *self )
{
  self->drag = GTK_GESTURE_DRAG (
                gtk_gesture_drag_new (GTK_WIDGET (self)));
  self->multipress = GTK_GESTURE_MULTI_PRESS (
                gtk_gesture_multi_press_new (GTK_WIDGET (self)));
  self->right_mouse_mp = GTK_GESTURE_MULTI_PRESS (
                gtk_gesture_multi_press_new (GTK_WIDGET (self)));
  gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (self->right_mouse_mp),
                                 GDK_BUTTON_SECONDARY);
}

/**
 * Draws the selection in its background.
 *
 * Should only be called by the bg widgets when drawing.
 */
void
arranger_bg_draw_selections (ArrangerWidget * self,
                             cairo_t *        cr)
{
  double offset_x, offset_y;
  offset_x = self->start_x + self->last_offset_x > 0 ?
    self->last_offset_x :
    1 - self->start_x;
  offset_y = self->start_y + self->last_offset_y > 0 ?
    self->last_offset_y :
    1 - self->start_y;
  if (self->action == ARRANGER_ACTION_SELECTING)
    {
      cairo_set_source_rgba (cr, 0.9, 0.9, 0.9, 1.0);
      cairo_rectangle (cr,
                       self->start_x,
                       self->start_y,
                       offset_x,
                       offset_y);
      cairo_stroke (cr);
      cairo_set_source_rgba (cr, 0.3, 0.3, 0.3, 0.3);
      cairo_rectangle (cr,
                       self->start_x,
                       self->start_y,
                       offset_x,
                       offset_y);
      cairo_fill (cr);
    }
}

