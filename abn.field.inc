<?php
/**
 * @file
 * Field definition for ABN fields.
 */

/**
 * Define constants.
 */

// Intervals in seconds for cache expiry times that map roughly to common
// "calendar" intervals. For our usage fast is better than accurate.
define('ABN_EXPIRES_NEVER', -1);
define('ABN_EXPIRES_IMMEDIATELY', 0);
define('ABN_EXPIRES_DAY', 60 * 60 * 24);
define('ABN_EXPIRES_WEEK', 60 * 60 * 24 * 7);
define('ABN_EXPIRES_FORTNIGHT', 60 * 60 * 24 * 7 * 2);
// Four weeks.
define('ABN_EXPIRES_MONTH', 60 * 60 * 24 * 7 * 4);
// Quarter of a year.
define('ABN_EXPIRES_QUARTER', 60 * 60 * 24 * 7 * 13);
// Half of a year.
define('ABN_EXPIRES_SEMESTER', 60 * 60 * 24 * 7 * 26);
// One year.
define('ABN_EXPIRES_YEAR', 60 * 60 * 24 * 7 * 52);

// Defaults for field settings
define('ABN_SYNC_DEFAULT', TRUE);
define('ABN_EXPIRES_DEFAULT', ABN_EXPIRES_MONTH);


/**
 * Implements hook_field_info().
 */
function abn_field_info() {
  return array(
    'abn' => array(
      'label' => t('Australian Business Number'),
      'description' => t('Store an ABN (Australian Business Number) in the database.'),
      'default_widget' => 'abn_text',
      'default_formatter' => 'abn_plain',
      'settings' => array(
        'sync' => ABN_SYNC_DEFAULT,
        'expires' => ABN_EXPIRES_DEFAULT,
      ),
    ),
  );
}

/**
 * Implements hook_field_settings_form().
 */
function abn_field_settings_form($field, $instance, $has_data) {
  $form = array();
  $form['sync'] = array(
    '#type' => 'checkbox',
    '#title' => t('Request public data associated with each ABN from the ABR (Recommended).'),
    '#description' => t('Without sending requests to the ABR this field can only display each ABN in plain text format, ie. no company trading or historical information.'),
    '#default_value' => isset($field['settings']['sync']) ? $field['settings']['sync'] : ABN_SYNC_DEFAULT,
  );

  $form['expires'] = array(
    '#type' => 'select',
    '#title' => t('Minimum lifetime of ABR data'),
    '#description' => t('Smaller values improve freshness of ABR data, at the expense of scalability.'),
    '#options' => array(
      ABN_EXPIRES_IMMEDIATELY => 'No minimum lifetime',
      ABN_EXPIRES_DAY => '1 day',
      ABN_EXPIRES_WEEK => '1 week',
      ABN_EXPIRES_FORTNIGHT => '2 weeks',
      ABN_EXPIRES_MONTH => '1 month (default)',
      ABN_EXPIRES_QUARTER => '3 months',
      ABN_EXPIRES_SEMESTER => '6 months',
      ABN_EXPIRES_YEAR => '1 year',
      ABN_EXPIRES_NEVER => 'Never expire data',
    ),
    '#default_value' => isset($field['settings']['expires']) ? $field['settings']['expires'] : ABN_EXPIRES_DEFAULT,
  );

  return $form;
}

/**
 * Implements hook_field_validate().
 */
function abn_field_validate($entity_type, $entity, $field, $instance, $langcode, $items, &$errors) {
  foreach ($items as $delta => $item) {
    if (!empty($item['abn'])) {
      // Don't consider whitespace in validation.
      $sans_space = preg_replace('/\s+/', '', $item['abn']);

      if (!ctype_digit($sans_space)) {
        $errors[$field['field_name']][$langcode][$delta][] = array(
          'error' => 'abn_invalid',
          'message' => t('An ABN must contain only numbers.'),
        );
      }

      // An ABN is 11 digits.
      if (drupal_strlen($sans_space) !== 11) {
        $errors[$field['field_name']][$langcode][$delta][] = array(
          'error' => 'abn_invalid',
          'message' => t('An ABN must be exactly 11 digits long.'),
        );
      }
    }
  }
}

/**
 * Implements hook_field_presave().
 */
function abn_field_presave($entity_type, $entity, $field, $instance, $langcode, &$items) {
  foreach($items as $key => $item) {
    _abn_sync_field_item($items[$key]);
  }
}

/**
 * Implements hook_field_is_empty().
 */
function abn_field_is_empty($item, $field) {
  return empty($item['abn']);
}

/**
 * Implements hook_field_formatter_info().
 */
function abn_field_formatter_info() {
  return array(
    'abn_plain' => array(
      'label' => t('ABN, as plain text (default)'),
      'field types' => array('abn'),
    ),
  );
}

/**
 * Implements hook_field_formatter_view().
 */
function abn_field_formatter_view($entity_type, $entity, $field, $instance, $langcode, $items, $display) {
  $element = array();
  foreach ($items as $delta => $item) {
    $element[$delta] = array(
      '#type' => 'html_tag',
      '#tag' => 'span',
      '#attributes' => array(
        'class' => 'abn',
      ),
      '#value' => check_plain($item['abn']),
    );
  }

  return $element;
}

/**
 * Implements hook_field_widget_info().
 */
function abn_field_widget_info() {
  return array(
    'abn_text' => array(
      'label' => t('ABN'),
      'field types' => array('abn'),
    ),
  );
}

/**
 * Implements hook_field_widget_form().
 */
function abn_field_widget_form(&$form, &$form_state, $field, $instance, $langcode, $items, $delta, $element) {
  $value = isset($items[$delta]['abn']) ? $items[$delta]['abn'] : '';
  $widget = $element;

  $widget += array(
    '#type' => 'textfield',
    '#maxlength' => 16,
    '#size' => 14,
    '#default_value' => $value,
    '#delta' => $delta,
  );

  $element['abn'] = $widget;
  return $element;
}
