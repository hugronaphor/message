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

  $recipients_plain = '';
  $body = '';

  // convert recipients to array of user objects
  $unique = FALSE;
  if (!empty($recipients) && is_string($recipients) || is_int($recipients)) {
    $unique = TRUE;
    $recipients = _umsg_generate_user_array($recipients);
  }
  elseif (is_object($recipients)) {
    $recipients = array($recipients);
  }
  elseif (empty($recipients) && is_string($recipients)) {
    $recipients = array();
  }
  
  dsm($recipients);

  $usercount = 0;
  $to = array();
  $to_plain = array();
  $blocked_messages = array();
  foreach ($recipients as $recipient) {
    $recipient->recipient = $recipient->uid;


    if (isset($to[$recipient->uid])) {
      // We already added the recipient to the list, skip him.
      continue;
    }

    // Check if another module is blocking the sending of messages to the recipient by current user.
    //$user_blocked = module_invoke_all('privatemsg_block_message', $user, array($recipient->uid => $recipient), array('thread_id' => $thread_id));
    if (!count($user_blocked) <> 0 && $recipient->recipient) {
      if ($recipient->type == 'user' && $recipient->recipient == $user->uid) {
        $usercount++;
        // Skip putting author in the recipients list for now.
        continue;
      }
      $to[$recipient->uid] = umsg_recipient_format($recipient);
      $to_plain[$recipient->uid] = umsg_recipient_format($recipient, array('plain' => TRUE, 'unique' => $unique));
    }
    else {
      // Store blocked messages. These are only displayed if all recipients
      // are blocked.
      $first_reason = reset($user_blocked);
      $blocked_messages[] = $first_reason['message'];
    }
  }

  if (empty($to) && $usercount >= 1 && empty($blocked_messages)) {
    // Assume the user sent message to own account as if the usercount is one or less, then the user sent a message but not to self.
    $to['user_' . $user->uid] = umsg_recipient_format($user);
    $to_plain['user_' . $user->uid] = umsg_recipient_format($user, array('plain' => TRUE));
  }

  // Subject has / encoded twice if clean urls are enabled to get it through
  // mod_rewrite and the menu system. Decode it once more.
  //$subject = str_replace('%2F', '/', $subject);

  if (!empty($to)) {
    $recipients_plain = implode(', ', $to_plain);
  }
  if (isset($form_state['values'])) {
    if (isset($form_state['values']['recipient'])) {
      $recipients_plain = $form_state['values']['recipient'];
    }
    if (isset($form_state['values']['body'])) {
      $body = $form_state['values']['body'];
    }
  }
  if (!$thread_id && !empty($recipients_plain)) {
    drupal_set_title(t('Write new message to @recipient', array('@recipient' => $recipients_plain)));
  }
  elseif (!$thread_id) {
    drupal_set_title(t('Write new message'));
  }

  $form = array(
    '#access' => (umsg_user_access('write umsg')),
  );

  $form['author'] = array(
    '#type' => 'value',
    '#value' => $user,
  );
  if (is_null($thread_id)) {

    $form['recipient'] = array(
      '#type' => 'textfield',
      '#title' => t('Recipient name'),
      '#default_value' => $recipients_plain,
      '#required' => TRUE,
      '#weight' => -10,
      '#size' => 50,
      '#autocomplete_path' => 'messages/autocomplete',
    );
  }

  $form['body'] = array(
    '#type' => 'textarea',
    '#title' => t('Message'),
    '#rows' => 6,
    '#weight' => -3,
    '#default_value' => $body,
    '#resizable' => TRUE,
  );

  $form['actions'] = array('#type' => 'actions');

  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Send'),
    '#weight' => 49,
  );

  if (!is_null($thread_id)) {
    $form['thread_id'] = array(
      '#type' => 'value',
      '#value' => $thread_id,
    );

    $form['reply'] = array(
      '#markup' => '<h2 class="umsg-reply">' . t('Reply') . '</h2>',
      '#weight' => -10,
    );
    if (empty($to)) {
      // If there are no valid recipient, hide all visible parts of the form.
      foreach (element_children($form) as $element) {
        $form[$element]['#access'] = FALSE;
      }
    }
  }
  // Only set read all if it is a boolean TRUE. It might also be an integer set
  // through the URL.
  $form['read_all'] = array(
    '#type' => 'value',
    '#value' => $read_all === TRUE,
  );
  // Attach field widgets.
  $message = (object) array();
  if (isset($form_state['validate_built_message'])) {
    $message = $form_state['validate_built_message'];
  }

  // If a module (e.g. OG) adds a validate or submit callback to the form in
  // field_attach_form, the form system will not add ours automatically
  // anymore. Therefore, explicitly adding them here.
  $form['#submit'] = array('umsg_new_submit');
  // $form['#validate'] = array('umsg_new_validate');  // !!!

  field_attach_form('umsg_message', $message, $form, $form_state);
  return $form;
}

/**
 * Submit callback for the privatemsg_new form.
 */
function umsg_new_submit($form, &$form_state) {
  
  
  dsm($form_state);
  
  return;
  
  $message = $form_state['validate_built_message'];
  field_attach_submit('privatemsg_message', $message, $form, $form_state);

  // Format each recipient.
  $recipient_names = array();
  foreach ($message->recipients as $recipient) {
    $recipient_names[] = privatemsg_recipient_format($recipient);
  }
  try {
    $message = _privatemsg_send($message);
    _privatemsg_handle_recipients($message->mid, $message->recipients);
    drupal_set_message(t('A message has been sent to !recipients.', array('!recipients' => implode(', ', $recipient_names))));
    // Only redirect on new threads.
    if ($message->mid == $message->thread_id || variable_get('privatemsg_default_redirect_reply', FALSE)) {
      $redirect = variable_get('privatemsg_default_redirect', '<new-message>');
      if ($redirect == '<new-message>' || (!empty($_REQUEST['destination']) && $_REQUEST['destination'] == '[new-message]')) {
        if (!empty($_REQUEST['destination']) && $_REQUEST['destination'] == '[new-message]') {
          // Remove GET param so that drupal_goto() uses the redirect from
          // $form_state.
          unset($_GET['destination']);
        }
        // Forward to the new message in the thread.
        $form_state['redirect'] = array('messages/view/' . $message->thread_id, array('fragment' => 'privatemsg-mid-' . $message->mid));
      }
      elseif (!empty($redirect)) {
        $form_state['redirect'] = $redirect;
      }
    }
  }
  catch (Exception $e) {
    if (error_displayable()) {
      require_once DRUPAL_ROOT . '/includes/errors.inc';
      $variables = _drupal_decode_exception($e);
      drupal_set_message(t('Failed to send a message to !recipients. %type: !message in %function (line %line of %file).', array('!recipients' => implode(', ', $recipient_names)) + $variables), 'error');
    }
    else {
      drupal_set_message(t('Failed to send a message to !recipients. Contact your site administrator.', array('!recipients' => implode(', ', $recipient_names))), 'error');
    }
  }
}

/**
 * Return autocomplete results for usernames.
 */
function umsg_autocomplete($string) {
  // Parse $string and build list of valid user names.
  $matches = array();
  $return = umsg_user_autocomplete($string);

  if (is_array($return) && !empty($return)) {
    $matches = $return;
  }

  // Format the suggestions.
  $themed_matches = array();
  foreach ($matches as $key => $match) {
    $themed_matches[$key] = umsg_recipient_format($match, array('plain' => TRUE));
  }

  $suggestions = array();
  foreach ($themed_matches as $match) {
    $suggestions[$match] = $match;
  }

  // convert to object to prevent drupal bug, see http://drupal.org/node/175361
  drupal_json_output((object) $suggestions);
}