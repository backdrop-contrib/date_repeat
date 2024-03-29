<?php

/**
 * @file
 * Theme functions.
 */

/**
 * @addtogroup themeable
 * @{
 *
 * Formatter themes
 */

/**
 * Returns HTML for a date element formatted as a Start/End combination.
 *
 * $entity->date_id
 *   If set, this will show only an individual date on a field with
 *   multiple dates. The value should be a string that contains
 *   the following values, separated with periods:
 *   - module name of the module adding the item
 *   - node nid
 *   - field name
 *   - delta value of the field to be displayed
 *   - other information the module's custom theme might need.
 *
 *   Used by the calendar module and available for other uses.
 *   example: 'date.217.field_date.3.test'
 *
 * $entity->date_repeat_show
 *   If TRUE, tells the theme to show all the computed values of a repeating
 *   date. If not TRUE or not set, only the start date and the repeat rule
 *   will be displayed.
 *
 * $dates['format']
 *   The format string used on these dates
 * $dates['value']['local']['object']
 *   The local date object for the Start date
 * $dates['value2']['local']['object']
 *   The local date object for the End date
 * $dates['value']['local']['datetime']
 *   The datetime value of the Start date database (GMT) value
 * $dates['value2']['local']['datetime']
 *   The datetime value of the End date database (GMT) value
 * $dates['value']['formatted']
 *   Formatted Start date, i.e. 'February 15, 2007 2:00 pm';
 * $dates['value']['formatted_date']
 *   Only the date part of the formatted Start date
 * $dates['value']['formatted_time']
 *   Only the time part of the formatted Start date
 * $dates['value2']['formatted']
 *   Formatted End date, i.e. 'February 15, 2007 6:00 pm';
 * $dates['value2']['formatted_date']
 *   Only the date part of the formatted End date
 * $dates['value2']['formatted_time']
 *   Only the time part of the formatted End date
 */
function theme_date_repeat_display_combination($variables) {
  static $repeating_ids = array();

  $entity_type = $variables['entity_type'];
  $entity      = $variables['entity'];
  $field       = $variables['field'];
  $instance    = $variables['instance'];
  $langcode    = $variables['langcode'];
  $item        = $variables['item'];
  $delta       = $variables['delta'];
  $display     = $variables['display'];
  $field_name  = $field['field_name'];
  $formatter   = $display['type'];
  $options     = $display['settings'];
  $dates       = $variables['dates'];
  $attributes  = $variables['attributes'];
  $precision   = date_granularity_precision($field['settings']['granularity']);
  $show_remaining_days = $variables['show_remaining_days'];

  $output = '';

  // If date_id is set for this field and delta doesn't match, don't display it.
  if (!empty($entity->date_id)) {
    foreach ((array) $entity->date_id as $key => $id) {
      list($module, $nid, $field_name, $item_delta, $other) = explode('.', $id . '.');
      if ($field_name == $field['field_name'] && isset($delta) && $item_delta != $delta) {
        return $output;
      }
    }
  }

  // Check the formatter settings to see if the repeat rule should be displayed.
  // Show it only with the first multiple value date.
  list($id) = entity_extract_ids($entity_type, $entity);
  if (!in_array($id, $repeating_ids) && module_exists('date_repeat_field') && !empty($item['rrule']) && $options['show_repeat_rule'] == 'show') {
    $repeat_vars = array(
      'field' => $field,
      'item' => $item,
      'entity_type' => $entity_type,
      'entity' => $entity,
    );
    $output .= theme('date_repeat_display', $repeat_vars);
    $repeating_ids[] = $id;
  }

  // If this is a full node or a pseudo node created by grouping multiple
  // values, see exactly which values are supposed to be visible.
  if (isset($entity->$field_name)) {
    $entity = date_prepare_entity($formatter, $entity_type, $entity, $field, $instance, $langcode, $item, $display);
    // Did the current value get removed by formatter settings?
    if (empty($entity->{$field_name}[$langcode][$delta])) {
      return $output;
    }
    // Adjust the $element values to match the changes.
    $element['#entity'] = $entity;
  }

  switch ($options['fromto']) {
    case 'value':
      $date1 = $dates['value']['formatted'];
      $date2 = $date1;
      break;

    case 'value2':
      $date2 = $dates['value2']['formatted'];
      $date1 = $date2;
      break;

    default:
      $date1 = $dates['value']['formatted'];
      $date2 = $dates['value2']['formatted'];
      break;
  }

  // Pull the timezone, if any, out of the formatted result and tack it back on
  // at the end, if it is in the current formatted date.
  $timezone = $dates['value']['formatted_timezone'];
  if (date_timezone_is_valid($timezone) && $date1 !== $date2) {
    // We only handle timezone identifiers.
    // Start and end can have different offsets (DST).
    // Removing an offset in seconds (Z) creates false positives.
    // And we only do this, if we really have a range,
    // single dates are left untouched.
    $date1 = str_replace($timezone, '', $date1);
    $date2 = str_replace($timezone, '', $date2);
    $timezone = ' ' . $timezone;
  }
  else {
    $timezone = '';
  }
  $time1 = preg_replace('`^([\(\[])`', '', $dates['value']['formatted_time']);
  $time1 = preg_replace('([\)\]]$)', '', $time1);
  $time2 = preg_replace('`^([\(\[])`', '', $dates['value2']['formatted_time']);
  $time2 = preg_replace('([\)\]]$)', '', $time2);

  // A date with a granularity of 'hour' has a time string that is an integer
  // value. We can't use that to replace time strings in formatted dates.
  $has_time_string = date_has_time($field['settings']['granularity']);
  if ($precision == 'hour') {
    $has_time_string = FALSE;
  }

  // Check remaining days.
  $show_remaining_days = '';
  if (!empty($variables['show_remaining_days'])) {
    // Create a date object from the timestamp.
    $date = new DateTime('@' . strtotime($variables['dates']['value']['formatted_iso']));
    // Using date_diff()->days allows to get the days diff, without having to
    // use complex math to calculate it. It also takes into account leap years.
    $remaining_days = date_diff($date, date_now())->days;
    // Show remaining days only for future events.
    if ($date->format('U') > REQUEST_TIME) {
      $show_remaining_days = theme('date_display_remaining', array(
        'remaining_days' => $remaining_days,
      ));
    }
  }

  // No date values, display nothing.
  if (empty($date1) && empty($date2)) {
    $output .= '';
  }
  // Start and End dates match or there is no End date, display a complete
  // single date.
  elseif ($date1 == $date2 || empty($date2)) {
    $output .= theme('date_display_single', array(
      'date' => $date1,
      'timezone' => $timezone,
      'attributes' => $attributes,
      'dates' => $dates,
      'show_remaining_days' => $show_remaining_days,
    ));
  }
  // Same day, different times, don't repeat the date but show both Start and
  // End times. We can NOT do this if the replacement value is an integer
  // instead of a time string.
  elseif ($has_time_string && $dates['value']['formatted_date'] == $dates['value2']['formatted_date']) {
    // Replace the original time with the start/end time in the formatted start
    // date. Make sure that parentheses or brackets wrapping the time will be
    // retained in the final result.
    $time = theme('date_display_range', array(
      'date1' => $time1,
      'date2' => $time2,
      'timezone' => $timezone,
      'attributes' => $attributes,
      'dates' => $dates,
      'show_remaining_days' => '',
    ));
    $replaced = str_replace($time1, $time, $date1);
    $output .= theme('date_display_single', array(
      'date' => $replaced,
      'timezone' => $timezone,
      'attributes' => array(),
      'dates' => $dates,
      'show_remaining_days' => $show_remaining_days,
    ));
  }
  // Different days, display both in their entirety.
  else {
    $output .= theme('date_display_range', array(
      'date1' => $date1,
      'date2' => $date2,
      'timezone' => $timezone,
      'attributes' => $attributes,
      'dates' => $dates,
      'show_remaining_days' => $show_remaining_days,
    ));
  }

  return $output;
}

/**
 * Render a date combo as a form element.
 */
function theme_date_repeat_form_element($variables) {
  $element = &$variables['element'];

  // Detect whether element is multiline.
  $count = preg_match_all('`<(?:div|span)\b[^>]* class="[^"]*\b(?:date-no-float|date-clear)\b`', $element['#children'], $matches, PREG_OFFSET_CAPTURE);
  $multiline = FALSE;
  if ($count > 1) {
    $multiline = TRUE;
  }
  elseif ($count) {
    $before = substr($element['#children'], 0, $matches[0][0][1]);
    if (preg_match('`<(?:div|span)\b[^>]* class="[^"]*\bdate-float\b`', $before)) {
      $multiline = TRUE;
    }
  }

  // Detect if there is more than one subfield.
  $count = count(explode('<label', $element['#children'])) - 1;
  if ($count == 1) {
    // If there is only one subfield, hide the label of the main field only if
    // the label of the subfield is visible.
    if (empty($variables['element']['value']['#instance'])
      || empty($variables['element']['value']['#instance']['widget'])
      || empty($variables['element']['value']['#instance']['widget']['settings'])
      || empty($variables['element']['value']['#instance']['widget']['settings']['label_position'])
      || $variables['element']['value']['#instance']['widget']['settings']['label_position'] != 'none') {
      $element['#title_display'] = 'none';
    }
  }

  // Wrap children with a div and add an extra class if element is multiline.
  $element['#children'] = '<div class="date-form-element-content' . ($multiline ? ' date-form-element-content-multiline' : '') . '">' . $element['#children'] . '</div>';

  return theme('form_element', $variables);
}

/**
 * @} End of addtogroup themeable */
