<?php

/**
 * @file
 * Administration menu callbacks for User Message.
 */
function umsg_admin_settings() {
  $form = array();

  $form['settings'] = array(
    '#type' => 'vertical_tabs',
  );

  $form['umsg_db'] = array(
    '#type' => 'fieldset',
    '#title' => t('Database'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#weight' => -5,
    '#group' => 'settings',
  );

  $form['umsg_db']['umsg_db_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Message Database name'),
    '#default_value' => variable_get('umsg_db_name', ''),
    '#description' => t('Not sure that we need to be able change database name in admin, as this *have no reason.'),
    '#weight' => 0,
  );

  // ......... //
  
  $form['umsg_listing'] = array(
    '#type' => 'fieldset',
    '#title' => t('Message lists'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#weight' => -5,
    '#group' => 'settings',
  );

  $form['umsg_listing']['umsg_per_page'] = array(
    '#type' => 'select',
    '#title' => t('Threads per page'),
    '#default_value' => variable_get('umsg_per_page', 50),
    '#options' => drupal_map_assoc(array(10, 25, 50, 75, 100)),
    '#description' => t('Choose the number of conversations that should be listed per page.'),
  );

//  $amounts = drupal_map_assoc(array(5, 10, 20, 30, 50, 70, 90, 150, 200, 250, 300));
//  $form['umsg_listing']['umsg_view_max_amount'] = array(
//    '#type' => 'select',
//    '#title' => t('Number of messages on thread pages'),
//    '#options' => $amounts + array(UMSG_UNLIMITED => t('Unlimited')),
//    '#default_value' => variable_get('umsg_view_max_amount', 20),
//    '#description' => t('Threads will not show more than this number of messages on a single page.'),
//    '#weight' => 10,
//  );

  return system_settings_form($form);
}
