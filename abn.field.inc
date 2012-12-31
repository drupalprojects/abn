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
    ),
  );
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
