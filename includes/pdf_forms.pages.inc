<?php

/**
 * - Generates the settings form for the module.
 *
 * @param $form - form element passed to generate the form.
 * @param $form_state - passes the state values for the form.
 *
 * @return $form - returns the built form array for generating our admin form.
 */
function pdf_forms_admin_settings($form, &$form_state) {
  $form['pdf_forms_pdftk_path'] = array(
    '#type' => 'textfield',
    '#title' => t('PDFTK Path'),
    '#description' => t('Enter the path to the PDFTK Executable.'),
    '#default_value' => variable_get('pdf_forms_pdftk_path', 'pdftk'),
  );

  $form['pdf_forms_templates_path'] = array(
    '#type' => 'textfield',
    '#title' => t('PDF Template Path'),
    '#description' => t('Enter the path to store the PDF Form Templates.'),
    '#default_value' => variable_get('pdf_forms_templates_path', 'public://pdf_form_templates'),
  );

    $form['pdf_forms_submissions_path'] = array(
    '#type' => 'textfield',
    '#title' => t('PDF Submission Path'),
    '#description' => t('Enter the path to store the PDF Form Submissions.'),
    '#default_value' => variable_get('pdf_forms_submissions_path', 'public://pdf_form_submissions'),
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save Settings'),
  );

  return $form;
}

/**
 * Form validation.
 */
function pdf_forms_admin_settings_validate($form, &$form_state) {

  // Check if the path is correct.
  module_load_include('inc', 'pdf_forms', 'includes/pdf_forms.pdftk');
  if (!pdf_forms_pdftk_check($form_state['values']['pdf_forms_pdftk_path'])) {
    form_set_error('pdf_forms_pdftk_path', t('Invalid path for PDFTK executable, or invalid permissions.'));
  }

  if (!file_prepare_directory($form_state['values']['pdf_forms_templates_path'], FILE_CREATE_DIRECTORY)) {
    form_set_error('pdf_forms_templates_path', t('Could not create directory for saving PDF Form Templates.'));
  }


  if (!file_prepare_directory($form_state['values']['pdf_forms_submissions_path'], FILE_CREATE_DIRECTORY)) {
    form_set_error('pdf_forms_submissions_path', t('Could not create directory for saving PDF Form Submissions.'));
  }

}

/**
 * Form submit handler.
 */
function pdf_forms_admin_settings_submit($form, &$form_state) {
  variable_set('pdf_forms_pdftk_path', $form_state['values']['pdf_forms_pdftk_path']);
  file_prepare_directory($form_state['values']['pdf_forms_templates_path']);
  variable_set('pdf_forms_templates_path', $form_state['values']['pdf_forms_templates_path']);
  file_prepare_directory($form_state['values']['pdf_forms_submission_path']);
  variable_set('pdf_forms_submissions_path', $form_state['values']['pdf_forms_submissions_path']);
}

/**
 * Overview display for the Admin page (Pdf Forms)
 */
function pdf_forms_type_admin_overview() {
  $header = array(
    array(
      'field' => 'label',
      'data' => t('Title'),
    ),
    array(
      'field' => 'description',
      'data' => t('Description'),
    ),
    array(
      'field' => 'status',
      'data' => t('Status'),
    ),
    array(
      'data' => t('Total Submissions'),
    ),
    array(
      'data' => t('Actions'),
    ),
  );
  $query = db_select('pdf_forms_type', 't');
  $query->fields('t', array('bundle', 'label', 'description', 'status'));
  $query->extend('PagerDefault')->limit(20);

  $rows = array();
  $results = $query->execute();
  foreach ($results as $row) {

    $submission_total = db_query('SELECT COUNT(*) FROM {pdf_forms_data} WHERE bundle = :bundle', array(':bundle' => $row->bundle))->fetchField();
    $action_links = l('edit form', "admin/content/pdf_forms/{$row->bundle}/edit")
      . " | " . l('view submissions', "admin/content/form_submissions/{$row->bundle}")
      . " | " . l('submission link', "pdf_form/{$row->bundle}");
    $rows[] = array(
      $row->label,
      $row->description,
      $row->status == 0 ? t('unpublished') : t('published'),
      $submission_total,
      $action_links,
    );
  }

  return theme('table', array('header' => $header, 'rows' => $rows, 'empty' => t('No form types created yet.'))) . theme('pager');
}

/**
 * Overview of submissions for forms.
 */
function pdf_forms_submissions_overview() {
  $header = array(
    array(
      'field' => 't.label',
      'data' => t('Form'),
    ),
    array(
      'field' => 'u.name',
      'data' => t('Author'),
    ),
    array(
      'field' => 's.created',
      'data' => t('Created'),
    ),
    array(
      'field' => 's.submission_id',
      'data' => t('Edit Form'),
    ),
    array(
      'field' => 's.fid',
      'data' => t('View PDF'),
    ),
  );

  $query = db_select('pdf_forms_data', 's');
  $query->fields('s', array('submission_id', 'uid', 'created', 'fid'))
    ->join('users', 'u', 'u.uid = s.uid');
  $query->fields('u', array('name'))
    ->join('pdf_forms_type', 't', 't.bundle = s.bundle');
  $query->fields('f', array('uri'))
    ->join('file_managed', 'f', 'f.fid = s.fid');
  $query->fields('t', array('label'));
	if($bundle = arg(3)) $query->condition('t.label', $bundle);
  $query->extend('TableSort')->orderByHeader($header);
  $query->extend('PagerDefault')->limit(20);
  $results = $query->execute();
  $rows = array();

  foreach ($results as $row) {
    $rows[] = array(
      $row->label,
      $row->name,
      format_date($row->created),
      l('edit submission', "pdf_form/submissions/{$row->submission_id}/edit"),
      l('view submission', file_create_url($row->uri)),
    );
  }

  return theme('table', array('header' => $header, 'rows' => $rows, 'empty' => t('No submissions in database.'))) . theme('pager');
}

/**
 *
 */
function pdf_forms_type_submission_edit($form, &$form_state, $sid) {

    $submission = pdf_forms_load($sid);

    $map = array();
    $results = db_query('SELECT machine_name, type, pdf_page, fieldinfo FROM {pdf_forms_field_mapping} WHERE bundle = :bundle ORDER BY weight ASC', array(':bundle' => $submission->bundle));
    foreach ($results as $field_map) {
      $map[$field_map->pdf_page][$field_map->machine_name]['type'] = $field_map->type;
      $map[$field_map->pdf_page][$field_map->machine_name]['info'] = unserialize($field_map->fieldinfo);
    }

    $pdfdata = unserialize($submission->pdfdata);

    // $file = file_load($submission->fid);.
    module_load_include('inc', 'pdf_forms', 'includes/pdf_forms.pdftk');
    if (count($map)) {
			foreach($map as $fieldset => $field){
				$form['fieldset'.$fieldset] = array(
					'#type' => 'fieldset',
					'#title' => t($fieldset),
					'#collapsible' => TRUE,
					'#collapsed' => isset($collapsed) ? TRUE : FALSE,
				);
				foreach ($field as $fname => $fvalue) {
					$formfield = array();
					if ($fvalue['type'] == "Button") {
						$formfield['#type'] = "checkbox";
					}
					elseif ($fvalue['type'] == "Radio") {
						$formfield['#type'] = "radios";
						$options = $fvalue['info']['options'];
						if (!in_array($pdfdata[$fname], $options)) {
							$pdfdata[$fname] = in_array("Off", $options) ? "Off" : NULL;
						}
						$vars = array();
						foreach ($options as $option) {
							$vars[$option] = $option;
						}
						$formfield['#options'] = $vars;

					}
					elseif ($fvalue['type'] == "Choice") {
						$formfield['#type'] = "select";
						$options = $fvalue['info']['options'];
						if (!in_array($pdfdata[$fname], $options)) {
							$pdfdata[$fname] = in_array("Off", $options) ? "Off" : NULL;
						}
						$vars = array();
						foreach ($options as $option) {
							$vars[$option] = $option;
						}
						$formfield['#options'] = $vars;

					}
					else {
						$formfield['#type'] = "textfield";
						$formfield['#maxlength'] = "256";
					}
					// Check required field
					if(!empty($fvalue['info']['required'])){
						$formfield['#required'] = TRUE;
					}
					// Check readonly field
					if(!empty($fvalue['info']['readonly'])){
						$formfield['#disabled'] = $fvalue['info']['readonly'];
						$formfield['#type'] = "hidden";
						$hiddenfield = TRUE;
					}
					// Add field description
					if(isset($fvalue['info']['tooltip'])){
						$formfield['#description'] = $fvalue['info']['tooltip'];
					}
					$formfield['#title'] = ucfirst(str_replace('^', ' ', $fname));
					switch($formfield['#type']){
						case 'checkbox':
							$formfield['#default_value'] = (bool)$pdfdata[$fname];
							break;
						default: 
							$formfield['#default_value'] = $pdfdata[$fname];
					}
					

					if(isset($hiddenfield)){
						$form['fieldset'.$fieldset]['hiddenfields'][$fname] = $formfield;
						unset($hiddenfield);
					}
					else{
						$form['fieldset'.$fieldset]['fields'][$fname] = $formfield;
					}
				}
				if(!isset($form['fieldset'.$fieldset]['fields'])){
					hide($form['fieldset'.$fieldset]);
				}
				$collapsed = TRUE;
			}
    }
    else {
      $form['nofields'] = array(
        '#markup' => '<p>No fields found for this submission</p>',
      );
    }
    $form['submit_button'] = array(
      '#type' => 'submit',
      '#value' => t('Save'),
    );
    $form['cancel_button'] = array(
      '#type' => 'submit',
      '#value' => t('Cancel'),
			'#attributes' => array('onclick' => 'if(!confirm("Are you sure you want to discard all changes?")){return false;}'),
			'#limit_validation_errors' => array(),
			'#submit' => array(),
    );
    $form['delete_button'] = array(
      '#type' => 'submit',
      '#value' => t('Delete'),
			'#attributes' => array('onclick' => 'if(!confirm("Are you sure you want to delete submission?")){return false;}'),
    );

    return $form;

}

/**
 *
 */
function pdf_forms_type_submission_edit_submit($form, &$form_state) {

  if ($form_state['clicked_button']['#value'] == 'Save') {
    unset($form_state['values']['form_id'], $form_state['values']['form_token'], $form_state['values']['form_build_id'], $form_state['values']['op'], $form_state['values']['submit_button'], $form_state['values']['delete_button'], $form_state['values']['cancel_button']);

    $submission = pdf_forms_load($form_state['build_info']['args'][0]);
    $file = pdf_forms_pdftk_create_completed_form($submission, $form_state['values']);
    $submission->fid = $file->fid;
		$values = serialize($form_state['values']);
		$submission->pdfdata = $values;		
		$submission->is_new_revision = TRUE;
    pdf_forms_save($submission);
		drupal_set_message('Changes saved');
  }
  elseif ($form_state['clicked_button']['#value'] == 'Delete') {
    $submission = pdf_forms_load($form_state['build_info']['args'][0]);
    entity_get_controller('pdf_form')->delete($submission);
    drupal_goto('admin/content/form_submissions');
  }
}

/**
 *
 */
function pdf_forms_type_delete_form($form, &$form_state, $type) {
  $form = confirm_form($form,
        'Are you sure you want to delete PDF form and all submissions?',
        'admin/content/pdf_forms/' . $type,
        'The action cannot be undone.',
        'Delete',
        'Cancel'
  );
  return $form;
}

/**
 *
 */
function pdf_forms_type_delete_form_submit($form, &$form_state) {
  $type = $form_state['build_info']['args'][0];
  pdf_forms_type_delete($type);
}
