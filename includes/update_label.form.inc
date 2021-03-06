<?php

function islandora_datastreams_io_update_label_form(array $form, array &$form_state, $batch = NULL) {
  module_load_include('inc', 'islandora_datastreams_io', 'includes/utilities');

  $upload_size = min((int) ini_get('post_max_size'), (int) ini_get('upload_max_filesize'));
  $pids_attributes = array();
  $pids_fetch_method = isset($_SESSION['pids_fetch_method']) ? $_SESSION['pids_fetch_method'] : ISLANDORA_DATASTREAMS_IO_FETCH_LISTPIDS;

  $pid_values = (isset($_SESSION['pids_value']) && $pids_fetch_method == ISLANDORA_DATASTREAMS_IO_FETCH_LISTPIDS) ?
    json_decode($_SESSION['pids_value'], true) :
    array();
  // This will set the radio button to the appropriate choice for fetch method, and
  // the javascript will hide / show the appropriate form elements.
  if (isset($_SESSION['pids_value']) && $pids_fetch_method == ISLANDORA_DATASTREAMS_IO_FETCH_LISTPIDS) {
    $form_state['input']['pids_fetch_method'] = $pids_fetch_method;
    $pids_attributes['readonly'] = 'readonly';
    $pids_attributes['disabled'] = 'disabled';
  }

  $default_pids_value = implode('
', $pid_values);
  $fetch_methods = islandora_datastreams_io_get_fetch_methods($pid_values);
  $tmp_model_options = islandora_datastreams_io_get_models();

  $extensions = array('xsl','xslt');
  
  $tmp_collections = islandora_datastreams_io_get_solr_options('RELS_EXT_hasModel_uri_ms:info\:fedora\/islandora\:collectionCModel', 'PID', 'fgs_label_s');
  asort($tmp_collections);
  $collections = array();
  foreach ($tmp_collections as $k => $collection) {
    $collections['info:fedora/' . $k] = $collection;
  }

  foreach ($tmp_model_options as $k => $model) {
    $model_options['info:fedora/' . $k] = $model;
  }

  $markup_export = '<p>' . t('This will allow a specific datastream from a given set of objects to be downloaded. ' .
          'A set of objects can be defined by a Solr query, by collection membership, by content ' .
          'model, or by a list of PID values.') . '</p>';

  $form['update_labelform'] = array(
    'form_description' => array('#markup' => $markup_export),
    'pids' => array(
      '#type' => 'hidden',
      '#default_value' => $default_pids_value,
    ),
    'objectsformset' => array(
      '#type' => 'fieldset',
      '#title' => t('Select objects'),
      '#collapsible' => TRUE,
      '#description' => t('For the export, select the method to select the objects.'),
      'pids_fetch_method' => array(
        '#type' => 'radios',
        '#options' => $fetch_methods,
        '#default_value' => ((isset($form_state['input']['pids_fetch_method']) &&
          ($form_state['input']['pids_fetch_method']) <> '') ?
          $form_state['input']['pids_fetch_method'] : ''),
      ),
      'list_of_pids' => array(
        '#type' => 'textarea',
        '#title' => t('PID values'),
        '#attributes' => $pids_attributes,
        '#description' => t('PID values, one on each line.'),
        '#default_value' => $default_pids_value,
        '#group' => 'update_labelform',
      ),
      'solr_query' => array(
        '#type' => 'textarea',
        '#title' => t('Solr query'),
        '#default_value' => (isset($form_state['input']['solr_query']) ? $form_state['input']['solr_query'] : $default_pids_value),
        '#description' => 'Use the query "q" section of any Solr query - ' .
          l(variable_get('islandora_solr_url', 'localhost:8080/solr'), variable_get('islandora_solr_url', 'localhost:8080/solr')) .
          ' do not try to use filter queries "fq" or field lists "fl" or any special Solr options for this query.',
        '#group' => 'update_labelform',
      ),
      'collection' => array(
        '#type' => 'select',
        '#description' => 'Select collection',
        '#title' => t('Collection'),
        '#options' => $collections,
        '#default_value' => (isset($form_state['input']['collection']) ? $form_state['input']['collection'] : $default_pids_value),
        '#size' => 10,
        '#group' => 'update_labelform',
      ),
      'model' => array(
        '#type' => 'select',
        '#description' => 'Select model',
        '#title' => t('Model'),
        '#options' => $model_options,
        '#default_value' => (isset($form_state['input']['model']) ? $form_state['input']['model'] : $default_pids_value),
        '#size' => 7,
        '#group' => 'update_labelform',
      ),
    ),
    'submit' => array(
      '#type' => 'submit',
      '#value' => t('Update'),
    ),
  );

  return $form;
}

function islandora_datastreams_io_update_label_form_submit(array $form, array &$form_state) {
  module_load_include('inc', 'islandora_datastreams_io', 'includes/utilities');
  global $user;

  $pids_fetch_method = $form_state['values']['pids_fetch_method'];
  $pids = NULL;

  // Unset the _SESSION value here.
  if (isset($_SESSION['pids_value']) && isset($_SESSION['pids_fetch_method'])) {
    unset($_SESSION['pids_value']);
    unset($_SESSION['pids_fetch_method']);
  }

  // Depending on the pids_fetch_method value, the PID values are derived one of 4 different ways:
  switch ($pids_fetch_method) {
    // 0 - Solr query
    case ISLANDORA_DATASTREAMS_IO_FETCH_SOLR:
      $solr_query = $form_state['values']['solr_query'];
      drupal_set_message('Lookup PIDS for Solr query: "' . $solr_query . '"');
      $pids = islandora_datastreams_io_get_pids_solr_query($solr_query);
      break;

    // 1 - List of PIDS
    case ISLANDORA_DATASTREAMS_IO_FETCH_LISTPIDS:
      $pids = $form_state['values']['list_of_pids'];
      break;

    // 2 - Collection
    case ISLANDORA_DATASTREAMS_IO_FETCH_COLLECTION:
      $collection = $form_state['values']['collection'];
      drupal_set_message('Lookup PIDS for Collection: "' . $collection . '"');
      $pids = islandora_datastreams_io_get_pids_collection($collection);
      break;

    // 3 - Model
    case ISLANDORA_DATASTREAMS_IO_FETCH_MODEL:
      $model = $form_state['values']['model'];
      drupal_set_message('Lookup PIDS for fedora Model: "' . $model . '"');
      $pids = islandora_datastreams_io_get_pids_model($model);
      break;
  }
  if ($pids) {
    $bad_pids = $skipped = array();
    $updated_count = 0;
    $pids_arr = explode("\r\n", $pids);

    foreach ($pids_arr as $pid) {
      $pid = trim($pid);
      $islandora_object = islandora_object_load($pid);
      if (is_object($islandora_object)) {
        // run the $transform on the datastream

        if (islandora_datastreams_io_update_label($islandora_object)) {
          $updated_count++;
        }
        else {
          $skipped[] = $pid;
        }
      }
      else {
        $bad_pids[] = $pid;
      }
    }
    if (count($skipped) > 0) {
      drupal_set_message('At least one of datastream for the objects could not be updated.  These are: \'' . implode("', '", $relationship_skipped) . '\'', 'info');
    }
    if (count($bad_pids) > 0) {
      drupal_set_message('At least one of the PID values were not able to be loaded as an object.  These are: \'' . implode("', '", $bad_pids) . '\'', 'warning');
    }
    drupal_set_message('The object labels have been updated.');
  }
  else {
    drupal_set_message(t('There were no PID values for this export.  Please try again.'));
  }
}

function islandora_datastreams_io_update_label($islandora_object) {
  // Get the object's datastream saved to a temp file
  $object_MODS = $islandora_object['MODS']; // ->getContent();
  // $title
  // Pull out the titleInfo/title and compare this to the current object->label
  // $islandora_object->label = 'updated';
  $tempFilename = tempnam("/tmp", "MODS_xml_initial_");
  // save the html body to a file -- datastream can load from the file
  $mods_file = $object_MODS->getContent($tempFilename);
  islandora_datastreams_io_doLabel($islandora_object, $mods_file);

  // ISLANDORA-189
  unlink($tempFilename);
  return ($got_datastream && (isset($islandora_object[$dsid])));
}

/**
 * This will update the object label based on the MODS titleInfo title value.
 */
function islandora_datastreams_io_doLabel($object, $mods_content) {
  echo $mods_content . "\n-------------------------------\n\n";

  $mods_xml = new SimpleXMLElement($mods_content);
  $mods_xml->registerXPathNamespace('mods', 'http://www.loc.gov/mods/v3');

  $title_results = $mods_xml->xpath('//mods:mods[1]/mods:titleInfo/mods:title');
  if (is_array($title_results)) {
    $titles = $title_results[0];
    if (is_object($titles)) {
      $titles_arr = (array) $titles;
      if (is_array($titles_arr)) {
        $title = trim($titles_arr[0]);
        echo $title . "\n";
        if ($object->label <> $title) {
          $object->label = $title;
        }
      }
    }
  }
  return;
}

