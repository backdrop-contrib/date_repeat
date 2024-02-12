<?php


/**
 * Parses a ical date element.
 *
 * Possible formats to parse include:
 *   PROPERTY:YYYYMMDD[T][HH][MM][SS][Z]
 *   PROPERTY;VALUE=DATE:YYYYMMDD[T][HH][MM][SS][Z]
 *   PROPERTY;VALUE=DATE-TIME:YYYYMMDD[T][HH][MM][SS][Z]
 *   PROPERTY;TZID=XXXXXXXX;VALUE=DATE:YYYYMMDD[T][HH][MM][SS]
 *   PROPERTY;TZID=XXXXXXXX:YYYYMMDD[T][HH][MM][SS]
 *
 *   The property and the colon before the date are removed in the import
 *   process above and we are left with $field and $data.
 *
 * @param string $field
 *   The text before the colon and the date, i.e.
 *   ';VALUE=DATE:', ';VALUE=DATE-TIME:', ';TZID='
 * @param string $data
 *   The date itself, after the colon, in the format YYYYMMDD[T][HH][MM][SS][Z]
 *   'Z', if supplied, means the date is in UTC.
 *
 * @return array
 *   $items array, consisting of:
 *      'datetime'   => date in YYYY-MM-DD HH:MM format, not timezone adjusted
 *      'all_day'    => whether this is an all-day event with no time
 *      'tz'         => the timezone of the date, could be blank if the ical
 *                      has no timezone; the ical specs say no timezone
 *                      conversion should be done if no timezone info is
 *                      supplied
 * @todo
 *   Another option for dates is the format PROPERTY;VALUE=PERIOD:XXXX. The
 *   period may include a duration, or a date and a duration, or two dates, so
 *   would have to be split into parts and run through date_repeat_ical_parse_date()
 *   and date_repeat_ical_parse_duration(). This is not commonly used, so ignored for
 *   now. It will take more work to figure how to support that.
 */
function date_repeat_ical_parse_date($field, $data) {

  $items = array('datetime' => '', 'all_day' => '', 'tz' => '');
  if (empty($data)) {
    return $items;
  }
  // Make this a little more whitespace independent.
  $data = trim($data);

  // Turn the properties into a nice indexed array of
  // array(PROPERTYNAME => PROPERTYVALUE);
  $field_parts = preg_split('/[;:]/', $field);
  $properties = array();
  foreach ($field_parts as $part) {
    if (strpos($part, '=') !== FALSE) {
      $tmp = explode('=', $part);
      $properties[$tmp[0]] = $tmp[1];
    }
  }

  // Make this a little more whitespace independent.
  $data = trim($data);

  // Record if a time has been found.
  $has_time = FALSE;

  // If a format is specified, parse it according to that format.
  if (isset($properties['VALUE'])) {
    switch ($properties['VALUE']) {
      case 'DATE':
        preg_match(DATE_REGEX_ICAL_DATE, $data, $regs);
        // Date.
        $datetime = date_pad($regs[1]) . '-' . date_pad($regs[2]) . '-' . date_pad($regs[3]);
        break;

      case 'DATE-TIME':
        preg_match(DATE_REGEX_ICAL_DATETIME, $data, $regs);
        // Date.
        $datetime = date_pad($regs[1]) . '-' . date_pad($regs[2]) . '-' . date_pad($regs[3]);
        // Time.
        $datetime .= ' ' . date_pad($regs[4]) . ':' . date_pad($regs[5]) . ':' . date_pad($regs[6]);
        $has_time = TRUE;
        break;
    }
  }
  // If no format is specified, attempt a loose match.
  else {
    preg_match(DATE_REGEX_LOOSE, $data, $regs);
    if (!empty($regs) && count($regs) > 2) {
      // Date.
      $datetime = date_pad($regs[1]) . '-' . date_pad($regs[2]) . '-' . date_pad($regs[3]);
      if (isset($regs[4])) {
        $has_time = TRUE;
        // Time.
        $datetime .= ' ' . (!empty($regs[5]) ? date_pad($regs[5]) : '00') .
         ':' . (!empty($regs[6]) ? date_pad($regs[6]) : '00') .
         ':' . (!empty($regs[7]) ? date_pad($regs[7]) : '00');
      }
    }
  }

  // Use timezone if explicitly declared.
  if (isset($properties['TZID'])) {
    $tz = $properties['TZID'];
    // Fix alternatives like US-Eastern which should be US/Eastern.
    $tz = str_replace('-', '/', $tz);
    // Unset invalid timezone names.
    module_load_include('inc', 'date_repeat', 'includes/date_api.admin');
    $tz = _date_repeat_timezone_replacement($tz);
    if (!date_timezone_is_valid($tz)) {
      $tz = '';
    }
  }
  // If declared as UTC with terminating 'Z', use that timezone.
  elseif (strpos($data, 'Z') !== FALSE) {
    $tz = 'UTC';
  }
  // Otherwise this date is floating.
  else {
    $tz = '';
  }

  $items['datetime'] = $datetime;
  $items['all_day'] = $has_time ? FALSE : TRUE;
  $items['tz'] = $tz;
  return $items;
}

/**
 * Parse an ical repeat rule.
 *
 * @param string $field
 *   Deprecated.
 * @param string $data
 *   RRULE string.
 * @return array
 *   Array in the form of PROPERTY => array(VALUES)
 *   PROPERTIES include FREQ, INTERVAL, COUNT, BYDAY, BYMONTH, BYYEAR, UNTIL
 */
function date_repeat_ical_parse_rrule($field, $data) {
  $data = preg_replace("/RRULE.*:/", '', $data);
  $items = array('DATA' => $data);
  $rrule = explode(';', $data);
  foreach ($rrule as $key => $value) {
    $param = explode('=', $value);
    // Must be some kind of invalid data.
    if (count($param) != 2) {
      continue;
    }
    if ($param[0] == 'UNTIL') {
      $values = date_repeat_ical_parse_date('', $param[1]);
    }
    else {
      $values = explode(',', $param[1]);
    }
    // Treat items differently if they have multiple or single values.
    if (in_array($param[0], array('FREQ', 'INTERVAL', 'COUNT', 'WKST'))) {
      $items[$param[0]] = $param[1];
    }
    else {
      $items[$param[0]] = $values;
    }
  }
  return $items;
}

/**
 * Parse exception dates (can be multiple values).
 *
 * @param string $field
 * @param string $data
 *
 * @return array
 *   an array of date value arrays.
 */
function date_repeat_ical_parse_exceptions($field, $data) {
  $data = str_replace($field . ':', '', $data);
  $items = array('DATA' => $data);
  $ex_dates = explode(',', $data);
  foreach ($ex_dates as $ex_date) {
    $items[] = date_repeat_ical_parse_date('', $ex_date);
  }
  return $items;
}



/**
 * Return a date object for the ical date, adjusted to its local timezone.
 *
 * @param array $ical_date
 *   An array of ical date information created in the ical import.
 * @param string $to_tz
 *   The timezone to convert the date's value to.
 *
 * @return BackdropDateTime
 *   A timezone-adjusted date object.
 */
function date_repeat_ical_date($ical_date, $to_tz = FALSE) {

  // If the ical date has no timezone, must assume it is stateless
  // so treat it as a local date.
  if (empty($ical_date['datetime'])) {
    return NULL;
  }
  elseif (empty($ical_date['tz'])) {
    $from_tz = date_default_timezone();
  }
  else {
    $from_tz = $ical_date['tz'];
  }
  if (strlen($ical_date['datetime']) < 11) {
    $ical_date['datetime'] .= ' 00:00:00';
  }
  $date = new BackdropDateTime($ical_date['datetime'], new DateTimeZone($from_tz));

  if ($to_tz && $ical_date['tz'] != '' && $to_tz != $ical_date['tz']) {
    date_timezone_set($date, timezone_open($to_tz));
  }
  return $date;
}

/**
 * Escape #text elements for safe iCal use.
 *
 * @param string $text
 *   Text to escape
 *
 * @return string
 *   Escaped text
 */
function date_repeat_ical_escape_text($text) {
  $text = backdrop_html_to_text($text);
  $text = trim($text);
  // TODO Per #38130 the iCal specs don't want : and " escaped
  // but there was some reason for adding this in. Need to watch
  // this and see if anything breaks.
  // $text = str_replace('"', '\"', $text);
  // $text = str_replace(":", "\:", $text);
  $text = preg_replace("/\\\b/", "\\\\", $text);
  $text = str_replace(",", "\,", $text);
  $text = str_replace(";", "\;", $text);
  $text = str_replace("\n", "\\n ", $text);
  return trim($text);
}

/**
 * Build an iCal RULE from $form_values.
 *
 * @param array $form_values
 *   An array constructed like the one created by date_repeat_ical_parse_rrule().
 *     [RRULE] => Array (
 *       [FREQ] => Array (
 *         [0] => MONTHLY
 *       )
 *       [BYDAY] => Array (
 *         [0] => 1SU
 *         [1] => -1SU
 *       )
 *       [UNTIL] => Array (
 *         [datetime] => 1997-21-31 09:00:00
 *         [all_day] => 0
 *         [tz] => US/Eastern
 *       )
 *     )
 *     [EXDATE] => Array (
 *       [0] = Array (
 *         [datetime] => 1997-09-21 09:00:00
 *         [all_day] => 0
 *         [tz] => US/Eastern
 *       )
 *       [1] = Array (
 *         [datetime] => 1997-10-05 09:00:00
 *         [all_day] => 0
 *         [tz] => US/Eastern
 *       )
 *     )
 *     [RDATE] => Array (
 *       [0] = Array (
 *         [datetime] => 1997-09-21 09:00:00
 *         [all_day] => 0
 *         [tz] => US/Eastern
 *       )
 *       [1] = Array (
 *         [datetime] => 1997-10-05 09:00:00
 *         [all_day] => 0
 *         [tz] => US/Eastern
 *       )
 *     )
 *
 * @return string
 *   A RRULE formatted string.
 */
function date_repeat_ical_build_rrule($form_values) {
  $rrule = '';
  if (empty($form_values) || !is_array($form_values)) {
    return $rrule;
  }

  // Grab the RRULE data and put them into iCal RRULE format.
  $rrule .= 'RRULE:FREQ=' . (!array_key_exists('FREQ', $form_values) ? 'DAILY' : $form_values['FREQ']);
  $rrule .= ';INTERVAL=' . (!array_key_exists('INTERVAL', $form_values) ? 1 : $form_values['INTERVAL']);

  // Unset the empty 'All' values.
  if (array_key_exists('BYDAY', $form_values) && is_array($form_values['BYDAY'])) {
    unset($form_values['BYDAY']['']);
  }
  if (array_key_exists('BYMONTH', $form_values) && is_array($form_values['BYMONTH'])) {
    unset($form_values['BYMONTH']['']);
  }
  if (array_key_exists('BYMONTHDAY', $form_values) && is_array($form_values['BYMONTHDAY'])) {
    unset($form_values['BYMONTHDAY']['']);
  }

  if (array_key_exists('BYDAY', $form_values) && is_array($form_values['BYDAY']) && $byday = implode(",", $form_values['BYDAY'])) {
    $rrule .= ';BYDAY=' . $byday;
  }
  if (array_key_exists('BYMONTH', $form_values) && is_array($form_values['BYMONTH']) && $bymonth = implode(",", $form_values['BYMONTH'])) {
    $rrule .= ';BYMONTH=' . $bymonth;
  }
  if (array_key_exists('BYMONTHDAY', $form_values) && is_array($form_values['BYMONTHDAY']) && $bymonthday = implode(",", $form_values['BYMONTHDAY'])) {
    $rrule .= ';BYMONTHDAY=' . $bymonthday;
  }
  // The UNTIL date is supposed to always be expressed in UTC.
  // The input date values may already have been converted to a date object on a
  // previous pass, so check for that.
  if (array_key_exists('UNTIL', $form_values) && array_key_exists('datetime', $form_values['UNTIL']) && !empty($form_values['UNTIL']['datetime'])) {
    // We only collect a date for UNTIL, but we need it to be inclusive, so
    // force it to a full datetime element at the last second of the day.
    if (!is_object($form_values['UNTIL']['datetime'])) {
      // If this is a date without time, give it time.
      if (strlen($form_values['UNTIL']['datetime']) < 11) {
        $granularity_options = backdrop_map_assoc(array(
          'year',
          'month',
          'day',
          'hour',
          'minute',
          'second',
        ));

        $form_values['UNTIL']['datetime'] .= ' 23:59:59';
        $form_values['UNTIL']['granularity'] = serialize($granularity_options);
        $form_values['UNTIL']['all_day'] = FALSE;
      }
      $until = date_repeat_ical_date($form_values['UNTIL'], 'UTC');
    }
    else {
      $until = $form_values['UNTIL']['datetime'];
    }
    $rrule .= ';UNTIL=' . date_format($until, DATE_FORMAT_ICAL) . 'Z';
  }
  // Our form doesn't allow a value for COUNT, but it may be needed by
  // modules using the API, so add it to the rule.
  if (array_key_exists('COUNT', $form_values)) {
    $rrule .= ';COUNT=' . $form_values['COUNT'];
  }

  // iCal rules presume the week starts on Monday unless otherwise specified,
  // so we'll specify it.
  if (array_key_exists('WKST', $form_values)) {
    $rrule .= ';WKST=' . $form_values['WKST'];
  }
  else {
    $rrule .= ';WKST=' . date_repeat_dow2day(system_date_format_load('first_day'));
  }

  // Exceptions dates go last, on their own line.
  // The input date values may already have been converted to a date
  // object on a previous pass, so check for that.
  if (isset($form_values['EXDATE']) && is_array($form_values['EXDATE'])) {
    $ex_dates = array();
    foreach ($form_values['EXDATE'] as $value) {
      if (!empty($value['datetime'])) {
        $date = !is_object($value['datetime']) ? date_repeat_ical_date($value, 'UTC') : $value['datetime'];
        $ex_date = !empty($date) ? date_format($date, DATE_FORMAT_ICAL) . 'Z' : '';
        if (!empty($ex_date)) {
          $ex_dates[] = $ex_date;
        }
      }
    }
    if (!empty($ex_dates)) {
      sort($ex_dates);
      $rrule .= chr(13) . chr(10) . 'EXDATE:' . implode(',', $ex_dates);
    }
  }
  elseif (!empty($form_values['EXDATE'])) {
    $rrule .= chr(13) . chr(10) . 'EXDATE:' . $form_values['EXDATE'];
  }

  // Exceptions dates go last, on their own line.
  if (isset($form_values['RDATE']) && is_array($form_values['RDATE'])) {
    $ex_dates = array();
    foreach ($form_values['RDATE'] as $value) {
      $date = !is_object($value['datetime']) ? date_repeat_ical_date($value, 'UTC') : $value['datetime'];
      $ex_date = !empty($date) ? date_format($date, DATE_FORMAT_ICAL) . 'Z' : '';
      if (!empty($ex_date)) {
        $ex_dates[] = $ex_date;
      }
    }
    if (!empty($ex_dates)) {
      sort($ex_dates);
      $rrule .= chr(13) . chr(10) . 'RDATE:' . implode(',', $ex_dates);
    }
  }
  elseif (!empty($form_values['RDATE'])) {
    $rrule .= chr(13) . chr(10) . 'RDATE:' . $form_values['RDATE'];
  }

  return $rrule;
}
