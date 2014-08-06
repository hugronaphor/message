<?php

/**
 * @file
 * Theme functions for umsg.
 */

/**
 * @addtogroup theming
 * 
 */

/**
 * Default theme function for field theme.
 *
 * To hide all fields that don't have an explicit theme pattern defined, this
 * theme doesn't return anything.
 *
 * @param $thread
 *   Thread row returned by the list query.
 *
 * @return
 *   A theme_table() compatible field definition.
 */
function theme_umsg_list_field($thread) {
  
}

/**
 * Theme the participants field.
 *
 * @see theme_umsg_list_field()
 */
function theme_umsg_list_field__participants($variables) {
  $thread = $variables['thread'];

  $field = $user_name = array();

  // Parse sql concatinated data.
  foreach (explode(',', $thread['participants']) as $participant_data) {
    $participant = explode('_', $participant_data);
    // This can cause performance issue,
    // need to decite if we really need to check for deleted users.
    if ($user = user_load($participant[0])) {
      $user_name[] = $user->name;
    }
    else {
      $user_name[] = $participant[1];
    }
  }

  $field['data'] = implode(', ', $user_name);
  $field['class'][] = 'umsg-list-participants';
  if (!empty($thread['is_new'])) {
    $field['class'][] = 'umsg-item-new';
  }
  return $field;
}

/**
 * Theme the subject of the thread.
 *
 * @see theme_umsg_list_field()
 */
function theme_umsg_list_field__subject($variables) {
  $thread = $variables['thread'];
  $field = array();
  $options = array();
  $is_new = '';
  if (!empty($thread['is_new'])) {
    //$is_new = theme('mark', array('type' => MARK_NEW));
    $is_new = '';
    $options['fragment'] = 'new';
    $field['class'][] = 'umsg-item-new';
  }

  $subject = $thread['subject'];
  $field['data'] = l($subject, 'messages/view/' . $thread['thread_id'], $options) . $is_new;
  $field['class'][] = 'umsg-list-subject';
  return $field;
}

/**
 * Theme the replies field.
 *
 * @see theme_umsg_list_field()
 */
function theme_umsg_list_field__count($variables) {
  $thread = $variables['thread'];
  $field = array();
  $field['data'] = $thread['count'];
  $options = array();
  if (!empty($thread['is_new']) && $thread['is_new'] < $thread['count']) {
    $options['fragment'] = 'new';
    $field['data'] .= '<br />' . l((format_plural($thread['is_new'], '(1 new)', '(@count new)')), 'messages/view/' . $thread['thread_id'], $options);
  }
  $field['class'][] = 'umsg-list-count';
  return $field;
}

/**
 * Theme the last updated column.
 *
 * @see theme_umsg_list_field()
 */
function theme_umsg_list_field__last_updated($variables) {
  $thread = $variables['thread'];
  $field['data'] = umsg_format_date($thread['last_updated']);
  $field['class'][] = 'umsg-list-date';
  if (!empty($thread['is_new'])) {
    $field['class'][] = 'umsg-item-new';
  }
  return $field;
}

/* * HE
 * Theme the thread started column.
 *
 * @see theme_umsg_list_field()
 */

function theme_umsg_list_field__thread_started($variables) {
  $thread = $variables['thread'];
  $field = array();
  $field['data'] = umsg_format_date($thread['thread_started']);
  $field['class'][] = 'umsg-list-date-started';
  return $field;
}

/**
 * Define the table header for a specific column.
 *
 * This default theme function is used to ignore columns that should not be
 * displayed. Only columns with a specific theme pattern function are displayed.
 *
 * @return
 *   A theme_table() compatible table header definition. Additionally, the key
 *   "key" should be used to specify which row column should be displayed in
 *   this column.
 */
function theme_umsg_list_header() {
  
}

/**
 * Define the subject header.
 *
 * @see theme_umsg_list_header()
 */
function theme_umsg_list_header__subject() {
  return array(
    'data' => t('Subject'),
    'field' => 'subject',
    'class' => array('umsg-header-subject'),
    '#weight' => -40,
  );
}

/**
 * Define the answers column.
 *
 * @see theme_umsg_list_header()
 */
function theme_umsg_list_header__count() {
  return array(
    'data' => t('Messages'),
    'class' => array('umsg-header-count'),
    '#weight' => -25,
  );
}

/**
 * Define the participants column.
 *
 * @see theme_umsg_list_header()
 */
function theme_umsg_list_header__participants() {
  return array(
    'data' => t('Participants'),
    'class' => array('umsg-header-participants'),
    '#weight' => -30,
  );
}

/**
 * Define the last updated column.
 *
 * @see theme_umsg_list_header()
 */
function theme_umsg_list_header__last_updated() {
  return array(
    'data' => t('Last Updated'),
    'field' => 'last_updated',
    'sort' => 'desc',
    'class' => array('umsg-header-lastupdated'),
    '#weight' => -20,
  );
}

/**
 * Define the thread started column.
 *
 * @see theme_umsg_list_header()
 */
function theme_umsg_list_header__thread_started() {
  return array(
    'data' => t('Started'),
    'field' => 'thread_started',
    'class' => array('umsg-header-threadstarted'),
    '#weight' => -15,
  );
}

/**
 * Theme a block which displays the number of new messages a user has.
 */
function theme_umsg_new_block($count) {
  $count = $count['count'];
  if ($count == 0) {
    $text = t('Click here to go to your messages.');
  }
  else {
    $text = format_plural($count, 'You have a new message! Click here to read it.', 'You have @count new messages! Click here to read them.');
  }

  return l($text, 'messages', array('attributes' => array('id' => 'umsg-new-link')));
}

/**
 * Used to theme and display user recipients.
 *
 * Wrapper for theme_username() with a few additional options.
 */
function theme_umsg_username($variables) {
  $recipient = $variables['recipient'];
  $options = $variables['options'];
  if (!isset($recipient->uid)) {
    $recipient->uid = $recipient->recipient;
  }
  if (!empty($options['plain'])) {
    $name = strip_tags(format_username($recipient));
    if (!empty($options['unique'])) {
      $name .= ' [user]';
    }
    return $name;
  }
  else {
    return theme('username', array('account' => $recipient));
  }
}
