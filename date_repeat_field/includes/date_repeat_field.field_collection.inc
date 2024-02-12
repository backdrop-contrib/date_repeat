<?php

/**
 * @file
 * Optional Field Collection functionality
 */

 /**
  * Define the repeat menu tabs for field collection.
  *
  * @param array $bundles
  * @return array
  *   List of menu items.
  */
function _date_repeat_field_field_collection_menu_tabs($bundles) {
  $items = array();

  foreach ($bundles as $bundle => $fields) {
    $field = field_info_field($bundle);
    if ($field['type'] == 'field_collection') {
      $path = field_collection_field_get_path($field);
      $count = count(explode('/', $path));
      $items[$path . '/%field_collection_item/repeats'] = array(
        'title' => 'Repeats',
        'page callback' => 'date_repeat_field_page',
        'page arguments' => array('field_collection_item', $count),
        'access callback' => 'date_repeat_field_show',
        'access arguments' => array('field_collection_item', $count),
        'type' => MENU_LOCAL_TASK,
        'context' => MENU_CONTEXT_PAGE | MENU_CONTEXT_INLINE,
      );
    }
  }
  return $items;
}
