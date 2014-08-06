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
          'effect' => 'none',
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
    '#required' => TRUE,
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

  // Explicitly adding validate/submit here.
  $form['#submit'] = array('umsg_new_submit');
  $form['#validate'] = array('umsg_new_validate');
  field_attach_form('umsg_message', $message, $form, $form_state);
  
  return $form;
}

function umsg_new_validate($form, &$form_state) {
  
  // The actual message that is being sent, we create this during validation and
  // pass to submit to send out.
  $message = (object) $form_state['values'];
  $message->mid = 0;
  $message->body = isset($message->body) ? $message->body : '';
  $message->timestamp = REQUEST_TIME;

  // Only parse the user string for a new thread.
  if (!isset($message->thread_id)) {
    $recipient_obj = user_load_by_name($message->recipient);

    // Check for valid user.
    if (isset($recipient_obj->uid)) {
      $message->recipients = array($recipient_obj->uid => $recipient_obj);
    }
    else {
      $message->recipients = array();
    }
  }
  else {
    // Load participants. Limit recipients to visible unless read_all is TRUE. !!!
    $message->recipients = _privatemsg_load_thread_participants($message->thread_id, $message->read_all ? FALSE : $message->author);
  }

  $validated = _umsg_validate_message($message, TRUE);
  foreach ($validated['messages'] as $type => $texts) {
    foreach ($texts as $text) {
      drupal_set_message($text, $type);
    }
  }
  $form_state['validate_built_message'] = $message;
}

/**
 * Submit callback for the privatemsg_new form.
 */
function umsg_new_submit($form, &$form_state) {

  $message = $form_state['validate_built_message']; 
  
  //field_attach_submit('privatemsg_message', $message, $form, $form_state);
  // Format each recipient.
  // As we use une-to-one user message, get first ant unique name.
  $recipient_first = reset($message->recipients);
  $recipient = $recipient_first->name;

  try {
    $umsg = new umsgController();
    $message = $umsg->send($message);
//    $message = _umsg_send($message);
    //_privatemsg_handle_recipients($message->mid, $message->recipients); // probably we don't need it !!!
    drupal_set_message(t('A message has been sent to !recipient.', array('!recipient' => $recipient)));
    // Only redirect on new threads. !!!
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
      drupal_set_message(t('Failed to send a message to !recipients. %type: !message in %function (line %line of %file).', array('!recipients' => $recipient) + $variables), 'error');
    }
    else {
      drupal_set_message(t('Failed to send a message to !recipients. Contact your site administrator.', $recipient), 'error');
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

/**
 * Menu callback for viewing a thread.
 *
 * @param $thread
 *   The thread id
 * @return
 *   The page content.
 */
function  umsg_view($thread) {
  
  dsm('the test');
  
  
  drupal_set_title($thread['subject']);

  $content = array(
    '#thread' => $thread,
  );

  if ($thread['to'] != $thread['message_count'] || !empty($thread['start'])) {
    // Generate paging links.
    $older = '';
    if (isset($thread['older_start'])) {
      $options = array(
        'query' => array('start' => $thread['older_start']),
        'title' => t('Display older messages'),
      );
      $older = l(t('<<'), 'messages/view/' . $thread['thread_id'], $options);
    }
    $newer = '';
    if (isset($thread['newer_start'])) {
      $options = array(
        'query' => array('start' => $thread['newer_start']),
        'title' => t('Display newer messages'),
      );
      $newer = l(t('>>'), 'messages/view/' . $thread['thread_id'], $options);
    }
    $substitutions = array('@from' => $thread['from'], '@to' => $thread['to'], '@total' => $thread['message_count'], '!previous_link' => $older, '!newer_link' => $newer);
    $title = t('!previous_link Displaying messages @from - @to of @total !newer_link', $substitutions);
    $content['pager'] = array(
      '#markup'  => trim($title),
      '#prefix' => '<div class="privatemsg-view-pager">',
      '#suffix' => '</div>',
      '#weight' => 3,
    );
  }

  // Render the participants.
  $content['participants'] = array(
    '#markup' => theme('privatemsg_recipients', array('thread' => $thread)),
    '#weight' => -5
  );

  // Render the messages.
  $content['messages']['#weight'] = 0;
  $i = 1;
  $count = count($thread['messages']);
  foreach ($thread['messages'] as $pmid => $message) {
    // Set message as read and theme it.
    // Add CSS classes.
    $message->classes = array('privatemsg-message', 'privatemsg-message-' . $i, $i % 2 == 1 ? 'privatemsg-message-even' : 'privatemsg-message-odd');
    if (!empty($message->is_new)) {
      // Mark message as read.
      privatemsg_message_change_status($pmid, PRIVATEMSG_READ, $thread['user']);
      $message->classes[] = 'privatemsg-message-new';
    }

    if ($i == 1) {
      $message->classes[] = 'privatemsg-message-first';
    }
    if ($i == $count) {
      $message->classes[] = 'privatemsg-message-last';
    }
    $i++;
    $content['messages'][$pmid] = array(
      '#markup' => theme('privatemsg_view', array('message' => $message)),
    );
  }

  // Display the reply form if user is allowed to use it.
  if (privatemsg_user_access('write privatemsg') || privatemsg_user_access('reply only privatemsg')) {
    $content['reply'] = drupal_get_form('privatemsg_new', $thread['participants'], $thread['subject-original'], $thread['thread_id'], $thread['read_all']);
    $content['reply']['#weight'] = 5;
  }
  // Check after calling the privatemsg_new form so that this message is only
  // displayed when we are not sending a message.
  if ($thread['read_all']) {
    // User has permission to read all messages AND is not a participant of the current thread.
    drupal_set_message(t('This conversation is being viewed with escalated privileges and may not be the same as shown to normal users.'), 'warning');
  }

  drupal_alter('privatemsg_view', $content);

  return $content;
}