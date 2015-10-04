<?php
/**
 * @file
 * Contains batch process for reverting archived webforms.
 */

/**
 * Batch process for reverting archived webforms.
 */
function webform_archive_revert_batch_process_start($webforms, &$context) {
  // Initializing context variable.
  if (!isset($context['sandbox']['progress'])) {
    $context['sandbox']['progress'] = 0;
    $context['sandbox']['current_submission'] = 0;
    $max_count = webform_archive_total_submissions($webforms);
    $context['sandbox']['max'] = $max_count;
  }
  // Setting limit variable to get no. of elements to be processed at a time.
  $limit = variable_get('webform_archive_batch_limit', WEBFORM_ARCHIVE_BATCH_LIMIT);
  // Retrieving the next group of nids in each pass.
  $archived_submissions = webform_archive_get_archived_submissions($webforms, $context['sandbox']['current_submission'], $limit);
  $archived_submitted_data = webform_archive_get_archived_submitted_data($webforms, $context['sandbox']['current_submission'], $limit);
  if (!empty($archived_submissions)) {
    // Processing current submission.
    foreach ($archived_submissions as $sid => $archived_submission) {
      if ($archived_submission != NULL) {
        $nid = $archived_submission['nid'];
        // Inserting archived submissions back into webform's table.
        drupal_write_record('webform_submissions', $archived_submission);
        // Inserting archived submitted data back into webform's table.
        drupal_write_record('webform_submitted_data', $archived_submitted_data[$sid]);
        // Updating progress information.
        $context['sandbox']['progress']++;
        $context['results'][] = $sid;
        $context['results']['webforms'][$nid] = $nid;
        $context['sandbox']['current_submission'] = $sid;
        $context['message'] = t('Now processing %sid', array('%sid' => $sid));
      }
    }
    $context['results']['last_sid'] = $context['sandbox']['current_submission'];
  }
  else {
    $context['sandbox']['progress'] = $context['sandbox']['max'] + 1;
  }

  // Inform the batch engine that we are not finished,
  // and provide an estimation of the completion level we reached.
  if ($context['sandbox']['progress'] != $context['sandbox']['max']) {
    $context['finished'] = $context['sandbox']['progress'] / $context['sandbox']['max'];
  }
}

/**
 * Revert Batch finished callback.
 */
function webform_archive_revert_batch_finished($success, $results, $operations) {
  if ($success) {
    $webforms = isset($results['webforms']) ? $results['webforms'] : array();
    $last_sid = isset($results['last_sid']) ? $results['last_sid'] : 0;
    unset($results['webforms']);
    unset($results['last_sid']);
    if (isset($webforms) && !empty($webforms)) {
      foreach ($webforms as $nid) {
        // Deleting submissions from webform archive tables.
        db_delete('archive_webform_submissions')->condition('nid', $nid)->condition('sid', $last_sid, '<=')->execute();
        db_delete('archive_webform_submitted_data')->condition('nid', $nid)->condition('sid', $last_sid, '<=')->execute();
        db_delete('archive_webform')->condition('nid', $nid)->execute();
      }
    }
    $count = count($results);
    $message = t('@count Archived Webform submissions are Reverted successfully', array('@count' => $count));
    drupal_set_message($message);
  }
  else {
    // An error occurred.
    // $operations contains the operations that remained unprocessed.
    $error_operation = reset($operations);
    $message = t('An error occurred while processing %error_operation with
      arguments: @arguments', array(
        '%error_operation' => $error_operation[0],
        '@arguments' => print_r($error_operation[1], TRUE))
      );
    if (module_exists('dblog')) {
      watchdog('webform_archive', $message, array(), WATCHDOG_CRITICAL);
      drupal_set_message(t('Some error occured while reverting archived webforms'), 'error');
    }
    else {
      drupal_set_message($message, 'error');
    }
  }
}


/**
 * Returns the total count of submissions archived for a webform.
 *
 * @param array $webforms
 *   Array of webform nid(s).
 *
 * @return int
 *   Total Count of archived webform submissions.
 */
function webform_archive_total_submissions($webforms) {
  // Gets total count of archived submissions of a webform.
  $total = db_select('archive_webform_submissions', 'aws')
  ->fields('aws', array('sid'))
  ->condition('aws.nid', $webforms, 'IN');
  $total = $total->execute();
  $total = $total->rowCount();

  return $total;
}

/**
 * Returns the archived submissions which needs to be reverted.
 *
 * @param array $webforms
 *   Webforms whose archived submissions are to be reverted.
 * @param int $current_submission_sid
 *   SID, all submissions whoses sid is greater then this current submission.
 * @param int $limit
 *   No. of submissions to return.
 *
 * @return array
 *   Webform Archived Submissions.
 */
function webform_archive_get_archived_submissions($webforms, $current_submission_sid, $limit) {
  $archived_submissions = array();
  $result = db_select('archive_webform_submissions', 'aws')
  ->fields('aws')
  ->orderBy('aws.sid', 'ASC')
  ->condition('nid', $webforms, 'IN')
  ->condition('sid', $current_submission_sid, '>')
  ->range(0, $limit);
  $result = $result->execute();
  if (isset($result) && !empty($result)) {
  	// Preparing array of submissions.
    while ($archived_submission = $result->fetchAssoc()) {
      $archived_submissions[$archived_submission['sid']] = $archived_submission;
    }
  }
  return $archived_submissions;
}

/**
 * Returns the archived submitted data which needs to be reverted.
 *
 * @param array $webforms
 *   Webforms whose archived submissions are to be reverted.
 * @param int $current_submission_sid
 *   SID, all submissions whoses sid is greater then this current submission.
 * @param int $limit
 *   No. of submissions to return.
 *
 * @return array
 *   Webform Archived Submissions data.
 */
function webform_archive_get_archived_submitted_data($webforms, $current_submission_sid, $limit) {
  $archived_data = array();
  $result = db_select('archive_webform_submitted_data', 'awsd')
  ->fields('awsd')
  ->orderBy('awsd.sid', 'ASC')
  ->condition('nid', $webforms, 'IN')
  ->condition('sid', $current_submission_sid, '>')
  ->range(0, $limit);
  $result = $result->execute();
  if (isset($result) && !empty($result)) {
  	// Preparing array of webform submitted data.
    while ($archived_submitted_data = $result->fetchAssoc()) {
      $archived_data[$archived_submitted_data['sid']] = $archived_submitted_data;
    }
  }
  return $archived_data;
}
