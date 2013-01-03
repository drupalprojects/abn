<?php
/**
 * @file
 * Field definition for ABN fields.
 */

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
    '#title' => t('Request public data associated with each ABN in this field from the ABR (Recommended).'),
    '#description' => t('Without sending requests to the ABR this field can only display each ABN in plain text format, ie. no company trading or historical information.'),
    '#default_value' => isset($field['settings']['sync']) ? $field['settings']['sync'] : ABN_SYNC_DEFAULT,
  );

  $form['expires'] = array(
    '#type' => 'select',
    '#title' => t('Minimum lifetime of ABR data'),
    '#description' => t('Smaller values improve freshness of ABR data, at the expense of scalability. This setting does not prevent data for this field being updated as content is saved.'),
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

  $form['sync_on'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Events that can trigger an update from the ABR'),
    '#description' => t('If nothing is selected here ABR data will only be updated as content is saved. Only expired field data will be updated.'),
    '#options' => array(
      ABN_SYNC_ON_CRON => 'Cron runs',
      ABN_SYNC_ON_LOAD => 'Viewing this field',
    ),
    '#default_value' => isset($field['settings']['sync_on']) ? $field['settings']['sync_on'] : drupal_map_assoc(array(ABN_SYNC_ON_CRON, ABN_SYNC_ON_LOAD)),
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
      $abn = preg_replace('@\s+@', '', $item['abn']);

      if (!ctype_digit($abn)) {
        $errors[$field['field_name']][$langcode][$delta][] = array(
          'error' => 'abn_invalid',
          'message' => t('An ABN must contain only numbers.'),
        );
      }

      // An ABN is 11 digits.
      if (drupal_strlen($abn) !== 11) {
        $errors[$field['field_name']][$langcode][$delta][] = array(
          'error' => 'abn_invalid',
          'message' => t('An ABN must be exactly 11 digits long.'),
        );
      }
    }
  }
}

/**
 * Implements hook_field_load().
 */
function abn_field_load($entity_type, $entities, $field, $instances, $langcode, &$items, $age) {
  // If we're allowed to sync on load, find all $item arrays and sync them if
  // required.
  if ($field['settings']['sync_on'][ABN_SYNC_ON_LOAD] === ABN_SYNC_ON_LOAD) {
    // Loop over all loaded entities.
    foreach ($items as $entity_id => $field_values) {
      // Loop over all values for the current entity.
      foreach ($field_values as $delta => $item) {
        abn_sync_field_item($items[$entity_id][$delta], $field);
      }
    }
  }
}

/**
 * Implements hook_field_presave().
 */
function abn_field_presave($entity_type, $entity, $field, $instance, $langcode, &$items) {
  foreach($items as $key => $item) {
    // The ABN in this field may have changed so don't respect expiry dates for
    // ABR data.
    abn_sync_field_item($items[$key], $field, FALSE);
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

  $base_widget = $element;
  $base_widget['#delta'] = $delta;

  $abn_widget = $base_widget + array(
    '#type' => 'textfield',
    '#maxlength' => 16,
    '#size' => 14,
    '#default_value' => isset($items[$delta]['abn']) ? $items[$delta]['abn'] : '',
    // Display the name registered to the ABN if we know it.
    '#field_suffix' => isset($items[$delta]['abn_name']) ? $items[$delta]['abn_name'] : '',
  );
  $element['abn'] = $abn_widget;

  // All data pulled from the ABR is stored in hidden fields that the user
  // cannot access directly.
  $hidden_fields = array(
    'abn_name',
    'abn_status',
    'abn_gst_start',
    'abn_type',
    'abn_type_code',
    'abn_address_state',
    'abn_address_postcode',
    'updated',
  );

  $hidden_widget = $base_widget + array(
    '#type' => 'hidden',
    '#access' => FALSE,
  );

  foreach($hidden_fields as $hidden_field) {
    $element[$hidden_field] = $hidden_widget + array(
      '#default_value' => isset($items[$delta][$hidden_field]) ? $items[$delta][$hidden_field] : '',
    );
  }

  return $element;
}
