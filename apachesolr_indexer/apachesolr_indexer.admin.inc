<?php
// $Id$

/**
 * @file
 *   Administrative pages for the Apachesolr indexer module.
 */

function apachesolr_indexer_configure() {

    try {
    $solr = apachesolr_get_solr();
    // TODO: possibly clear this every page view if we are running multi-site.
    if (apachesolr_index_get_last_updated()) {
      $solr->clearCache();
    }
    $data = $solr->getLuke();
  }
  catch (Exception $e) {
    watchdog('Apache Solr', nl2br(check_plain($e->getMessage())), NULL, WATCHDOG_ERROR);
    drupal_set_message(nl2br(check_plain($e->getMessage())), "warning");
    $data = new stdClass;
    $data->fields = array();
  }

  $output = array();
  $status_header = '';
  if (isset($data->index->numDocs)) {
    $pending_docs = $delay_msg = $delete_msg = '';
    try {
      $stats_summary = $solr->getStatsSummary();

      $stats = array();
      $stats[] = t('Using schema.xml version: <strong>@schema_version</strong>', $stats_summary);
      if ($stats_summary['@core_name']) {
        $stats[] = t('Solr core name: <strong>@core_name</strong>', $stats_summary);
      }

      $stats[] = t('The server has a @autocommit_time delay before updates are processed.', $stats_summary);
      $stats[] = t('Number of pending deletions: @deletes_total', $stats_summary);
      $stats[] = t('Number of documents in index: @num', array('@num' => $data->index->numDocs));
      $stats[] = t('@pending_docs sent but not yet processed', $stats_summary);

      $output['stats'] = array(
        '#theme' => 'item_list',
        '#items' => $stats,
        '#title' => t('Solr status'),
      );
    }
    catch (Exception $e) {
      watchdog('Apache Solr', nl2br(check_plain($e->getMessage())), NULL, WATCHDOG_ERROR);
    }
  }

  if (variable_get('apachesolr_read_only', APACHESOLR_READ_WRITE) == APACHESOLR_READ_WRITE) {
    $output['apachesolr_index_action_status_header'] = array(
      '#markup' => l(t('View more details on the search index contents'), 'admin/reports/apachesolr'),
    );
  }
  else {
    drupal_set_message(t('The index is in read-only mode. Options for deleting and re-indexing are therefore not available. The index mode can be changed on the !settings_page', array('!settings_page' => l(t('settings page'), 'admin/config/search/apachesolr'))), 'warning');
  }
  
  return $output;
}

/**
 * Form builder for the Apachesolr Indexer actions form.
 *
 * @see apachesolr_indexer_action_form_delete_submit().
 */
function apachesolr_indexer_action_form() {
  $form = array();

  $form['reset'] = array(
    '#prefix' => '<div>',
    '#suffix' => '</div>',
    '#type' => 'submit',
    '#value' => t('Reset submitted content'),
    '#submit' => array('apachesolr_indexer_action_form_reset_submit'),
  );
  $form['reset_description'] = array(
    '#prefix' => '<div>',
    '#suffix' => '</div>',
    '#markup' => t('All content on the site will be queued for indexing. The documents currently in the Solr index will remain searchable. The content will be gradually resubmitted to Solr during cron runs.'),
  );

  $form['delete'] = array(
    '#prefix' => '<div>',
    '#suffix' => '</div>',
    '#type' => 'submit',
    '#value' => t('Delete index'),
    '#submit' => array('apachesolr_indexer_action_form_delete_submit'),
  );
  $form['delete_description'] = array(
    '#prefix' => '<div>',
    '#suffix' => '</div>',
    '#markup' => t('All documents in the Solr index will be deleted. This is rarely necessary unless your index is corrupt or you have installed a new schema.xml. After doing this your content will need to be resubmitted for indexing.'),
  );

  return $form;
}

/**
 * Submit handler for the Indexer actions form, delete button.
 */
function apachesolr_indexer_action_form_delete_submit($form, &$form_state) {
  $destination = array();
  if (isset($_GET['destination'])) {
    $destination = drupal_get_destination(); unset($_GET['destination']);
  }

  $form_state['redirect'] = array('admin/config/search/apachesolr/indexer/reindex/delete', array('query' => $destination));
}

/**
 * Submit handler for the Indexer actions form, delete button.
 */
function apachesolr_indexer_action_form_reset_submit($form, &$form_state) {
  $destination = array();
  if (isset($_GET['destination'])) {
    $destination = drupal_get_destination(); unset($_GET['destination']);
  }

  $form_state['redirect'] = array('admin/config/search/apachesolr/indexer/reindex/reset', array('query' => $destination));
}

/**
 * Form builder for the index delete/clear form.
 *
 * @see apachesolr_indexer_action_form_delete_confirm_submit().
 */
function apachesolr_indexer_action_form_delete_confirm($form, &$form_state) {
  return confirm_form($form,
    t('Are you sure you want to clear your index?'),
    'admin/config/search/apachesolr/indexer/reindex',
    t('This will remove all data from your index and all search results will be incomplete until your site is reindexed.'),
    t('Delete index')
  );
}

/**
 * Form builder for the index re-enqueue form.
 *
 * @see apachesolr_indexer_action_form_reset_confirm_submit().
 */
function apachesolr_indexer_action_form_reset_confirm($form, &$form_state) {
  return confirm_form($form,
    t('Are you sure you want to queue content for reindexing?'),
    'admin/config/search/apachesolr/indexer/reindex',
    t('All content on the site will be queued for indexing. The documents currently in the Solr index will remain searchable. The content will be gradually resubmitted to Solr during cron runs.'),
    t('Reindex')
  );
}

/**
 * Submit handler for the deletion form.
 */
function apachesolr_indexer_action_form_reset_confirm_submit($form, &$form_state) {
  $form_state['redirect'] = 'admin/config/search/apachesolr/indexer/reindex';

  foreach (entity_get_info() as $entity_type => $entity_info) {
    if ($entity_info['apachesolr']['indexable']) {
      $reindex_callback = apachesolr_entity_get_callback($entity_type, 'reindex callback');
      if (! $reindex_callback()) {
        drupal_set_message(t('There was an error reindexing @entity_type.  Please consult the log for more information.', array('@entity_type' => $entity_info['label'])), 'error');
        return;
      }
    }
  }

  drupal_set_message(t('All the content on your site is queued for indexing. You can wait for it to be indexed during cron runs, or you can manually reindex it.'));
}

/**
 * Submit handler for the deletion form.
 */
function apachesolr_indexer_action_form_delete_confirm_submit($form, &$form_state) {
  // Instantiate a new Solr object.
  $solr = apachesolr_get_solr();
  $query = '*:*';

  // Allow other modules to modify the delete query.
  // For example, use the site hash so that you only delete this site's
  // content:  $query = 'hash:' . apachesolr_site_hash()
  drupal_alter('apachesolr_delete_index', $query);
  $solr->deleteByQuery($query);
  $solr->commit();

  // Rebuild our tracking table.
  variable_get('apachesolr_indexer_last_run', 0);

  drupal_set_message(t('The index has been deleted.'));

  $form_state['redirect'] = 'admin/config/search/apachesolr/indexer/reindex';
}

/**
 * Form builder for the bundle configuration form.
 *
 * @see apachesolr_indexer_config_form_submit().
 */
function apachesolr_indexer_config_form() {
  $form['bundles'] = array(
    '#type' => 'markup',
    '#markup' => t('Select the entity types and bundles that should be indexed.'),
  );

  // For future extensibility, when we have multiple cores.
  $form['core'] = array(
    '#type' => 'value',
    '#value' => 'default',
  );

  foreach (entity_get_info() as $entity_type => $entity_info) {
    if ($entity_info['apachesolr']['indexable']) {
      $options = array();
      foreach ($entity_info['bundles'] as $key => $info) {
        $options[$key] = $info['label'];
      }

      $form['entities']['#tree'] = TRUE;

      $form['entities'][$entity_type] = array(
        '#type' => 'checkboxes',
        '#title' => $entity_info['label'],
        '#options' => $options,
        '#default_value' => apachesolr_indexer_get_bundles('default', $entity_type),
      );
    }
  }

  $form['submit'] = array('#type' => 'submit', '#value' => t('Save'));

  return $form;
}

/**
 * Submit handler for the bundle configuration form.
 */
function apachesolr_indexer_config_form_submit($form, &$form_state) {
  $form_values = $form_state['values'];

  $core = $form_values['core'];

  foreach ($form_values['entities'] as $entity_type => $bundles) {
    apachesolr_indexer_set_bundles($core, $entity_type, $bundles);
  }

  // Clear the entity cache, since we will be changing its data.
  entity_info_cache_clear();

  drupal_set_message(t('Your settings have been saved. You should reindex your site for the changes to take effect.'));
}
