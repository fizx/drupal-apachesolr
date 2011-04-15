<?php

/**
 * @file
 *   Administrative settings for searching.
 */

/**
 *
 */
function apachesolr_search_page_list_page() {
  $build = array();

  // Build the sortable table header.
  $header = array(
    'label' => array('data' => t('Name'), 'field' => 's.label'),
    'path' => array('data' => t('Path'), 'field' => 's.search_path'),
    'server' => array('data' => t('Server'), 'field' => 'ser.server_name'),
    'operations' => array('data' => t('Operations'), 'colspan' => '2'),
  );

  // Initializes the search page query.
  $query = db_select('apachesolr_search_page', 's')
    ->fields('s', array('label', 'name', 'page_title', 'description', 'search_path'))
    ->extend('PagerDefault')
    ->extend('TableSort')
    ->limit(20)
    ->orderByHeader($header);

  $query->addField('ser', 'name', 'server_name');

  // Joins on the server table.
  $query->leftJoin('apachesolr_server', 'ser', 's.server_id = ser.server_id');

  $rows = array();
  foreach ($query->execute() as $record) {
    $row = array();

    $row[] = check_plain($record->label);

    $row[] = array(
      'data' => array(
        '#type' => 'link',
        '#title' => $record->search_path,
        '#href' => $record->search_path,
      ),
    );

    $row[] = check_plain($record->server_name);

    // Operations
    $row[] = array('data' => l(t('Edit'), 'admin/config/search/apachesolr/search-pages/' . $record->name . '/edit'));
    $row[] = array('data' => l(t('Delete'), 'admin/config/search/apachesolr/search-pages/' . $record->name . '/delete'));
      
    $rows[] = $row;
  }

  $build['list'] = array(
    '#theme' => 'table',
    '#header' => $header,
    '#rows' => $rows,
    '#empty' => t('No available search pages.'),
  );

  return $build;
}

/**
 *
 */
function apachesolr_search_page_settings_form($form, &$form_state, $search_page = NULL) {

  // Initializes form with common settings.
  $form['label'] = array(
    '#type' => 'textfield',
    '#title' => t('Name'),
    '#description' => '',
    '#required' => TRUE,
    '#size' => 30,
    '#maxlength' => 32,
    '#weight' => -50,
    '#default_value' => !empty($search_page->label) ? $search_page->label : '',
  );

  $form['name'] = array(
    '#type' => 'machine_name',
    '#maxlength' => 32,
    '#required' => TRUE,
    '#machine_name' => array(
      'exists' => 'apachesolr_search_page_exists',
      'source' => array('label'),
    ),
    '#description' => '',
    '#weight' => -40,
    '#default_value' => !empty($search_page->name) ? $search_page->name : '',
    '#disabled' => !empty($search_page),
  );

  $form['info']['description'] = array(
    '#title' => t('Description'),
    '#type' => 'textfield',
    '#required' => FALSE,
    '#maxlength' => 255,
    '#description' => '',
    '#default_value' => !empty($search_page->description) ? $search_page->description : '',
  );

  $form['info']['search_path'] = array(
    '#title' => t('Path'),
    '#type' => 'textfield',
    '#required' => TRUE,
    '#maxlength' => 255,
    '#description' => t('For example: search/my-search-page. Search keywords will appear at the end of the path.'),
    '#default_value' => !empty($search_page->search_path) ? $search_page->search_path : '',
  );

  $form['info']['filters'] = array(
    '#title' => t('Default filters'),
    '#type' => 'textfield',
    '#required' => FALSE,
    '#maxlength' => 255,
    '#description' => t('A comma-separated list of lucene filter queries to apply by default. E.g. "bundle:blog, is_uid:(1 OR 2 OR 3)"'),
    '#default_value' => !empty($search_page->settings['fq'])  ? implode(', ', $search_page->settings['fq']) : '',
  );

  $form['info']['page_title'] = array(
    '#title' => t('Title'),
    '#type' => 'textfield',
    '#required' => TRUE,
    '#maxlength' => 255,
    '#description' => '',
    '#default_value' => !empty($search_page->page_title) ? $search_page->page_title : '',
  );

  // Sets the descriptions and button text.
  $form['label']['#description'] = t('The human-readable name of the search page configuration.');
  $form['name']['#description'] = t('A unique machine-readable name for the search page configuration. It must only contain lowercase letters, numbers, and underscores.');
  $form['info']['description']['#description'] = t('The description of the search page configuration.');

  $id = variable_get('apachesolr_default_server', 'solr');
  $servers = apachesolr_load_all_servers();

  $options = array();
  foreach ($servers as $id => $server) {
    $options[$id] = $server['name'];
  }

  $form['info']['server_id'] = array(
    '#title' => t('Server'),
    '#type' => 'select',
    '#options' => $options,
    '#default_value' => !empty($search_page->server_id) ? $search_page->server_id : NULL,
    '#description' => t('The server that is searched by this search page.'),
    '#required' => TRUE,
    '#weight' => -30,
  );

  $form['actions'] = array(
    '#type' => 'actions',
    '#weight' => 20,
  );

  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save configuration'),
  );

  $form['actions']['submit_list'] = array(
    '#type' => 'submit',
    '#value' => t('Save and go back to list'),
  );

  $form['actions']['cancel'] = array(
    '#type' => 'link',
    '#title' => t('Cancel'),
    '#href' => 'admin/config/search/apachesolr/search-pages',
  );

  $form['#submit'][] = 'apachesolr_search_page_settings_form_submit';

  return $form;
}

function apachesolr_search_page_settings_form_validate($form, &$form_state) {
  // Performs basic validation of the menu path.
  if (url_is_external($form_state['values']['search_path'])) {
    form_set_error('search_path', t('Path must be local.'));
  }
  $form_state['values']['search_path'] = trim($form_state['values']['search_path'], '/');
  if (empty($form_state['values']['search_path'])) {
    form_set_error('search_path', t('Path required.'));
  }
}

/**
 * Processes apachesolr_search_page_settings_form form submissions.
 */
function apachesolr_search_page_settings_form_submit($form, &$form_state) {
  $settings = array();
  if ($form_state['values']['filters']) {
    foreach(explode(',', $form_state['values']['filters']) as $string) {
      $string = trim($string);
      // Minimal validation.  ':' must exist and can't be the 1st char..
      if (strpos($string, ':')) {
        $settings['fq'][] = $string;
      }
    }
  }
  db_merge('apachesolr_search_page')
    ->key(array('name' => $form_state['values']['name']))
    ->fields(array(
      'label' => $form_state['values']['label'],
      'name' => $form_state['values']['name'],
      'description' => $form_state['values']['description'],
      'server_id' => $form_state['values']['server_id'],
      'search_path' => $form_state['values']['search_path'],
      'page_title' => $form_state['values']['page_title'],
      'settings' => serialize($settings),
    ))
    ->execute();

  // Saves our values in the database, sets redirect path on success.
  drupal_set_message(t('The configuration options have been saved.'));

  if (t('Save and go back to list') == $form_state['clicked_button']['#value']) {
      $form_state['redirect'] = 'admin/config/search/apachesolr/search-pages';
  }

  // Menu rebuild needed to pick up search path.
  menu_rebuild();
}

/**
 * Deletes a single search page configuration.
 */
function apachesolr_search_delete_search_page_confirm($form, &$form_state, $search_page) {

  // Sets values required for deletion.
  $form['name'] = array('#type' => 'value', '#value' => $search_page->name);
  $form['label'] = array('#type' => 'value', '#value' => $search_page->label);

  // Sets the message, or the title of the page.
  $message = t(
    'Are you sure you want to delete the %label search page configuration?',
    array('%label' => $form['label']['#value'])
  );

  // Builds caption.
  $caption = '<p>';
  $caption .= t(
    'The %label search page configuration will be deleted.',
    array('%label' => $form['label']['#value'])
  );
  $caption .= '</p>';
  $caption .= '<p><strong>' . t('This action cannot be undone.') . '</strong></p>';

  // Finalizes and returns the confirmation form.
  $return_path = 'admin/config/search/apachesolr/search-pages';
  $button_text = t('Delete configuration');
  return confirm_form($form, $message, $return_path, $caption, $button_text);
}

/**
 * Process content type delete confirm submissions.
 */
function apachesolr_search_delete_search_page_confirm_submit($form, &$form_state) {
  // Deletes the index configuration settings.
  // @todo Invoke a hook that allows backends and indexers to delete their stuff.
  db_delete('apachesolr_search_page')
    ->condition('name', $form_state['values']['name'])
    ->execute();

  // Sets message, logs action.
  drupal_set_message(t(
    'The %label search page configuration has been deleted.',
    array('%label' => $form_state['values']['label'])
  ));
  watchdog('apachesolr_search', 'Deleted search page configuration "@name".', array('@name' => $form_state['values']['name']), WATCHDOG_NOTICE);

  // Rebuilds the menu cache.
  menu_rebuild();

  // Returns back to search page list page.
  $form_state['redirect'] = 'admin/config/search/apachesolr/search-pages';
}


/**
 * Menu callback - the settings form.
 */
function apachesolr_search_settings_page($server_id = NULL) {
  if (empty($server_id)) {
    $server_id = apachesolr_default_server();
  }

  // Try to fetch the schema fields.
  try {
    $solr = apachesolr_get_solr($server_id);
    $fields = $solr->getFields();
    $output = array(
      'apachesolr_server' => array(
        '#theme' => 'apachesolr_settings_title',
        '#server_id' => $server_id,
      ),
      'apachesolr_search_settings_form' => drupal_get_form('apachesolr_search_settings_form', $fields, $server_id)
    );
  }
  catch (Exception $e) {
    watchdog('Apache Solr', nl2br(check_plain($e->getMessage())), NULL, WATCHDOG_ERROR);
    drupal_set_message(nl2br(check_plain($e->getMessage())), 'warning');
    $output = t('Cannot get information about the fields in the index at this time.');
  }
  return $output;
}

/**
 * Menu callback - boosts settings form.
 */
function apachesolr_boost_settings_page($server_id = NULL) {
  if (empty($server_id)) {
    $server_id = apachesolr_default_server();
  }

  // Initializes output with information about which server's setting we are
  // editing, as it is otherwise not transparent to the end user.
  $output = array(
    'apachesolr_server' => array(
      '#theme' => 'apachesolr_settings_title',
      '#server_id' => $server_id,
    ),
  );

  // Adds content bias and type boost / exclusion forms.
  $output['apachesolr_search_bias_form'] = drupal_get_form('apachesolr_search_bias_form', $server_id);
  $output['apachesolr_search_type_boost_form'] = drupal_get_form('apachesolr_search_type_boost_form', $server_id);
  return $output;
}

/**
 * Form builder function to set date, comment, etc biases.
 */
function apachesolr_search_bias_form($form, &$form_state, $server_id) {

  $date_settings = apachesolr_server_variable_get($server_id, 'apachesolr_search_date_boost', '0:0');
  $comment_settings = apachesolr_server_variable_get($server_id, 'apachesolr_search_comment_boost', '0:0');
  $changed_settings = apachesolr_server_variable_get($server_id, 'apachesolr_search_changed_boost', '0:0');
  $sticky_boost = apachesolr_server_variable_get($server_id, 'apachesolr_search_sticky_boost', '0');
  $promote_boost = apachesolr_server_variable_get($server_id, 'apachesolr_search_promote_boost', '0');

  $options = array(
    '10:2000.0' => '10',
    '8:1000.0' => '9',
    '8:700.0' => '8',
    '8:500.0' => '7',
    '4:300.0' => '6',
    '4:200.0' => '5',
    '4:150.0' => '4',
    '2:150.0' => '3',
    '2:100.0' => '2',
    '1:100.0' => '1',
    '0:0' => t('Ignore'),
  );

  $weights = drupal_map_assoc(array('21.0', '13.0', '8.0', '5.0', '3.0', '2.0', '1.0', '0.8', '0.5', '0.3', '0.2', '0.1'));
  $weights['0'] = t('Ignore');

  $form['biasing'] = array(
    '#type' => 'fieldset',
    '#title' => t('Result biasing'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#description' => t('Give bias to certain properties when ordering the search results. Any value except <em>Ignore</em> will increase the score of the given type in search results. Choose <em>Ignore</em> to ignore any given property.'),
  );
  $form['biasing']['apachesolr_search_sticky_boost'] = array(
    '#type' => 'select',
    '#options' => $weights,
    '#title' => t("Sticky at top of lists"),
    '#default_value' => $sticky_boost,
    '#description' => t("Select additional bias to give to nodes that are set to be 'Sticky at top of lists'."),
  );
  $form['biasing']['apachesolr_search_promote_boost'] = array(
    '#type' => 'select',
    '#options' => $weights,
    '#title' => t("Promoted to home page"),
    '#default_value' => $promote_boost,
    '#description' => t("Select additional bias to give to nodes that are set to be 'Promoted to home page'."),
  );
  $form['biasing']['apachesolr_search_date_boost'] = array(
    '#type' => 'select',
    '#options' => $options,
    '#title' => t("More recently created"),
    '#default_value' => $date_settings,
    '#description' => t('This setting will change the result scoring so that nodes created more recently may appear before those with higher keyword matching.'),
  );
  $form['biasing']['apachesolr_search_comment_boost'] = array(
    '#type' => 'select',
    '#options' => $options,
    '#title' => t("More comments"),
    '#default_value' => $comment_settings,
    '#description' => t('This setting will change the result scoring so that nodes with more comments may appear before those with higher keyword matching.'),
  );
  $form['biasing']['apachesolr_search_changed_boost'] = array(
    '#type' => 'select',
    '#options' => $options,
    '#title' => t("More recent comments"),
    '#default_value' => $changed_settings,
    '#description' => t('This setting will change the result scoring so that nodes with the most recent comments (or most recent updates to the node itself) may appear before those with higher keyword matching.'),
  );

  return apachesolr_settings_form($form, $server_id);
}

/**
 * Form builder function to set query field weights.
 */
function apachesolr_search_settings_form($form, &$form_state, $fields, $server_id) {
  // get the current weights
  $defaults = array(
    'content' => '1.0',
    'ts_comments' => '0.5',
    'tos_content_extra' => '0.1',
    'label' => '5.0',
    'tos_name' => '3.0',
    'taxonomy_names' => '2.0',
    'tags_h1' => '5.0',
    'tags_h2_h3' => '3.0',
    'tags_h4_h5_h6' => '2.0',
    'tags_inline' => '1.0',
    'tags_a' => '0',
  );
  $qf = apachesolr_server_variable_get($server_id, 'apachesolr_search_query_fields', $defaults);
  $weights = drupal_map_assoc(array('21.0', '13.0', '8.0', '5.0', '3.0', '2.0', '1.0', '0.8', '0.5', '0.3', '0.2', '0.1'));
  $weights['0'] = t('Omit');
  if (!$qf) {
    $qf = $defaults;
  }
  if ($fields) {
    $form['apachesolr_search_query_fields'] = array(
      '#type' => 'fieldset',
      '#title' => t('Field biases'),
      '#collapsible' => TRUE,
      '#collapsed' => FALSE,
      '#tree' => TRUE,
      '#description' => t('Specify here which fields are more important when searching. Give a field a greater numeric value to make it more important. If you omit a field, it will not be searched.'),
    );
    foreach ($fields as $field_name => $field) {
      // Only indexed feids are searchable.
      if ($field->schema{0} == 'I') {
        // By default we only show text fields.  Use hook_form_alter to change.
        $form['apachesolr_search_query_fields'][$field_name] = array(
          '#access' => $field->type == 'text',
          '#type' => 'select',
          '#options' => $weights,
          '#title' => apachesolr_field_name_map($field_name),
          '#default_value' => isset($qf[$field_name]) ? $qf[$field_name] : '0',
        );
      }
    }
    // Make sure all the default fields are included, even if they have
    // no indexed content.
    foreach ($defaults as $field_name => $weight) {
      $form['apachesolr_search_query_fields'][$field_name] = array(
        '#type' => 'select',
        '#options' => $weights,
        '#title' => apachesolr_field_name_map($field_name),
        '#default_value' => isset($qf[$field_name]) ? $qf[$field_name] : $defaults[$field_name],
      );
    }

    ksort($form['apachesolr_search_query_fields']);
  }

  return apachesolr_settings_form($form, $server_id);
}

/**
 * Form builder function to set query type weights.
 */
function apachesolr_search_type_boost_form($form, &$form_state, $server_id) {

  $form['apachesolr_search_type_settings'] = array(
    '#type' => 'fieldset',
    '#title' => t('Type biasing and exclusion'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );
  $form['apachesolr_search_type_settings']['apachesolr_search_type_boosts'] = array(
    '#type' => 'item',
    '#description' => t("Specify here which node types should get a higher relevancy score in searches. Any value except <em>Ignore</em> will increase the score of the given type in search results."),
    '#tree' => TRUE,
  );

  $weights = drupal_map_assoc(array('21.0', '13.0', '8.0', '5.0', '3.0', '2.0', '1.0', '0.8', '0.5', '0.3', '0.2', '0.1'));
  $weights['0'] = t('Ignore');

  // Get the current boost values.
  $type_boosts = apachesolr_server_variable_get($server_id, 'apachesolr_search_type_boosts', array());
  $names = node_type_get_names();

  foreach ($names as $type => $name) {
    $form['apachesolr_search_type_settings']['apachesolr_search_type_boosts'][$type] = array(
      '#type' => 'select',
      '#title' => t('%type type content bias', array('%type' => $name)),
      '#options' => $weights,
      '#default_value' => isset($type_boosts[$type]) ? $type_boosts[$type] : 0,
    );
  }

  $form['apachesolr_search_type_settings']['apachesolr_search_excluded_types'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Types to exclude from the search index'),
    '#options' => $names,
    '#default_value' => apachesolr_server_variable_get($server_id, 'apachesolr_search_excluded_types', array()),
    '#description' => t("Specify here which node types should be totally excluded from the search index. Content excluded from the index will never appear in any search results."),
  );
  $form['#original_excluded_types'] =  $form['apachesolr_search_type_settings']['apachesolr_search_excluded_types']['#default_value'];

  $form['#submit'][] = 'apachesolr_search_type_boost_form_submit';

  return apachesolr_settings_form($form, $server_id);
}

/**
 * Submit callback for apachesolr_search_type_boost_form().
 *
 * This is called before system_settings_form_submit().
 */
function apachesolr_search_type_boost_form_submit($form, &$form_state) {
  $old_excluded_types = $form['#original_excluded_types'];
  $new_excluded_types = $form_state['values']['apachesolr_search_excluded_types'];
  // Check whether we are resetting the values.
  if ($form_state['clicked_button']['#value'] == t('Reset to defaults')) {
    $new_excluded_types = array();
  }

  foreach ($new_excluded_types as $type => $excluded) {
    // Remove newly omitted node types.
    // Note - we omit a check on empty($old_excluded_types[$type]) so that
    // the admin can re-submit this page if the delete operation fails.
    if (!empty($new_excluded_types[$type])) {
      try {
        $solr = apachesolr_get_solr($form['#server_id']);
        $solr->deleteByQuery("bundle:$type");
        apachesolr_index_set_last_updated(REQUEST_TIME);
      }
      catch (Exception $e) {
        watchdog('Apache Solr', nl2br(check_plain($e->getMessage())), NULL, WATCHDOG_ERROR);
        drupal_set_message(t('The Apache Solr search engine is not available. Please contact your site administrator.'), 'error');
      }
    }
  }

  foreach ($old_excluded_types as $type => $excluded) {
    // Set no longer omitted node types for reindexing.
    if (empty($new_excluded_types[$type]) && !empty($old_excluded_types[$type])) {
      $nids = db_select('node')->fields('node', array('nid'))->condition('type', $type);
      db_update('apachesolr_search_node')->fields(array('changed' => REQUEST_TIME))->condition('nid', $nids, 'IN')->execute();
    }
  }
}
