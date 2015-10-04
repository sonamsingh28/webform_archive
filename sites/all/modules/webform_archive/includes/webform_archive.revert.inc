<?php
/**
 * @file
 * Revert Archive Webforms.
 */

/**
 * Page Callback function.
 */
function webform_archive_revert_page() {
  $title = t('Revert Archived Webforms');
  // Return webform_archive_revert_form.
  drupal_set_title($title);
  return drupal_get_form('webform_archive_revert_form');
}

/**
 * Form builder for Reverting Archived webforms.
 */
function webform_archive_revert_form($form, &$form_state) {
  $form = array();
  // Fetching list of archived webforms.
  $archived_webform_list = webform_archive_list();
  if (!empty($archived_webform_list)) {
  	// Storing list of archived webforms in form_state.
    $form_state['archived_webforms'] = $archived_webform_list;
    // Form element to select all webforms for reverting.
  	$form['webforms_archive_revert_all'] = array(
      '#type' => 'checkbox',
      '#title' => t('Revert all archived webforms'),
      '#description' => t('Select this checkbox to revert all archived webforms.'),
    );
    // Form element to select specific webform(s) to revert.
    $form['webform_archived_nid'] = array(
      '#type' => 'select',
      '#title' => t('Choose Webform(s)'),
      '#description' => t('Choose webform(s) to revert archived submissions.'),
      '#options' => $archived_webform_list,
      '#multiple' => TRUE,
    );
    $form['webform_archive_revert_submit'] = array(
      '#type' => 'submit',
      '#value' => t('Revert'),
    );
  }

  else {
    $message = t('There are no archived webforms.');
    drupal_set_message($message);
  }

  return $form;
}

/**
 * Validation function for webform archive revert form.
 */
function webform_archive_revert_form_validate($form, $form_state) {
  // If form is submitted without selecting any webform to revert, setting an error msg.
  if ($form_state['values']['webforms_archive_revert_all'] == 0) {
    if (empty($form_state['values']['webform_archived_nid'])) {
      form_set_error('webform_archived_nid', t('Please Select a Webform to Revert.'));
    }
  }
}

/**
 * Submit callback for webform archive revert form.
 */
function webform_archive_revert_form_submit($form, $form_state) {
  // Fetching webforms to revert from form_state variable.
  if ($form_state['values']['webforms_archive_revert_all'] == 1) {
    $webforms_to_revert = $form_state['archived_webforms'];
  }
  else {
    $webforms_to_revert = $form_state['values']['webform_archived_nid'];
  }
  
  // Extracting the nids of the webforms to be reverted.
  $webforms_nid = array_keys($webforms_to_revert);
  // Creating operations array form batch process.
  $operations = array(
    array('webform_archive_revert_batch_process_start',
      array($webforms_nid)),
  );
  // Creating batch array.
  $batch = array(
    'title' => t('Revert Archived Webform Submissions'),
    'operations' => $operations,
    'file' => drupal_get_path('module', 'webform_archive') . '/includes/webform_archive.revert_batch.inc',
    'finished' => 'webform_archive_revert_batch_finished',
    'init_message' => t('Revert Batch is starting...'),
    'progress_message' => t('Processed @current out of @total.'),
    'error_message' => t('Revert Batch has encountered an error.'),
  );
  // Setting up batch process.
  batch_set($batch);
}

/**
 * Fetching archieved webforms.
 *
 * @return array
 *   Archived webform list.
 */
function webform_archive_list() {
  // Fetching all archived webform nids and titles.
  $query = db_select('archive_webform', 'aw');
  $query->join('node', 'n', 'aw.nid = n.nid');
  $query->fields('n', array('nid', 'title'));
  $query->condition('n.type', 'webform');
  
  $result = $query->execute();
  // Creating array of webform nid with their respective titles.
  while ($data = $result->fetchAssoc()) {
    $archived_webform_list[$data['nid']] = $data['title'];
  }

  return $archived_webform_list;
}