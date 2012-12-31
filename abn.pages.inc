<?php
/**
 * @file
 * Page callbacks for abn module
 */

/**
 * Page callback to render ajax abn lookup as JSON.
 */
function abn_lookup_callback() {
  $var = array(
    'stuff' => 'otherstuff',
  );
  drupal_json_output($var);
}
