<?php

/**
 * @file
 * User menu callbacks for Umsg.
 */
//require_once 'api/umsgController.php';

/**
 * Returns a form which handles and displays thread actions.
 *
 * @return
 *   The FAPI definitions for the thread action form.
 */
function _umsg_action_form($type) {

  $form = array(
    '#prefix' => '<div class="container-inline">',
    '#suffix' => '</div>',
    '#weight' => -5,
  );
  // Display all operations.
  $operations = umsg_thread_operations($type);

  foreach ($operations as $operation => $array) {
    if (!empty($array['button'])) {
      $form[$operation] = array(
        '#type' => 'submit',
        '#value' => $array['label'],
        '#ajax' => array(
          'callback' => 'umsg_list_js',
          'wrapper' => 'umsg-list-form',
          'effect' => 'fade',
        ),
      );
    }
  }

  return $form;
}

function umsg_fake_function($threads) {
  // Nothing to do.
}

/**
 * AJAX callback to return the form again.
 */
function umsg_list_js($form, $form_state) {
  return $form['updated'];
}

/**
 * Delete specific message of thread
 * 
 * @param type $form
 * @param type $form_state
 * @param type $thread
 * @param type $message
 * @return type
 */
function umsg_delete($form, $form_state, $thread, $message) {
  
}

function umsg_delete_submit($form, &$form_state) {
  
}

/**
 * List messages.
 *
 * @param $argument
 *   An argument to pass through to the query builder.
 * @param $uid
 *   User id messages of another user should be displayed
 *
 * @return
 *   Form array
 */
function umsg_list_page($argument = 'list', $uid = NULL) {
  global $user;

  // Setting default behavior...
  $account = $user;
  // Because uid is submitted by the menu system, it's a string not a integer.
  if ((int) $uid > 0 && $uid != $user->uid) {
    // Trying to view someone else's messages...
    if (!$account_check = user_load($uid)) {
      return MENU_NOT_FOUND;
    }
    if (!umsg_user_access('read all user messages')) {
      return MENU_ACCESS_DENIED;
    }
    // Has rights and user_load return an array so user does exist
    $account = $account_check;
  }

  return drupal_get_form('umsg_list', $argument, $account);
}

function umsg_list($form, &$form_state, $argument, $account) {
  drupal_add_css(drupal_get_path('module', 'umsg') . '/styles/umsg-list.css');
  drupal_add_js(drupal_get_path('module', 'umsg') . '/js/umsg.js');

  // If this is an AJAX request, update $_GET['q'] so that table sorting and
  // similar links are using the correct base path.
  if ($_GET['q'] == 'system/ajax') {
    $q = 'messages';
    if (!empty($argument)) {
      $q .= '/' . $argument;
    }
    $_GET['q'] = $q;
  }

  // Load the table columns.
  $columns = array('participants', 'subject', 'last_updated');
  // Load the themed list headers based on the available data.
  $headers = _umsg_list_headers($columns);

  $form = array(
    '#list_argument' => $argument,
    '#submit' => array('umsg_list_submit'),
    'updated' => array(
      '#prefix' => '<div id="umsg-list-form">',
      '#suffix' => '</div>',
    ),
  );

  $navigation = module_invoke('umsg', 'block_view', 'umsg_msg_navigation');
  // Attach main navigation links.
  $form['updated']['navigation'] = array(
    '#type' => 'item',
    '#prefix' => '<div id="umsg-list-main-navigation">',
    '#suffix' => '</div>',
    '#markup' => !empty($navigation['content']['content']) ? $navigation['content']['content'] : '',
    '#weight' => 3,
  );

//  $form['updated']['controls'] = array(
//    '#type' => 'item',
//    '#prefix' => '<div id="umsg-list-main-contols">',
//    '#suffix' => '</div>',
//    '#markup' => 'test',
//    '#weight' => 4,
//  );

  $form['updated']['list'] = array(
    '#type' => 'tableselect',
    '#header' => $headers,
    // to replace with simple array, no need for theme_ ad the header will be hidden.
    //'#header' => array('participants'),
    '#options' => array(),
    '#attributes' => array('class' => array('umsg-list')),
    '#empty' => t('No messages available.'),
    '#weight' => 5,
    '#pre_render' => array('_umsg_list_thread'),
  );

  $umsg = new umsgController();
  $query = $umsg->load($argument);

  $i = 0;
  foreach ($query->execute() as $row) {
    // Store the raw row data.
    $form['updated']['list']['#options'][$row->thread_id] = (array) $row;
    $form['updated']['list']['#options'][$row->thread_id]['#weight'] = $i++;
  }

  if (!empty($form['updated']['list']['#options'])) {
    $form['updated']['actions'] = _umsg_action_form($argument);

    $form['updated']['controls']['check_all'] = array(
      '#type' => 'checkbox',
      '#title' => t('All'),
    );
  }

  // Save the currently active account, used for actions.
  $form['account'] = array('#type' => 'value', '#value' => $account);

  //dsm($form, 'list form');
  // Define checkboxes, pager and theme
  // FOR SOME REASON IT TRY TO QUERY IN MSG DATABASE
  // $form['updated']['pager'] = array('#markup' => theme('pager'), '#weight' => 20);

  return $form;
}

/**
 * Process umsg_list form submissions.
 *
 * Execute the chosen action on the selected messages. This function is
 * based on node_admin_nodes_submit().
 */
function umsg_list_submit($form, &$form_state) {

  // Load all available operation definitions.
  $operations = umsg_thread_operations($form['#list_argument']);

  // Default "default" operation, which won't do anything.
  $operation = array('callback' => 0);

  // Check if a valid operation has been submitted.
  if (isset($form_state['values']['operation']) && isset($operations[$form_state['values']['operation']])) {
    $operation = $operations[$form_state['values']['operation']];
  }

  if (!empty($form_state['values']['op'])) {
    // Load all keys where the value is the current op.
    $keys = array_keys($form_state['values'], $form_state['values']['op']);

    // Loop over them and detect if a matching button was pressed.
    foreach ($keys as $key) {
      if ($key != 'op' && isset($operations[$key])) {
        $operation = $operations[$key];
      }
    }
  }

  // Only execute something if we have a valid callback and at least one checked thread.
  if (!empty($operation['callback'])) {
    // Hack to fix destination during ajax requests.
    if (isset($form_state['input']['ajax_page_state'])) {
      $destination = 'messages';
      if (!empty($form['#list_argument'])) {
        $destination .= '/' . $form['#list_argument'];
      }
      $_GET['destination'] = $destination;
    }

    // Update all form.
    if ($operation['callback'] !== UMSG_FAKE_CALLBACK) {
      umsg_operation_execute($operation, $form_state['values']['list'], $form_state['values']['account']);
    }
  }

  $form_state['rebuild'] = TRUE;
  $form_state['input'] = array();
}

// --------------------------------------------- //

function umsg_new($form, &$form_state, $recipients = array(), $subject = '', $thread_id = NULL, $read_all = FALSE) {
  global $user;
}
