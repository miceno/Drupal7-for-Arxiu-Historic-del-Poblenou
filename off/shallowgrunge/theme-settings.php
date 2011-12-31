<?php
// $Id:

function shallowgrunge_form_system_theme_settings_alter(&$form, $form_state) {

  // Create the form widgets using Forms API
  $form['themecolor'] = array(
      '#type' => 'fieldset',
      '#title' => t('Theme Color'),
      '#description' => t('Insert hex code. Include the # sign.<br />Darker colors tend to work better for this theme.<br />These fields support the <a href="http://drupal.org/project/colorpicker" target="_blank">colorpicker module</a>.'),
      '#collapsible' => TRUE,
      '#collapsed' => FALSE,
    );
  $form['themecolor']['headcolor'] = array(
    '#type' => (module_exists('colorpicker') ? 'colorpicker_' : '') . 'textfield',
    '#title' => t('Header color'),
    '#default_value' => theme_get_setting('headcolor'),
    '#size' => 7,
    '#maxlength' => 7,    
  );
  $form['themecolor']['navcolor'] = array(
    '#type' => (module_exists('colorpicker') ? 'colorpicker_' : '') . 'textfield',
    '#title' => t('Navigation color'),
    '#default_value' => theme_get_setting('navcolor'),
    '#size' => 7,
    '#maxlength' => 7, 
  );
  $form['themecolor']['headingscolor'] = array(
      '#type' => (module_exists('colorpicker') ? 'colorpicker_' : '') . 'textfield',
      '#title' => t('Headings color'),
      '#default_value' => theme_get_setting('headingscolor'),
      '#size' => 7,
      '#maxlength' => 7, 
    );
  $form['themecolor']['linkcolor'] = array(
    '#type' => (module_exists('colorpicker') ? 'colorpicker_' : '') . 'textfield',
    '#title' => t('Link color'),
    '#default_value' => theme_get_setting('linkcolor'),
    '#size' => 7,
    '#maxlength' => 7, 
  );
  $form['grunge'] = array(
    '#type' => 'radios',
    '#title' => t('Grunge'),
    '#default_value' => theme_get_setting('grunge'),
    '#options' => array( 'grunge1' => t('Grunge 1'), 
                         'grunge2' => t('Grunge 2'), 
                         'grunge3' => t('Grunge 3'), 
                         'grunge4' => t('Grunge 4')
                        ),
  );
  
  // Return the additional form widgets
  return $form;
}
