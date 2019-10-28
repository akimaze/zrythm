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

/**
 * \file
 *
 * AutomationCurve API.
 */

#ifndef __AUDIO_AUTOMATION_CURVE_H__
#define __AUDIO_AUTOMATION_CURVE_H__

#include "audio/position.h"
#include "gui/backend/arranger_object.h"
#include "utils/types.h"

typedef struct AutomationTrack AutomationTrack;
typedef struct _AutomationCurveWidget
  AutomationCurveWidget;
typedef enum AutomatableType AutomatableType;

/**
 * @addtogroup audio
 *
 * @{
 */

#define AP_MAX_CURVINESS 6.0
/*#define AP_MIN_CURVINESS \
  //(1.f / AP_MAX_CURVINESS)*/
#define AP_MIN_CURVINESS 0.01
#define AP_CURVINESS_RANGE \
  (AP_MAX_CURVINESS - AP_MIN_CURVINESS)
#define AP_MID_CURVINESS 1.0

/**
 * Type of AutomationCurve.
 */
typedef enum AutomationCurveType
{
  AUTOMATION_CURVE_TYPE_BOOL,
  AUTOMATION_CURVE_TYPE_STEP,
  AUTOMATION_CURVE_TYPE_FLOAT
} AutomationCurveType;

/**
 * The curve between two AutomationPoint's.
 */
typedef struct AutomationCurve
{
  /** Base struct. */
  ArrangerObject      base;

  /** Curviness. */
  curviness_t         curviness;
  AutomationCurveType type;

  /** Pointer back to parent. */
  Region *            region;

  /** Index in the automation track, for faster
   * performance when getting ap before/after
   * curve. */
  int                 index;
} AutomationCurve;

static const cyaml_strval_t
automation_curve_type_strings[] =
{
	{ "Boolean",  AUTOMATION_CURVE_TYPE_BOOL },
	{ "Step",     AUTOMATION_CURVE_TYPE_STEP },
	{ "Float",    AUTOMATION_CURVE_TYPE_FLOAT },
};

static const cyaml_schema_field_t
  automation_curve_fields_schema[] =
{
  CYAML_FIELD_MAPPING (
    "base", CYAML_FLAG_DEFAULT,
    AutomationCurve, base,
    arranger_object_fields_schema),
	CYAML_FIELD_FLOAT (
    "curviness", CYAML_FLAG_DEFAULT,
    AutomationCurve, curviness),
  CYAML_FIELD_ENUM (
    "type", CYAML_FLAG_DEFAULT,
    AutomationCurve, type,
    automation_curve_type_strings,
    CYAML_ARRAY_LEN (automation_curve_type_strings)),
  CYAML_FIELD_INT (
    "index", CYAML_FLAG_DEFAULT,
    AutomationCurve, index),

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
automation_curve_schema = {
	CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
    AutomationCurve, automation_curve_fields_schema),
};

/**
 * Creates an AutomationCurve.
 *
 * @param a_type The AutomationType, used to
 *   figure out the AutomationCurve type.
 */
AutomationCurve *
automation_curve_new (
  const AutomatableType a_type,
  const Position *      pos,
  const int             is_main);

/**
 * The function to return a point on the curve.
 *
 * See https://stackoverflow.com/questions/17623152/how-map-tween-a-number-based-on-a-dynamic-curve
 *
 * @param ac The start point (0, 0).
 * @param x Normalized x.
 */
double
automation_curve_get_normalized_value (
  AutomationCurve * ac,
  double            x);

/**
 * Sets the curviness of the AutomationCurve.
 */
void
automation_curve_set_curviness (
  AutomationCurve * ac,
  const curviness_t curviness);

/**
 * @}
 */

#endif // __AUDIO_AUTOMATION_CURVE_H__
