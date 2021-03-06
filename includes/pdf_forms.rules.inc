<?php

/**
 * Implementation of hook_rules_data_type_info.
 */
function pdf_forms_rules_data_type_info() {
  $info = array();

  // Crate the data type pdf_form_data.
  $info['pdf_form'] = array(
    'label' => t('pdf form submission'),
    'parent' => 'entity',
    'group' => t('PDF Form'),
  );

  return $info;
}

/**
 * Implements hook_rules_event_info.
 */
function pdf_forms_rules_event_info() {

  $info = array();

  $defaults = array(
    'group' => t('PDF Forms'),
  );

  $info['pdf_forms_delete'] = $defaults + array(
    'label' => t('After deleting a PDF Form submission'),
    'group' => t('PDF Forms'),
    'variables' => array(
      'pdf_form' => array(
        'type' => 'pdf_form',
      ),
      'pdf' => array(
        'type' => 'file',
        'label' => t('PDF File'),
      ),
    ),
  );

  $info['pdf_forms_insert'] = $defaults + array(
    'label' => t('After saving a new PDF Form submission'),
    'group' => t('PDF Forms'),
    'variables' => array(
      'pdf_form' => array(
        'type' => 'pdf_form',
        'label' => t('PDF Form Submission'),
      ),
      'pdf' => array(
        'type' => 'file',
        'label' => t('PDF File'),
      ),
    ),
  );

  $info['pdf_forms_update'] = $defaults + array(
    'label' => t('After updating a PDF Form submission'),
    'group' => t('PDF Forms'),
    'variables' => array(
      'pdf_form' => array(
        'type' => 'pdf_form',
        'label' => t('PDF Form Submission'),
      ),
      'pdf' => array(
        'type' => 'file',
        'label' => t('PDF File'),
      ),
    ),
  );

  $info['pdf_forms_presave'] = $defaults + array(
    'label' => t('Before saving a PDF Form submission'),
    'group' => t('PDF Forms'),
    'variables' => array(
      'pdf_form' => array(
        'type' => 'pdf_form',
        'label' => t('PDF Form Submission'),
      ),
      'pdf' => array(
        'type' => 'file',
        'label' => t('PDF File'),
      ),
    ),
  );

  $info['pdf_forms_view'] = $defaults + array(
    'label' => t('A PDF Form submission is viewed'),
    'group' => t('PDF Forms'),
    'variables' => array(
      'pdf_form' => array(
        'type' => 'pdf_form',
        'label' => t('PDF Form Submission'),
      ),
      'pdf' => array(
        'type' => 'file',
        'label' => t('PDF File'),
      ),
    ),
  );

  return $info;
}

/**
 * Implements hook_rules_action_info
 */
function pdf_forms_rules_action_info() {

  $info = array();

  $defaults = array(
    'group' => t('PDF Forms'),
  );

  $info['pdf_forms_save_submission'] = $defaults + array(
    'label' => t('Create submission data From Entity'),
    'base' => 'pdf_forms_rules_action_save_submission',
    'provides' => array(
      'file' => array(
        'type' => 'file',
        'label' => t('PDF File'),
      ),
      'submission' => array(
         'type' => 'pdf_form',
         'label' => t('The created PDF Form Submission'),
         ),
    ),
    'parameter' => array(
      'bundle' => array(
        'type' => 'text',
        'label' => t('PDF Form'),
        'description' => t("The machine name of the PDF Form template to populate"),
        'options list' => 'pdf_forms_bundle_options',
       // 'restriction' => 'input',.
      ),
      'token_objects' => array(
        'type' => 'list<entity>',
        'label' => t('Entities'),
        'description' => t('These entities will be used to retrieve token values when populating the PDF form '),
        'optional' => TRUE,
        'allow null' => TRUE,
      ),
      'versions' => array(
        'type' => 'list<integer>',
        'label' => t('Document Versions to use'),
        'description' => t('Include fields marked with these version numbers when generating the PDF'),
        'optional' => TRUE,
        'allow null' => TRUE,
      ),
    ),
  );

  return ($info);
}

/**
 * Provides options for the Action to select from.
 */
function pdf_forms_bundle_options() {

  // @TODO: provide bundle filtering based on the entity types contained in the entity list
  // $template_fid = db_query('SELECT fid FROM {pdf_forms_type} WHERE bundle = :bundle', array(':bundle' => $entity->bundle))->fetchField();
  $templates = db_query('SELECT label, bundle FROM {pdf_forms_type}');

  $bundle_options = array();

  foreach ($templates as $template) {
    $bundle_options[$template->bundle] = $template->label;
  }

  return $bundle_options;
}

/**
 * File Selector.
 */
function pdf_forms_file_options() {

}

/**
 * Implements hook_rules_condition_info
 * - Entity Type
 * - Bundle Type.
 */
function pdf_forms_rules_condition_info() {

}

/**
 * - Generates a submission for the provided entity.
 */
function pdf_forms_rules_action_save_submission($bundle, $token_objects = array(), $versions) {
  $entity = entity_get_controller('pdf_form')->create(array('bundle' => $bundle));
  global $user;
  $entity->uid = $user->uid;
  //Save the blank submission so we have a valid submission ID before generating the file
  pdf_forms_save($entity);
  module_load_include('inc', 'pdf_forms', 'includes/pdf_forms.pdftk');

  $entities = array();
  //Rules returns the list of entities using EntityDrupalWrapper, we convert them to traditional entity objects
  if (count($token_objects) > 0) {
    foreach ($token_objects as $token_entity) {
      //Uncomment this line will result in tokens being presented as [entity_type-bundle]
      //$entity_category = $token_entity->type().'-'.$token_entity->getBundle();
      $entity_category = $token_entity->type();
      $entity_array = entity_load($token_entity->type(), array($token_entity->getIdentifier()));
      foreach ($entity_array as $id => $object) {
        $entities[$entity_category] = $object;
      }
    }
  }

  if (!isset($versions)) {
    $versions = array(1);
  }

  $file = pdf_forms_pdftk_create_completed_form($entity, NULL, $entities, $template = FALSE, $versions);

  $entity->fid = $file->fid;
  $entity->pdfdata = serialize($file->pdfdata);
  pdf_forms_save($entity);

  return array('file' => $file, 'submission' => $entity);

}
