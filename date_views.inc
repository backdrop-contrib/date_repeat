<?php
// $Id$
/**
 * Views field filters
 */
function date_views_help() {
  return t('<h2>Date Views</h2>
        <h3>Date Range Argument</h3>
        <p>
        A flexible date range argument is provided in views. 
        The date range argument is based on the ISO 8601 week, duration, and time interval standards. 
        The argument expects a value like 2006-01-01T12:30:00--2006-01-15, or 2006-W24, or @P1W.
        Separate from and to dates with a separator, a double hyphen (--). 
        The from and to dates in argument are ISO dates, but parts can be omitted,  
        and will be assumed to be the first possible (for the from date)
        or the last possible (for the to date) value in that time period. 
        The \'to\' portion of the argument can be eliminated if it is the same as the \'from\' portion. 
        Use @ instead of a date to substitute in the current date and time. 
        </p>
        <p>Examples of views date range arguments:</p>
        <table>
        <tr><th>Argument</th><th>Resulting Query Range</th></tr>
        <tr><td>2006-W24</td><td>24th ISO week in 2006</td></tr>
        <tr><td>2006</td><td>the whole year of 2006</td></tr>
        <tr><td>2006-03</td><td>the whole month of Mar 2006</td></tr>
        <tr><td>2006-02--2007-03-15</td><td>Feb 1 2006 to Mar 15 2006</td></tr>
        <tr><td>2006-08-31T14:00:00P2H</td><td>the 14th to 16th hours of Aug 8 2006</td></tr>
        <tr><td>@--2006-12-31</td><td>NOW to 2006-12-31T23:59:59</td></tr>
        <tr><td>@P3H</td><td>NOW to three hours from now</td></tr>
        <tr><td>@P1Y90D</td><td>NOW to 1 year and 90 days from now</td></tr>
        <tr><td>2006-03-05P1W</td><td>the 1 week period starting Mar 5 2006</td></tr>
        <tr><td>2006-01P3M</td><td>the 3 month period starting Jan 2006</td></tr>
        </table>
        <h3>Date Browser</h3>
        <p>A Date Browser plugin is available to add back/next navigation links to a view that uses the 
        date range argument. Also adds "week of XXX", "month of XXX" headings to views and blocks  
        and defaults blocks and views w/out arguments to current period to start paging.
        </p>
        <p>To use the Date Browser:</p>
        <ul>
        <li>Create a view, and enable the Date: date range argument for the date field you want
        to page on. Set the argument option to year, month, day, week, or hour, depending 
        on the increment you want the browser to use as a period.</li>
        <li>Do not add any other arguments to the view. Filters are fine, though.</li>
        <li>In the page section of the view, select the option to display the view as Date: Date Browser. 
        (instead of teaser or table). Make sure display as a page is checked, and provide a url.</li>
        <li>Be sure to put something in "empty text" for the page. This is what will be displayed
        if you navigate to a period that has no results.</li>
        <li>Go to the view url. When no date is selected, the view will automatically display the
        current period (year, month, day, week, or hour), with back/next links to page through
        data by the selected period.</li>
        <li>You can also choose to use the Date Browser for block views. That will default the block
        view to display the current period, and provide a link to the full view.</li>
        <li>The page, navigation, label, and block displays are themed, and can be customized by overriding
        the provided themes.</li>');
}
 
function date_views_filters($field) {
  
  switch ($field['type']) {
    case ('date'):
      $handler = 'date_views_filter_handler';
      break;
    case ('datestamp'):
      $handler = 'date_views_timestamp_filter_handler';
      break;
  }
  return array(
   'default' => array(
     'operator' => array(
       '=' => t('is equal to'),
       '<>' => t('is not equal to'),
       '>' => t('greater than'),
       '>=' => t('greater than or equal to'),
       '<' => t('less than'),
       '<=' => t('less than or equal to'),
       ),
     'value' => views_handler_filter_date_value_form(),
     'option' => 'string',
     'handler' => $handler,
     'extra' => array('field' => $field),
     'help' => t('This filter allows events to be filtered by their  date. Enter dates in the format: CCYY-MM-DD HH:MM:SS. Enter \'now\' to use the current time. You may enter a delta (in seconds) to the option that will be added to the time; this is most useful when combined with now. If you have the jscalendar module from jstools installed, you can use a popup date picker here.'),
     ),
   );
}

/**
 *  Handler for date filter. 
 */

function date_views_timestamp_filter_handler($op, $filter, $filterinfo, &$query) {
  // this is just a wrapper function that sets the date type 
  return date_views_filter_handler($op, $filter, $filterinfo, $query, 'int');
}
function date_views_filter_handler($op, $filter, $filterinfo, &$query, $field_type = 'iso') {
  include_once(drupal_get_path('module', 'date') .'/date.inc');

  if (empty($filter['value'])) return;

  $field   = $filterinfo['extra']['field'];
  $formats = date_get_formats($field);
  $date    = $filter['value'] == 'now' ? "NOW()" : "'". str_replace('T', ' ', date_custom2iso($filter['value'], $formats['input']['text'])) ."'";

  $db_info = content_database_info($field);
  $value   = $db_info['columns']['value']['column'];
  $table   = 'node_data_'. $field['field_name']; // uses views alias table name
  $offset = date_views_offset($field);

  $query->ensure_table($table);
  $query->add_where(date_sql('DATE', $table .'.'. $value, $field_type, $offset) . $filter['operator'] . $date);

}

/**
 *  Return field or value for a timezone offset
 *  TODO - this needs work, date-specific timezone handling will not work right in views now
 *  views needs a field in the database that has the right offset that can be joined into the view
 *  probably will need to alter the table to add another column to store offset
 */
function date_views_offset($field) {
  switch ($field['tz_handling']) {
  case ('date'):
  case ('site'):
    $offset = variable_get('date_default_timezone', 0);
    break;
  default:
    $offset = '';
    break;
  }
  return $offset;
}


/**
 * Implementation of hook_views_arguments().
 */
function date_views_arguments() {
   
  // must create an argument with the name content: $field['field_name'] to override 
  // the one created automatically for all fields by content.module (which will NOT work correctly)
  // sending both to the same handler ultimately, using a wrapper function to identify
  // whether the date is an iso or timestamp date
  
  $field_types = _content_field_types();
  $arguments = array();

  foreach (content_fields() as $field) {
    $db_info = content_database_info($field);
    switch ($field['type']) {
    case 'date':
      $argument = array();
      $argument['name'] = $field_types[$field['type']]['label'] .' '. $field['widget']['label'] .' ('. $field['field_name'] .')';
      $argument['handler'] = 'date_views_argument_range_handler';
      $argument['help'] = t("Defines an argument to filter for dates within a range, in the format 'YYYY-MM-DD--YYYY-MM-DD'. Many other options can be used in arguments. See %link for other examples.", array('%link' => l(t('help'), 'admin/help/date')));
      $argument['option'] = 'date_range_arg_options';
      $arguments['content: '. $field['field_name']] = $argument;
      break;
    
    case 'datestamp':
      $argument = array();
      $argument['name'] = $field_types[$field['type']]['label'] .' '. $field['widget']['label'] .' ('. $field['field_name'] .')';
      $argument['handler'] = 'date_views_timestamp_argument_range_handler';
      $argument['help'] = t("Defines an argument to filter for dates within a range, in the format 'YYYY-MM-DD--YYYY-MM-DD'. Many other options can be used in arguments. See %link for other examples.", array('%link' => l(t('help'), 'admin/help/date')));
      $argument['option'] = 'date_range_arg_options';
      $arguments['content: '. $field['field_name']] = $argument;
      break;
    }
  }
  return $arguments;
}

/**
 * 
 * Flexible date range argument handler
 * 
 * Argument is based on ISO 8601 date duration and time interval standards
 * 
 * See http://en.wikipedia.org/wiki/ISO_8601#Week_dates for definitions of ISO weeks
 * See http://en.wikipedia.org/wiki/ISO_8601#Duration for definitions of ISO duration and time interval
 * 
 * Argument expects a value like 2006-01-01--2006-01-15, or 2006-W24, or @P1W
 * Separate from and to dates or date and period with a double hyphen (--)
 * 
 * From and to dates in argument are ISO dates, but can be shortened and missing parts will be added
 * Omitted parts of ISO dates will be assumed to be the first possible (for the from date)
 * or the last possible (for the to date) value in that time period
 * 
 * The 'to' portion of the argument can be eliminated if it is the same as the 'from' portion
 * Use @ instead of a date to substitute in the current date and time.
 * 
 * Use periods (P1H, P1D, P1W, P1M, P1Y) to get next hour/day/week/month/year from now
 * Use date before P sign to get next hour/day/week/month/year from that date
 * 
 * This module does not currently handle the option of using a period with an end date,
 * only a start date followed by a period.
 * 
 * The groupby selector values are used only if a summary view of the argument is requested
 * possible values are by year, by month, by week, by day, and by hour
 * 
 * if summaries are used, navigating to the view with no argument will display subtotals for the query, 
 * grouped by the selected range, with a link to the complete query for each range
 * 
 */
function date_views_timestamp_argument_range_handler($op, &$query, $argtype, $arg = '') {
  // this is just a wrapper function that sets the date type 
  return date_views_argument_range_handler($op, $query, $argtype, $arg, 'int');
}
function date_views_argument_range_handler($op, &$query, $argtype, $arg = '', $field_type = 'iso') {
  static $format;
  
  include_once(drupal_get_path('module', 'date') .'/date.inc');
  
  $name = explode(':', is_array($argtype) ? $argtype['type'] : $argtype);
  $field_name = trim($name[1]);
  $field      = content_fields($field_name);
  $db_info    = content_database_info($field);
  $value      = $db_info['columns']['value']['column'];
  $timezone   = $db_info['columns']['timezone']['column'];
  $table      = 'node_data_'. $field['field_name']; // uses views alias table name
  
  $offset = date_views_offset($field['tz_handling']);
  
  switch ($op) {
    case 'summary':
      
      // in the summary operation, the arg contains the selected option
      $groupby = $arg;
      
      switch ($groupby) {
        case ('year'):
          $format = 'Y';
          $fieldinfo['field'] = date_sql_concat(array(
              date_sql('YEAR', $table .'.'. $value, $field_type, $offset), 
              ));
          break;
        case ('month'):
          $format = 'F Y';
          $fieldinfo['field'] = date_sql_concat(array(
              date_sql('YEAR', $table .'.'. $value, $field_type), "'-'", 
              date_sql_pad(date_sql('MONTH', $table .'.'. $value, $field_type, $offset)),
              ));
          break;
        case ('day'):
          $format = 'F j Y';
          $fieldinfo['field'] = date_sql_concat(array(
              date_sql('YEAR', $table .'.'. $value, $field_type), "'-'", 
              date_sql_pad(date_sql('MONTH', $table .'.'. $value, $field_type, $offset)), "'-'",
              date_sql_pad(date_sql('DAY', $table .'.'. $value, $field_type, $offset)),
              ));
          break;
        case ('hour'):
          $format = 'F j Y - H';
          $fieldinfo['field'] = date_sql_concat(array(
              date_sql('YEAR', $table .'.'. $value, $field_type), "'-'", 
              date_sql_pad(date_sql('MONTH', $table .'.'. $value, $field_type, $offset)), "'-'",
              date_sql_pad(date_sql('DAY', $table .'.'. $value, $field_type, $offset)), "'T'",
              date_sql_pad(date_sql('HOUR', $table .'.'. $value, $field_type, $offset)),
              ));
          break;
        case ('week'):
          $format = 'F j Y (W)';
          $fieldinfo['field'] = date_sql_concat(array(
              date_sql('YEAR', $table .'.'. $value, $field_type, $offset), "'-W'", 
              date_sql('WEEK', $table .'.'. $value, $field_type, $offset),
              ));
          break;
      }
      $fieldinfo['fieldname'] = 'range';
      
      $query->ensure_table($table);
      $query->add_field($value, $table);
      $query->add_field($timezone, $table);
      return $fieldinfo;
      break;

    case 'link':
      // links are created from date range groupings
      $time = $query->$value > '' ? ($field_type == 'iso' ? date_iso2unix($query->$value) : $query->$value) : '';
      return l(date_date($format, $time), $arg .'/'. $query->range);

    case 'sort':
      break;

    case 'filter':
      $range = date_views_date_range($arg);
      $query->ensure_table($table);
      $query->add_field($value, $table);
      //$query->add_field($timezone, $table);
      $query->add_where(date_sql('DATE', $table .'.'. $value, $field_type, $offset) .">='". str_replace('T', ' ', $range[0]) ."'");
      $query->add_where(date_sql('DATE', $table .'.'. $value, $field_type, $offset) ."<='". str_replace('T', ' ', $range[1]) ."'");
      break;
  
    case 'title':
      $item = array(key($db_info['columns']) => $query);
      return content_format($field, $item, 'default');
  }
}

function date_views_date_range($arg) {
  if (stristr($arg, 'P')) {
    // for a date plus value, get the min and max values
    $range = date_plus_period($arg);
    $min_date = $range[0];
    $max_date = $range[1];
  }
  elseif (stristr($arg, '-W') && !stristr($arg, '--')) {
    // for a specified week, get the min and max values
    $range = date_week_value($arg);
    $min_date = $range[0];
    $max_date = $range[1];
  }
  else {
    // for all other get the date range from the supplied argument
    $range = (array) explode('--', $arg);
    $min_date = date_range_value($range[0], 'min');
    $max_date = date_range_value(($range[1] ? $range[1] : $range[0]), 'max');
  }
  return array($min_date, $max_date);
}
/**
 *  Validate and pad date range argument element
 * 
 *  @param $value - a full or partial ISO date from an argument
 *  @param $value_type - min or max, whether it is the from or the to part of the range
 *  @return complete, validated ISO date
 */
function date_range_value($value, $value_type = 'min') {
  
  include_once(drupal_get_path('module', 'date') .'/date.inc');

  $now   = date_date(DATE_STRING_ISO, time());
  if (trim($value) == '@' || trim($value) == '') return $now;

  switch (strlen($value)) {
  case (4):
    $return = ($value_type == 'min' ? $value .'-01-01T00:00:00' : $value .'-12-31T23:59:59');
    break;
  case (7):
    $return = ($value_type == 'min' ? $value .'-01T00:00:00' : $value .'-31T23:59:59');
    break;
  case (10):
    $return = ($value_type == 'min' ? $value .'T00:00:00' : $value .'T23:59:59');
    break;
  case (13):
    $return = ($value_type == 'min' ? $value .':00:00' : $value .':59:59');
    break;
  case (16):
    $return = ($value_type == 'min' ? $value .':00' : $value .':59');
    break;
  case (19):
    $return = $value;
    break;
  default:
    $return = $now;
  }
  // use date_preg to test for validity of constructed date
  return (date_preg($return) ? $return : $now);
}

/** 
 *  Compute min and max dates for a week
 * 
 *  based on ISO weeks, which start counting on the first Monday in a week that 
 *  has at least 4 days in the current year
 * 
 *  @value - an argument in the format 2006-W20 (year + -W + week number)
 *  @return an array of ISO dates representing the first and last day in the week
 */
function date_week_value($value) {
  
  include_once(drupal_get_path('module', 'date') .'/date.inc');

  $parts = explode('-W', $value);
  $year = $parts[0];
  // subtract 1 from week number so we don't double-count the final week
  $weeks = intval($parts[1] - 1);
  // get a unix value for the first day of the year
  $first_day_of_year = date_iso2unix($year .'-01-01T00:00:00');
  
  // get to the day of week of the first day of the year, 0 is Sunday
  $dow   = date_gmdate('w', $first_day_of_year);
  // ISO week counts actual first week only if it has at least 4 days in it
  if ($dow > 2) $weeks += 1;
  // calc adjustment from first day of year dow back or forward to Monday
  $shift = intval((1 - $dow) * 86400);
  // the day we want is $weeks away from first day of year, adjusted to the Monday of that week by $shift
  $first_day_of_week = $first_day_of_year + ($weeks * 604800) + $shift;
  $last_day_of_week  = $first_day_of_week + 604800 - 1;
  // convert the unix dates back to iso
  return array(date_unix2iso($first_day_of_week), date_unix2iso($last_day_of_week));

}

/**
 *  Compute min and max dates for a P value
 * 
 *  @value = an argument in the format (start date)P#(period type)
 *     where (period type) can be Y (year), M (month), D (day), W (week), H (hour)
 *     i.e. P1Y or P90D or P1Y3M2D4H
 *  @return an array of ISO dates representing the first and last day in the range
 */
function date_plus_period($value) {
  
  include_once(drupal_get_path('module', 'date') .'/date.inc');

  // min date is whatever is to the left of the period sign, defaults to current date
  $value = str_replace('--P', 'P', $value);
  $range = explode('P', $value);
  $min_date = date_range_value($range[0], 'min');

  // create a date object to use for the max_date
  $max_date = date_make_date($min_date, 'GMT', 'db', DATE_ISO);
  
  // iterate through the requested period, adding values as needed to the date object
  $remaining = $range[1];
  if ($years = strpos($remaining, 'Y')) {
    $sub       = explode('Y', $remaining);
    $remaining = $sub[1];
    $count     = intval($sub[0]);
    $max_iso   = intval(substr($max_date->db->iso, 0, 4) + $count) . substr($max_date->db->iso, 4, 15);
    date_set_date($max_date, $max_iso, 'GMT', 'db', DATE_ISO, TRUE);
  }
  if ($months = strpos($remaining, 'M')) {
    $sub       = explode('M', $remaining);
    $remaining = $sub[1];
    $count     = intval($sub[0]);
    $cur_mon   = intval(substr($max_date->db->iso, 6, 2));
    $cur_year  = intval(substr($max_date->db->iso, 0, 4));
    $max_iso   = (intval($cur_mon + $count) < 12 ? $cur_year : intval($cur_year + 1)) . '-'. sprintf('%02d', (intval($cur_mon + $count) < 12 ? intval($cur_mon + $count) : 1)) . substr($min_date, 7, 12);
    date_set_date($max_date, $max_iso, 'GMT', 'db', DATE_ISO, TRUE);
  }
  if (stristr($range[1], 'W')) {
    $sub       = explode('W', $remaining);
    $remaining = $sub[1];
    $count     = intval($sub[0]); 
    $max_unix  = intval($max_date->db->timestamp + (604800 * $count));
    date_set_date($max_date, $max_unix, 'GMT', 'db', DATE_UNIX, TRUE);
  }
  if ($days = strpos($remaining, 'D')) {
    $sub       = explode('D', $remaining);
    $remaining = $sub[1];
    $count     = intval($sub[0]); 
    $max_unix  = intval($max_date->db->timestamp + (86400 * $count));
    date_set_date($max_date, $max_unix, 'GMT', 'db', DATE_UNIX, TRUE);
  }
  if ($hours = strpos($remaining, 'H')) {
    $sub       = explode('H', $remaining);
    $remaining = $sub[1];
    $count     = intval($sub[0]); 
    $max_unix  = intval($max_date->db->timestamp + (3600 * $count));
    date_set_date($max_date, $max_unix, 'GMT', 'db', DATE_UNIX, TRUE);
  }
  // slice 1 second off max date to stop it just before end of period
  // needed because we are using <= as the operator
  $date->db->unix = intval($max_date->db->timestamp - 1);
  date_set_date($max_date, $date->db->unix, 'GMT', 'db', DATE_UNIX, TRUE);
  return array($min_date, $max_date->db->iso);

}

/**
 *  Define groupby options for date range summaries
 */
function date_range_arg_options() {
  return array(
    'year'  => t('summarize by year'), 
    'month' => t('summarize by month'), 
    'day'   => t('summarize by day'), 
    'week'  => t('summarize by week'), 
    'hour'  => t('summarize by hour')
    );
}

//============================== Date Browser ================================//

/**
 * Works only with views that use the date range argument
 * Adds this/next period navigation links to a date argument range view
 * Adds 'week of XXX', 'month of XXX' headings to views and blocks
 * Defaults blocks and views w/out arguments to current period to start paging
 * Choose period increments by selecting the option value of date range argument
 *  (year, month, week, day, hour)
 */

/**
 * Implementation of hook_views_style_plugins()
 */
function date_views_style_plugins() {

  $items = array();
  $items['date_views_browser'] = array(
    'name' => t('Date: Date Browser'),
    'theme' => 'date_views_browser_full_view',
    'summary_theme' => 'date_views_browser_summary_view',
    'needs_fields' => true,
    'needs_table_header' => true,
  );
  return $items;
}
/**
 * Implementation of hook_views_pre_view()
 */
function date_views_pre_view($view, $items) {
  $date_views_browser_views = date_views_browser_get_views();
  if (in_array($view->name, array_keys($date_views_browser_views))) {
    // make sure the navigation link gets added to empty pages
    if (sizeof($items) == 0 && $view->build_type == 'page') {
      $period = $date_views_browser_views[$view->name]->options;
      return date_views_browser_navigation($view, $period);  
    }
  }
}

/**
 *  Implementation of hook_views_query()
 *  Used to make sure view defaults to current date if no date selected
 */
function date_views_query_alter(&$query, &$view) {
  $date_views_browser_views = date_views_browser_get_views();
  if (in_array($view->name, array_keys($date_views_browser_views))) {
    if (sizeof($query->groupby) > 0) {
      // if no argument specified, add the current date range to the query
      // presence of groupby value indicates that argument is missing and query is performing a summary
      $path = explode('/', $view->url);
      $pos = sizeof($path);
      if ($view->build_type == 'block' || arg($pos) == '') {
        $arg = NULL;
      }
      else {
        $arg = arg($pos);
      }
      $arg        = date_views_browser_period_arg($arg, $view->argument[0]['options']);
      $name       = explode(':', $view->argument[0]['type']);
      $field_name = trim($name[1]);
      $field      = content_fields($field_name);
      $db_info    = content_database_info($field);
      $value      = $db_info['columns']['value']['column'];
      $timezone   = $db_info['columns']['timezone']['column'];
      $table      = 'node_data_'. $field['field_name']; // uses views alias table name
      $offset     = date_views_offset($field['tz_handling']);
      $range      = date_views_date_range($arg);
      $query->ensure_table($table);
      $query->add_field('nid', 'node');
      $query->add_field($value, $table);
      $query->add_where(date_sql('DATE', $table .'.'. $value, $field_type, $offset) .">='". str_replace('T', ' ', $range[0]) ."'");
      $query->add_where(date_sql('DATE', $table .'.'. $value, $field_type, $offset) ."<='". str_replace('T', ' ', $range[1]) ."'");
    }
    
  }
}
/**
 * Find all the views that qualify for date browser treatment
 */
function date_views_browser_get_views($refresh = FALSE) {
  static $date_views_browser_views;
  
  if (!isset($date_views_browser_views) || $refresh) {
    $date_views_browser_views = array();
    $arguments = date_views_arguments();
    $argument_list = "'". implode("','", array_keys($arguments)) ."'";
    $result = db_query("SELECT arg.*, view.name FROM {view_argument} arg JOIN {view_view} view ON arg.vid=view.vid WHERE arg.type IN ($argument_list) AND view.page_type='date_views_browser'");
    while ($view = db_fetch_object($result)) {
      $date_views_browser_views[$view->name] = $view;
    }
  }
  return $date_views_browser_views;
}
/**
 *  Make sure available views are refreshed when views are changed
 */
function date_form_alter($form_id, $form) {
  if ($form_id == 'views_edit_view') {
    date_views_browser_get_views(TRUE);
  }
}
/**
 *  Return the correct period for the date range argument
 */
function date_views_browser_period($period = 'month') {
  switch ($period) {
  case ('year'):
    return 'P1Y';
  case ('week'):
    return 'P1W';
  case ('day'):
    return 'P1D';
  case ('hour'):
    return 'P1H';
  default:
    return 'P1M';
  }
}
/**
 *  Format an argument for the date range
 */
function date_views_browser_period_arg($arg = NULL, $period = 'month') {
  include_once(drupal_get_path('module', 'date') .'/date.inc');
  switch ($period) {
  case ('week'):
    return date_gmdate('Y-m-d', date_views_browser_period_start_stamp($arg, $period)) . date_views_browser_period($period);
  case ('year'):
    return date_gmdate('Y', date_views_browser_period_start_stamp($arg, $period)) . date_views_browser_period($period);
  case ('day'):
    return date_gmdate('Y-m-d', date_views_browser_period_start_stamp($arg, $period)) . date_views_browser_period($period);
  case ('hour'):
    return date_gmdate('Y-m-d\TH', date_views_browser_period_start_stamp($arg, $period)) . date_views_browser_period($period);
  default:
    return date_gmdate('Y-m', date_views_browser_period_start_stamp($arg, $period)) . date_views_browser_period($period);
  }
}
/**
 *  Return label for current date range
 */
function date_views_browser_period_label($arg = NULL, $period = 'month') {
  include_once(drupal_get_path('module', 'date') .'/date.inc');

  // use the m/d/Y part of the preferred short date format
  if ($period != 'hour') {
    $format = array_shift(explode(' ', variable_get('date_format_short', 'm/d/Y - H:i')));
  }
  else {
    $format = str_replace(':i', '', variable_get('date_format_short', 'm/d/Y - H:i'));
  }
  return theme('date_views_browser_period_label', $period, date_gmdate($format, date_views_browser_period_start_stamp($arg, $period)));
}
/**
 *  Find the timestamp for the beginning of the period of the analyzed date arg
 */
function date_views_browser_period_start_stamp($arg = NULL, $period = 'month') {
  include_once(drupal_get_path('module', 'date') .'/date.inc');
  
  // get the range of dates
  $range = date_views_date_range($arg);
  $stamp = date_iso2unix($range[0]);

  // if a date range has already been determined, return the beginning of that range
  if ($arg) return $stamp;
  
  // if no range is yet set, find the beginning of the appropriate period
  switch ($period) {
  case ('week'):
    $dow = date_gmdate('w', $stamp);
    if ($dow >= 6) $adj = (86400 * 7);
    $start = intval($stamp - intval($dow * 86400) + $adj);
    break;
  case ('year'):
    $year = date_gmdate('Y', $stamp);
    $start = date_gmmktime(array('year' => $year, 'mon' => 1, 'mday' => 1));
    break;
  case ('day'):
    $year  = date_gmdate('Y', $stamp);
    $month = date_gmdate('n', $stamp);
    $day   = date_gmdate('j', $stamp);
    $start = date_gmmktime(array('year' => $year, 'mon' => $month, 'mday' => $day, 'hours' => 1));
    break;
  case ('hour'):
    $year  = date_gmdate('Y', $stamp);
    $month = date_gmdate('n', $stamp);
    $day   = date_gmdate('j', $stamp);
    $hour  = date_gmdate('H', $stamp);
    $start = date_gmmktime(array('year' => $year, 'mon' => $month, 'mday' => $day, 'hours' => $hour, 'minutes' => 1));
    break;
  default:
    $year  = date_gmdate('Y', $stamp);
    $month = date_gmdate('n', $stamp);
    $start = date_gmmktime(array('year' => $year, 'mon' => $month, 'mday' => 1));
    break;
  }
  return $start;
}
/**
 *  Navigation links for the full view
 */
function date_views_browser_navigation($view, $period) {
  include_once(drupal_get_path('module', 'date') .'/date.inc');

  $path = explode('/', $view->url);
  $pos = sizeof($path);
  if (arg($pos) == '') {
    $arg = NULL;
  } 
  else {
    $arg = arg($pos);
  }
  $range = date_views_date_range($arg);
  $stamp = date_views_browser_period_start_stamp($arg, $period);

  switch ($period) {
  case ('week'):
    $prev_date = date_gmdate('Y-m-d', intval($stamp - 604799));
    $next_date = date_gmdate('Y-m-d', intval($stamp + 604801));
    break;
  case ('year'):
    $prev_date = intval(substr($range[0], 0, 4) - 1);
    $next_date = intval(substr($range[1], 0, 4) + 1);
    break;
  case ('day'):
    $prev_date = date_gmdate('Y-m-d', intval($stamp - 86399));
    $next_date = date_gmdate('Y-m-d', intval($stamp + 86401));
    break;
  case ('hour'):
    $prev_date = date_gmdate('Y-m-d\TH', intval($stamp - 3599));
    $next_date = date_gmdate('Y-m-d\TH', intval($stamp + 3601));
    break;
  default:
    $year  = intval(substr($range[0], 0, 4));
    $month = intval(substr($range[1], 5, 2));
    $prev_date = ($month > 1 ? $year : intval($year - 1)) .'-'. ($month > 1 ? intval($month - 1) : 12);
    $next_date = ($month > 11 ? $year : intval($year + 1)) .'-'.  ($month < 12 ? intval($month + 1) : 1);
    break;
  }
  $prev = $view->url .'/'. $prev_date . date_views_browser_period($period);
  $next = $view->url .'/'. $next_date . date_views_browser_period($period);
  $label = date_views_browser_period_label($arg, $period);
  return theme('date_views_browser_navigation', $label, $period, $prev, $next);
}

//============================== THEMES ================================//

/**
 *  Theme for the current period label name
 */
function theme_date_views_browser_period_label($period, $date) {
  return t('%period of %date', array('%period' => ucwords($period), '%date' => $date));
}
/**
 *  Theme for page navigation
 */
function theme_date_views_browser_navigation($label, $period, $prev, $next) {
  theme_add_style(drupal_get_path('module', 'date') .'/date.css');
  $output  = '<div class="book-navigation date-browser-navigation">';
  $output .= '<div class="page-links">';
  $output .= l(t('‹ prev %period  ', array('%period' => $period)), $prev, array('class' => 'page-previous'));
  $output .= '<h3 class="date-browser-label"><span class="page-up">'. $label .'</span></h3>';
  $output .= l(t('  next %period  ›', array('%period' => $period)), $next, array('class' => 'page-next'));
  $output .= '</div></div>';
  return $output;
}
/**
 * Display a summary version of a view.
 */
function theme_date_views_browser_summary_view($view, $type, $level, $nodes, $args) {
  return theme('date_views_browser_full_view', $view, $nodes, $type);
}
/**
 *  View, themed so it can be overridden
 * 
 *  $display options are views_view_list, views_view_table, views_view_teasers, views_view_nodes
 */
function theme_date_views_browser_full_view($view, $nodes, $type) {
  
  $teasers = true;
  $links   = true;
  theme_add_style(drupal_get_path('module', 'date') .'/date.css');
  
  $date_views_browser_views = date_views_browser_get_views();
  $period = $date_views_browser_views[$view->name]->options;
  switch ($type) {
    case ('block'):
      $arg = date_views_browser_period_arg(NULL, $view->argument[0]['options']);
      if ($view->url) $url = $view->url . '/'. $arg;
      $output  .= '<h5 class="date-browser-block-label">'. l(date_views_browser_period_label(NULL, $period), $url) .'</h5>';
      $display = 'views_view_list';
      break;
    default:
      $output  .= date_views_browser_navigation($view, $period);
      $display = 'views_view_teasers';
      break;
  }
  $output .= theme($display, $view, $nodes, $type, $teasers, $links);

  return $output;
}