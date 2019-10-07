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
 * Code related to Carla plugins.
 */

#include "config.h"

#ifdef HAVE_CARLA_NATIVE_PLUGIN

#ifndef __PLUGINS_CARLA_NATIVE_PLUGIN_H__
#define __PLUGINS_CARLA_NATIVE_PLUGIN_H__

#include "CarlaNativePlugin.h"

typedef struct PluginDescriptor PluginDescriptor;

/**
 * @addtogroup plugins
 *
 * @{
 */

/**
 * The type of the Carla plugin.
 */
typedef enum CarlaPluginType
{
  CARLA_PLUGIN_RACK,
  CARLA_PLUGIN_PATCHBAY,
  CARLA_PLUGIN_PATCHBAY16,
  CARLA_PLUGIN_PATCHBAY32,
  CARLA_PLUGIN_PATCHBAY64,
} CarlaPluginType;

typedef struct CarlaNativePlugin
{
  NativePluginHandle       handle;
  NativeHostDescriptor     host;
  const NativePluginDescriptor * descriptor;

  uint32_t                 midi_event_count;
  NativeMidiEvent          midi_events[200];
  NativeTimeInfo           time_info;

  /** Pointer back to Plugin. */
  Plugin *                 plugin;
} CarlaNativePlugin;

/**
 * Creates an instance of a CarlaNativePlugin inside
 * the given Plugin.
 *
 * The given Plugin must have its descriptor filled in.
 */
void
carla_native_plugin_create (
  Plugin * plugin);

/**
 * Returns a filled in descriptor for the given
 * type.
 *
 * @param ins Set the descriptor to be an instrument.
 */
PluginDescriptor *
carla_native_plugin_get_descriptor (
  CarlaPluginType type,
  int             ins);

/**
 * Wrapper to get param count.
 */
static inline uint32_t
carla_native_plugin_get_param_count (
  CarlaNativePlugin * self)
{
  return
    self->descriptor->get_parameter_count (
      self->handle);
}

/**
 * Wrapper to get param info at given index.
 */
static inline const NativeParameter *
carla_native_plugin_get_param_info (
  CarlaNativePlugin * self,
  uint32_t            index)
{
  return
    self->descriptor->get_parameter_info (
      self->handle, index);
}

/**
 * Wrapper to get param value at given index.
 */
static inline float
carla_native_plugin_get_param_value (
  CarlaNativePlugin * self,
  uint32_t            index)
{
  return
    self->descriptor->get_parameter_value (
      self->handle, index);
}

/**
 * Instantiates the plugin.
 *
 * @ret 0 if no errors, non-zero if errors.
 */
int
carla_native_plugin_instantiate (
  CarlaNativePlugin * self);

/**
 * Shows or hides the UI.
 */
void
carla_native_plugin_show_ui (
  CarlaNativePlugin * self,
  int                 show);

/**
 * Returns the plugin Port corresponding to the
 * given parameter.
 */
Port *
carla_native_plugin_get_port_from_param (
  CarlaNativePlugin *     self,
  const NativeParameter * param);

/**
 * Deactivates, cleanups and frees the instance.
 */
void
carla_native_plugin_free (
  CarlaNativePlugin * self);

/**
 * @}
 */

#endif // header guard

#endif // HAVE_CARLA_NATIVE_PLUGIN
