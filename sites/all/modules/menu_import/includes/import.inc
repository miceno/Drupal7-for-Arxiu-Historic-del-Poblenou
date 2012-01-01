<?php

/**
 * @file
 * Import functions for menu_import module.
 */

/**
 * Helper function to find node associated to path.
 *
 * @param $path
 *   Path (node/* or any other) as described in the input file.
 * @param $title
 *   Node's title as described in the input file.
 * @param $options
 *   Array import options provided.
 *
 * @return
 *   An array('nid' => '<node_id_or_false>', 'link_path' => '<path_if_exists_or_empty>').
 */
function _menu_import_lookup_path($path, $title, array $options) {
  $nid = $result['nid'] = $is_node = FALSE;

  // Search by alias by default.
  $system_url = drupal_lookup_path('source', $path, $options['node_language']);
  // No such alias in the system.
  if (!$system_url) {
    $system_url = $path;
  }

  // Detect Drupal's direct node URI
  if (drupal_substr($system_url, 0, 4) == 'node') {
    $is_node = TRUE;
    $nid = drupal_substr($system_url, 5);
    if (!is_numeric($nid)) {
      $nid = FALSE;
    }
  }

  // Search node by title if nothing found.
  if (!$nid) {
    $nid = db_select('node', 'n')
      ->fields('n', array('nid'))
      ->condition('n.title', $title)
      ->condition('n.language', $options['node_language'])
      ->execute()->fetchField();
  }

  // The menu item links to a node.
  if ($nid || $is_node) {
    $system_url = 'node/' . $nid;
    if (drupal_valid_path($system_url) && $options['link_to_content']) {
      // The node exists and we can safely link to it.
      $result['link_path'] = $system_url;
      $result['nid'] = $nid;
    }
    else {
      // The node doesn't exist, do not link to it.
      $result['link_path'] = '';
    }
  }
  // The menu item links to something different on the site.
  elseif (drupal_valid_path($system_url)) {
    $result['link_path'] = $system_url;
  }
  // The menu item has no link at all.
  else {
    $result['link_path'] = '';
  }

  return $result;
}

/**
 * Parse a line of text containing the menu structure.
 *
 * @param $line
 *   One line from input file.
 * @param $prev_level
 *   Previous level to build ierarchy.
 * @param $weights
 *   Array of menu items' weights.
 * @param $parents
 *   Array of menu items' parents.
 * @param $options
 *   Array of importing options.
 *
 * @return
 *   Array representing a menu item.
 */
function menu_import_parse_line($line, $prev_level, array $weights, array $parents, array $options) {
  $menuitem = array(
    'error' => FALSE,
    'link_title' => NULL,
    'children' => array(),
    'parent' => NULL,
    'nid' => FALSE,
    'path' => FALSE,
    'weight' => 0,
    'external' => FALSE,
    'level' => 0,
  );

  // Only * and - are allowed as indentation characters.
  // URL aliases are separated from title by vertical bar (|) or semi-colon (;).
  preg_match('/^([\-]+|[\*]+)?(\s+)?([^\|;]*)([\|;]{1}[^\|;]*)?([\|;]{1}[^\|;]*)?/', $line, $matches);

  $level = strlen($matches[1]); // No sense to use drupal_strlen on indentation.
  $title = trim(strip_tags($matches[3]));
  $path  = isset($matches[4]) ? trim(str_replace(array('|', ';'), '', $matches[4])) : '';
  $description = isset($matches[5]) ? trim(str_replace(array('|', ';'), '', $matches[5])) : '';

  // Skip empty items
  if (!strlen($title)) {
    $menuitem['error'] = t('missing menu title');
    return $menuitem;
  }

  // Make sure this item is only 1 level below the last item.
  if ($level > $prev_level + 1) {
    $menuitem['error'] = t('wrong indentation');
    return $menuitem;
  }

  if (isset($weights[$level])) {
    if ($level > $prev_level) {
      $weight = 0;
    }
    else {
      $weight = $weights[$level] + 1;
    }
  }
  else {
    $weight = 0;
  }
  $menuitem['weight'] = $weight;
  $menuitem['parent'] = !$level ? 0 : $parents[$level - 1];
  $menuitem['link_title'] = $title;
  $menuitem['level'] = $level;
  $menuitem['path'] = $path;

  if (url_is_external($path)) {
    $menuitem['external'] = TRUE;
    $menuitem['link_path'] = $path;
  }
  else {
    $result = _menu_import_lookup_path($path, $title, $options);
    $menuitem['link_path'] = $result['link_path'];
    $menuitem['nid'] = $result['nid'];
  }

  if ($description) {
    $menuitem['description'] = $description;
  }

  return $menuitem;
}

/**
 * File parser function. Reads through the text file and constructs the menu.
 *
 * @param $uri
 *   uri of the uploaded file.
 * @param $menu_name
 *   internal name of existiong menu.
 * @param $options
 *   An associative array of search options.
 *   - search_title: search node by title.
 *   - node_language: language to use.
 *
 * @return array
 *   array structure of menu.
 */
function menu_import_parse_menu_from_file($uri, $menu_name, array $options) {
  $menu = array(
    'errors' => array(),
    0 => array(
      'menu_name' => $menu_name,
      'children' => array(),
    )
  );

  // Keep track of actual weights per level.
  $weights = array();
  // Keep track of actual parents per level.
  $parents = array();

  $handle = fopen($uri, "r");
  if (!$handle) {
    $menu['errors'][] = t("Couldn't open the uploaded file for reading.");
    return $menu;
  }

  $level = $current_line = $empty_lines = 0;
  while ($line = fgets($handle)) {
    $current_line++;

    // Skip empty lines.
    if (preg_match('/^\s*$/', $line)) {
      $empty_lines++;
    }
    else {
      $menuitem = menu_import_parse_line($line, $level, $weights, $parents, $options);
      if ($menuitem['error']) {
        $menu['errors'][] = t('Error on line @line_number: @error.', array('@line_number' => $current_line, '@error' => $menuitem['error']));
      }
      $menu[$current_line] = $menuitem;
      $menu[$menuitem['parent']]['children'][] = $current_line;

      $level = $menuitem['level'];
      $parents[$level] = $current_line;
      $weights[$level] = $menuitem['weight'];
    }
  }

  if ($empty_lines) {
    $menu['errors'][] = t('Empty lines skipped: @line_number.', array('@line_number' => $empty_lines));
  }

  fclose($handle);

  return $menu;
}

/**
 * Text parser function. Reads through the text and constructs the menu.
 *
 * @param $text
 *   Text containing the menu structure.
 *
 * @see menu_import_parse_menu_from_file()
 */
function menu_import_parse_menu_from_string($text, $menu_name, array $options) {
  $menu = array(
    'errors' => array(),
    0 => array(
      'menu_name' => $menu_name,
      'children' => array(),
    )
  );

  // Keep track of actual weights per level.
  $weights = array();
  // Keep track of actual parents per level.
  $parents = array();

  $level = $current_line = $empty_lines = 0;
  $lines = explode("\n", $text);
  foreach ($lines as $line) {
    $current_line++;

    // Skip empty lines.
    if (preg_match('/^\s*$/', $line)) {
      $empty_lines++;
    }
    else {
      $menuitem = menu_import_parse_line($line, $level, $weights, $parents, $options);
      if ($menuitem['error']) {
        $menu['errors'][] = t('Error on line @line_number: @error.', array('@line_number' => $current_line, '@error' => $menuitem['error']));
      }
      $menu[$current_line] = $menuitem;
      $menu[$menuitem['parent']]['children'][] = $current_line;

      $level = $menuitem['level'];
      $parents[$level] = $current_line;
      $weights[$level] = $menuitem['weight'];
    }
  }

  if ($empty_lines) {
    $menu['errors'][] = t('Empty lines skipped: @line_number.', array('@line_number' => $empty_lines));
  }

  return $menu;
}

/**
 * Import menu items.
 *
 * @param $menu
 *   An associative array containing the menu structure.
 * @param $options
 *   An associative array of import options.
 *   - link_to_content: look for existing nodes and link to them
 *   - create_content: create new content (also if link_to_content not set)
 *   - node_type: node type
 *   - node_language: node language
 *   - node_body: node body
 *   - node_author: node author
 *   - node_status: node status
 *
 * @return
 *   Array of different statistics accumulated during the import.
 */
function menu_import_save_menu($menu, $options) {
  $nodes_deleted_cnt = $unknown_links_cnt = $external_links_cnt = 0;
  $nodes_matched_cnt = $nodes_new_cnt = $failed_cnt = 0;

  // Delete existing menu items.
  $menu_name = $menu[0]['menu_name'];
  if ($options['remove_menu_items']) {
    $nodes_deleted_cnt = menu_import_delete_menuitems($menu_name);
  }

  $menu[0]['mlid'] = 0;

  foreach ($menu as $item) {
    if (!isset($item['children'])) {
      continue;
    }

    foreach ($item['children'] as $index) {
      $menuitem = $menu[$index];
      $menuitem['plid'] = $menu[$menuitem['parent']]['mlid'];
      $menuitem['menu_name'] = $menu_name;

      // Do not create nodes for external links.
      if ($menuitem['external']) {
        $external_links_cnt++;
      }
      // Internal link to not-a-node content.
      elseif ($menuitem['link_path'] && substr($menuitem['link_path'], 0, 4) != 'node') {
        $unknown_links_cnt++;
      }
      // Handle links to nodes or missing links.
      else {
        // Node exists.
        if ($menuitem['nid']) {
          if ($options['create_content'] || $options['link_to_content']) {
            // Need to create new content since no linking was selected.
            if (!$options['link_to_content']) {
              menu_import_delete_menuitem($menuitem);
              $options['node_title'] = $menuitem['link_title'];
              $nid = menu_import_create_node($options);
              $menuitem['nid'] = $nid;
              $nodes_new_cnt++;
              $menuitem['link_path'] = 'node/' . $menuitem['nid'];
            }
            else {
              menu_import_delete_menuitem($menuitem);
              $nodes_matched_cnt++;
              $menuitem['link_path'] = 'node/' . $menuitem['nid'];
            }
          }
        }
        // Node doesn't exist.
        else {
          // Create new link and node.
          if ($options['create_content']) {
            menu_import_delete_menuitem($menuitem);
            $options['node_title'] = $menuitem['link_title'];
            $nid = menu_import_create_node($options);
            $menuitem['nid'] = $nid;
            $nodes_new_cnt++;
            $menuitem['link_path'] = 'node/' . $menuitem['nid'];
          }
          // Recreate menu item.
          else {
            menu_import_delete_menuitem($menuitem);
            $unknown_links_cnt++;
            $menuitem['link_path'] = '<front>';
          }
        }
      }

      // Save description if allowed.
      if (isset($menuitem['description']) && $options['import_description']) {
        $menuitem['options']['attributes']['title'] = $menuitem['description'];
      }

      // Save menuitem and set mlid.
      $mlid = menu_link_save($menuitem);
      if (!$mlid) {
        $failed_cnt++;
      }
      $menu[$index]['mlid'] = $mlid;
    }
  }

  return array(
    'external_links' => $external_links_cnt,
    'unknown_links' => $unknown_links_cnt,
    'matched_nodes' => $nodes_matched_cnt,
    'new_nodes' => $nodes_new_cnt,
    'deleted_menu_items' => $nodes_deleted_cnt,
    'failed' => $failed_cnt,
  );
}

/**
 * Create new node of given content type.
 *
 * @param $options
 *   Array relevant array keys are:
 *   - node_title
 *   - node_type
 *   - node_language
 *   - node_body
 *   - node_author
 *   - node_status
 *
 * @return
 *   Node's nid field.
 */
function menu_import_create_node($options) {
  $node = new stdClass();

  $node->type     = $options['node_type'];
  $node->language = $options['node_language'];
  $node->title    = $options['node_title'];

  $node->body[$node->language][0]['value']   = $options['node_body'];
  $node->body[$node->language][0]['summary'] = text_summary($options['node_body']);
  $node->body[$node->language][0]['format']  = 'filtered_html';

  $node->status = $options['node_status'];
  $node->uid    = $options['node_author'];

  node_save($node);
  return $node->nid;
}

/**
 * Delete nodes attached to menu and menu items.
 *
 * @param $menu_name
 *   The machine name of the menu.
 * @param $with_nodes
 *   Delete nodes as well. Not used, reserved for future.
 *
 * @return
 *   Number of items deleted .
 */
function menu_import_delete_menuitems($menu_name, $with_nodes = FALSE) {
  $menuitems = db_select('menu_links', 'ml')
    ->fields('ml', array('mlid', 'link_path'))
    ->condition('ml.menu_name', $menu_name)
    ->execute()->fetchAllAssoc('mlid', PDO::FETCH_ASSOC);

  $deleted_cnt = 0;
  foreach ($menuitems as $menuitem) {
    db_delete('menu_links')->condition('mlid', $menuitem['mlid'])->execute();
    if ($with_nodes) {
      // Delete nodes only
      $link = explode('/', $menuitem['link_path']);
      if ($link[0] == 'node' && is_numeric($link[1])) {
        $nid = $link[1];
        db_delete('node')->condition('nid', $nid)->execute();
        db_delete('node_revision')->condition('nid', $nid)->execute();
      }
    }
    $deleted_cnt++;
  }

  return $deleted_cnt;
}

/**
 * Delete menu item by nid.
 *
 * @param $menuitem
 *   Array describing the menu item.
 */
function menu_import_delete_menuitem($menuitem) {
  db_delete('menu_links')
    ->condition('menu_name', $menuitem['menu_name'])
    ->condition('link_path', 'node/' . $menuitem['nid'])
    ->execute();
}
