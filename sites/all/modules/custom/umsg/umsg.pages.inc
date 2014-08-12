<?php

/**
 * @file
 * User menu callbacks for Umsg.
 */
//require_once 'api/umsgController.php';

/**
 * Returns a form which handles and displays thread actions.
 *
 * @returnvalid recipient
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
    // Has rights and user_load return an array so user does exist
    $account = $account_check;
  }

  return drupal_get_form('umsg_list', $argument, $account);
}

function umsg_list($form, &$form_state, $argument, $account) {

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

  $search_string = NULL;
  if (!empty($form_state['values']['search_string'])) {
    if (strlen($form_state['values']['search_string']) > 2) {
      $search_string = check_plain($form_state['values']['search_string']);
    }
    else {
      drupal_set_message(t('You must include at least one positive keyword with 3 characters or more.'), 'error');
    }
  }


  $umsg = new umsgController();
  $query = $umsg->load($argument, array(), NULL, $search_string);

  $i = 0;
  foreach ($query->execute() as $row) {
    // Store the raw row data.
    $form['updated']['list']['#options'][$row->thread_id] = (array) $row;
    $form['updated']['list']['#options'][$row->thread_id]['#weight'] = $i++;
  }

  // Define main top controls box item.
  $form['updated']['controls'] = array(
    '#type' => 'item',
    '#prefix' => '<div id="umsg-list-main-contols">',
    '#suffix' => '</div>',
    //'#markup' => 'test',
    '#weight' => 4,
  );
  $form['updated']['controls']['top_first'] = array(
    '#type' => 'item',
//    '#prefix' => '<div id="umsg-list-main-contols">',
//    '#suffix' => '</div>',
    //'#markup' => 'test',
    '#weight' => 1,
  );

  $form['updated']['controls']['top_first']['inbox_info'] = array(
    '#type' => 'item',
    '#markup' => 'Inbox(X)',
    '#weight' => 1,
  );

  $form['updated']['controls']['top_first']['search_box'] = array(
    '#type' => 'item',
    '#weight' => 2,
  );

  $form['updated']['controls']['top_first']['search_box']['search_string'] = array(
    '#type' => 'textfield',
    '#default_value' => $search_string,
    '#size' => 60,
    '#maxlength' => 128,
    '#attributes' => array(
      'class' => array('auto_submit'),
      'placeholder' => t('Search Messages'),
    ),
    '#weight' => 1,
  );

  $form['updated']['controls']['top_first']['search_box']['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Search',
    '#ajax' => array(
      'callback' => 'umsg_list_js',
      'wrapper' => 'umsg-list-form',
      'effect' => 'none',
    ),
    '#attributes' => array(
      'id' => array('index-search-submit'),
      'class' => array('hide'),
    ),
    '#weight' => 2,
  );

  $form['updated']['controls']['top_second'] = array(
    '#type' => 'item',
    '#weight' => 2,
  );

  if (!empty($form['updated']['list']['#options'])) {
    $form['updated']['controls']['top_second']['check_all'] = array(
      '#type' => 'checkbox',
      '#title' => t('All'),
      '#weight' => 1,
    );

    $form['updated']['controls']['top_second']['actions'] = _umsg_action_form($argument);
    $form['updated']['controls']['top_second']['actions']['#weight'] = 2;
  }

  // Save the currently active account, used for actions.
  $form['account'] = array('#type' => 'value', '#value' => $account);

  //dsm($form, 'list form');
  // Define checkboxes, pager and theme
  $form['updated']['controls']['top_second']['pager'] = array('#markup' => theme('pager'), '#weight' => 2);

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

function umsg_new_validate($form, &$form_state) {

  // The actual message that is being sent, we create this during validation and
  // pass to submit to send out.
  $message = (object) $form_state['values'];
  $message->mid = 0;
  $message->body = isset($message->body) ? $message->body : '';
  $message->timestamp = REQUEST_TIME;

  // Only parse the user string for a new thread.
  // Used for new thread (deal with one recipient).
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
  // Used for existing thread reply.
  // args 0 should be recipients array, from umsg_view
  elseif (is_array($form_state['build_info']['args'][0])) {
    $message->recipients = $form_state['build_info']['args'][0];
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
 * Submit callback for the umsg_new form.
 */
function umsg_new_submit($form, &$form_state) {

  $message = $form_state['validate_built_message'];

  // Format each recipient.
  // As we use une-to-one user message, get first ant unique name.
  $recipient_first = reset($message->recipients);
  $recipient = $recipient_first->name;

  try {
    $umsg = new umsgController();
    $message = $umsg->send($message);
    drupal_set_message(t('A message has been sent to !recipient.', array('!recipient' => $recipient)));
    // Forward to the new message in the thread.
    $form_state['redirect'] = array('messages/view/' . $message->thread_id);
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
function umsg_view($thread) {
  $umsg = new umsgController();
  $thread = $umsg->threadLoad(check_plain($thread));
//  dsm($thread);
  // Get prev/next thread id's
  $umsg->setDefaultDbActive();

  $pager_ids['prev'] = $umsg->threadCheckPager($thread, 'prev');
  $pager_ids['next'] = $umsg->threadCheckPager($thread, 'next');
  $umsg->setDefaultDbActive();

  $thread_pager = _umsg_build_thread_pager($pager_ids);

  drupal_set_title($thread['subject']);

  $content = array(
    '#thread' => $thread,
  );

  // Render the messages.
  $content['messages']['#weight'] = 0;
  $content['messages']['#prefix'] = '<div id="umsg-page-content">';
  $content['messages']['#suffix'] = '</div>';

  $i = 1;
  $count = count($thread['messages']);
  $write_umsg = umsg_user_access('write umsg');

  // User is not owner, or smth. else.
  if (empty($thread['messages'])) {
    drupal_not_found();
  }

  foreach ($thread['messages'] as $mid => $message) {

    if ($message->author) {
      $message->author = user_load($message->author);
    }

    // Recipient
    // Set message as read and theme it.
    // Add CSS classes.
    $message->classes = array('umsg-message', 'umsg-message-' . $i, $i % 2 == 1 ? 'umsg-message-even' : 'umsg-message-odd');

    if (!empty($message->is_new)) {
      // Mark message as read.
      $umsg = new umsgController();
      $umsg->changeMsgReadStatus($message->mid, UMSG_READ, $thread['user']);
      $message->classes[] = 'umsg-message-new';
    }

    if ($i == 1) {
      $message->classes[] = 'umsg-message-first';
    }
    if ($i == $count) {
      $message->classes[] = 'umsg-message-last';
    }
    $i++;
    $content['messages'][$mid] = array(
      '#markup' => theme('umsg_view', array('message' => $message)),
    );
  }

  $navigation = module_invoke('umsg', 'block_view', 'umsg_msg_navigation');
  // Attach main navigation links.
  $content['navigation'] = array(
    '#type' => 'item',
    '#prefix' => '<div id="umsg-list-main-navigation">',
    '#suffix' => '</div>',
    '#markup' => !empty($navigation['content']['content']) ? $navigation['content']['content'] : '',
    '#weight' => -100,
  );

  $content['top-actions'] = array(
    '#type' => 'item',
    '#prefix' => '<div id="umsg-view-top-actions">',
    '#suffix' => '</div>',
    '#weight' => -90,
  );

  $content['top-actions']['thread-subject'] = array(
    '#type' => 'item',
    '#prefix' => '<div id="umsg-view-top-subject">',
    '#suffix' => '</div>',
    '#markup' => $thread['subject'],
    '#weight' => 0,
  );

  $content['top-actions']['self_actions'] = array(
    '#type' => 'item',
    '#weight' => -80,
  );

  if (umsg_user_access('delete umsg')) {

    $options = array(
      // We need it for cancel, but if user press delete!
      //'query' => drupal_get_destination(),
      'html' => TRUE,
      'attributes' => array(
        'class' => array('delete-thread-link'),
      ),
    );

    $content['top-actions']['self_actions']['thread-delete'] = array(
      '#type' => 'item',
      '#prefix' => '<div id="umsg-view-top-delete">',
      '#suffix' => '</div>',
      '#markup' => l('Delete', 'messages/delete/' . $thread['thread_id'], $options),
      '#weight' => 1,
    );
  }

  $content['top-actions']['self_actions']['thread-pager'] = array(
    '#type' => 'item',
    '#prefix' => '<div id="umsg-view-top-pager">',
    '#suffix' => '</div>',
    '#markup' => $thread_pager,
    '#weight' => 1,
  );

  $participants_id = array();
  foreach ($thread['participants'] as $participant) {
    // Do we need to worry about recipient_name ?
    $participants_id[$participant->recipient] = $participant->recipient;
  }

  $thread['participants'] = user_load_multiple(array_unique($participants_id));

  // Display the reply form if user is allowed to use it.
  if ($write_umsg) {
    $content['reply'] = drupal_get_form('umsg_new', $thread['participants'], $thread['subject-original'], $thread['thread_id'], FALSE);
    $content['reply']['#prefix'] = '<div class="hide" id="umsg-view-base-reply-form">';
    $content['reply']['#suffix'] = '</div>';
  }

  return $content;
}

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

    if ($recipient->recipient) {
      if ($recipient->recipient == $user->uid) {
        $usercount++;
        // Skip putting author in the recipients list for now.
        continue;
      }
      $to[$recipient->uid] = umsg_recipient_format($recipient);
      $to_plain[$recipient->uid] = umsg_recipient_format($recipient, array('plain' => TRUE, 'unique' => $unique));
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

  $form = array(
    '#access' => (umsg_user_access('write umsg')),
  );

  $form['umsg-page-content']['#prefix'] = '<div id="umsg-page-content">';
  $form['umsg-page-content']['#suffix'] = '</div>';

  $form['umsg-page-content']['author'] = array(
    '#type' => 'value',
    '#value' => $user,
  );
  if (is_null($thread_id)) {

    $form['umsg-page-content']['recipient'] = array(
      '#type' => 'textfield',
      '#title' => t('Recipient name'),
      '#default_value' => $recipients_plain,
      '#required' => TRUE,
      '#weight' => -10,
      '#size' => 50,
      '#autocomplete_path' => 'messages/autocomplete',
    );

    $navigation = module_invoke('umsg', 'block_view', 'umsg_msg_navigation');
    // Attach main navigation links.
    $form['navigation'] = array(
      '#type' => 'item',
      '#prefix' => '<div id="umsg-list-main-navigation">',
      '#suffix' => '</div>',
      '#markup' => !empty($navigation['content']['content']) ? $navigation['content']['content'] : '',
      '#weight' => -100,
    );
  }

  $form['umsg-page-content']['body'] = array(
    '#type' => 'textarea',
    '#title' => t('Message'),
    '#rows' => 6,
    '#weight' => -3,
    '#default_value' => $body,
    '#resizable' => TRUE,
    '#required' => TRUE,
  );

  $form['umsg-page-content']['actions'] = array('#type' => 'actions');

  $form['umsg-page-content']['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Send'),
    '#weight' => 49,
  );
  $form['umsg-page-content']['actions']['cancel'] = array(
    '#markup' => l('X', '#', array(
      'html' => TRUE,
      'attributes' => array(
        'class' => array('cancel-umsg-thread-reply-form'),
      ),
            )
    ),
    '#weight' => 50,
  );

  if (!is_null($thread_id)) {
    $form['umsg-page-content']['thread_id'] = array(
      '#type' => 'value',
      '#value' => $thread_id,
    );

    if (empty($to)) {
      // If there are no valid recipient, hide all visible parts of the form.
      foreach (element_children($form) as $element) {
        $form[$element]['#access'] = FALSE;
      }
    }
  }

  // Explicitly adding validate/submit here.
  $form['umsg-page-content']['#submit'] = array('umsg_new_submit');
  $form['umsg-page-content']['#validate'] = array('umsg_new_validate');

  return $form;
}
