<?php

/**
 * UmsgController
 * 
 * Kind of Controller.
 * 
 * @scope: Perform queries to external message DataBase.
 */
class UmsgController {

  function __construct() {
    $this->current_user = $this->getCurrentUser();
    $this->umsg_db_name = variable_get('umsg_db_name');
    $this->setUmsgDbActive();
  }

  public function load($scope = 'list', $ids = array(), $account = NULL, $search_string = NULL, $range = array(0)) {

    // Get current user by default.
    if ($account === NULL && $this->current_user !== NULL) {

      if ($this->current_user === NULL) {
        return new stdClass();
      }

      $account = $this->current_user;
    }

    $passed_ids = $ids;

    switch ($scope) {
      case 'list':
      case 'list_trash':
      case 'list_sent':
        $query = $this->list_threads($scope, $account, $search_string, $range);
        break;
      case 'thread_messages':
        $query = $this->loadMessages($passed_ids);
        break;

      default:
        break;
    }


    return $query;
  }

  private function list_threads($scope, $account, $search_string, $range) {

    $trash_status = ($scope == 'list_trash') ? 1 : 0;
    $sent = ($scope == 'list_sent') ? 1 : 0;

//    dsm('To fix sort listing in inbox/Sent.');

    $query = db_select('message', 'm');
    $query->join('message_index', 'mi', 'mi.mid = m.mid');
    
    // Required columns
    $query->addField('mi', 'thread_id');

    $query->addExpression('SUM(mi.archived)', 'archived');
    // Strip message field in order to be used as short teaser for thread.
    $query->addExpression('m.subject', 'subject');
    $query->addExpression('MAX(m.timestamp)', 'last_updated');
    $query->addExpression('SUM(mi.is_new)', 'is_new');

    // Load enabled columns
    $fields = array('participants', 'subject', 'archived');

    if (in_array('participants', $fields)) {
      // We deal only with one-to-one message (keep this structure for future implementation).
//      $query->addExpression("(SELECT GROUP_CONCAT(DISTINCT CONCAT(mia.recipient, '_', mia.recipient_name))
      $query->addExpression("(SELECT GROUP_CONCAT(DISTINCT CONCAT(mia.recipient))
                                     FROM {message_index} mia
                                     WHERE mia.thread_id = mi.thread_id AND mia.recipient <> :current)", 'participants', array(':current' => $account->uid));
    }

    if ($sent) {
      $query->condition('m.author', $account->uid);
    }

    if ($search_string) {
      $query->condition('m.body', '%' . db_like($search_string) . '%', 'LIKE');
    }


    $query->condition('mi.recipient', $account->uid);
    // Trash messages.
    $query->condition('archived', $trash_status);
    $query->groupBy('mi.thread_id');
    $query->orderBy('last_updated', 'DESC');
    $query->range($range[0], $range[1]);

    return $query;
  }

  /**
   * Load a thread with all the messages and participants.
   *
   * @param $thread_id
   *   Thread id, mi.thread_id or m.mid of the first message in that thread.
   * @param $account
   *   User object for which the thread should be loaded, defaults to
   *   the current user.
   * @param $start
   *   Message offset from the start of the thread.
   *
   * @return
   *   $thread object, with keys messages, participants, title and user. messages
   *   contains an array of messages, participants an array of user, subject the
   *   subject of the thread and user the user viewing the thread.
   *
   *   If no messages are found, or the thread_id is invalid, the function returns
   *   FALSE.

   * @ingroup api
   */
  public function threadLoad($thread_id, $account = NULL) {

    $thread_id = (int) $thread_id;
    $threads = array();
    if ($thread_id > 0) {
      $thread = array('thread_id' => $thread_id);

      if (is_null($account)) {
        $account = $this->current_user;
      }

      if (!isset($threads[$account->uid])) {
        $threads[$account->uid] = array();
      }

      // Load the list of participants.
      $thread['participants'] = $this->getThreadParticipants($thread_id, $account, FALSE, 'view');

      $query = $this->loadMessages(array($thread_id), $account);

      $thread['messages'] = $this->messageLoadMultiple($query->execute()->fetchCol(), $account);

      // If there are no messages, don't allow access to the thread.
      if (empty($thread['messages'])) {
        $thread = FALSE;
      }
      else {
        // General data, assume subject is the same.
        $count_msg = count($thread['messages']);
        $thread['user'] = $account;
        $message = current($thread['messages']);
        $thread['subject'] = $thread['subject-original'] = $message->subject;
        $thread['last_updated'] = $message->timestamp;
        // Detect if we are on archived/not archived thread.
        $thread['archived'] = $message->archived;
      }
      $threads[$account->uid][$thread_id] = $thread;
      return $threads[$account->uid][$thread_id];
    }
    return FALSE;
  }

  /**
   * Load all thread participants.
   */
  function getThreadParticipants($thread_id, $account = NULL) {
    $query = db_select('message_index', 'mi');
    $query
            ->fields('mi', array('recipient', 'recipient_name'))
            ->condition('mi.thread_id', $thread_id);

    return $query->groupBy('mi.recipient')->execute()->fetchAll();
  }

  public function messageLoadMultiple($mids, $account = NULL) {

    if (empty($mids)) {
      return array();
    }

    $query = db_select('message_index', 'mi');
    $query->leftJoin('message', 'm', 'm.mid = mi.mid');
    $query->fields('mi');
    $query->fields('m');

    $query
            ->condition('m.mid', $mids, 'IN')
            // Order by timestamp first.
            ->orderBy('m.timestamp', 'DESC')
            ->orderBy('mi.mid', 'ASC');
    if ($account) {
      $query->condition('mi.recipient', $account->uid);
    }

    return $query->execute()->fetchAll();
  }

  /**
   * Get prev/next thread id based on last updated message in db.
   */
  public function threadCheckPager($thread, $scope) {

    if (!is_array($thread) || empty($thread)) {
      return NULL;
    }

    $this->setUmsgDbActive();
    $query = db_select('message_index', 'mi');
    $query->innerJoin('message', 'm', 'm.mid = mi.mid');
    $query->fields('mi');
    $query->fields('m');

    // Exclude archived.
    $query->condition('mi.archived', $thread['archived']);
    $query->condition('mi.thread_id', $thread['thread_id'], '<>');
    $query->condition('mi.recipient', $this->current_user->uid);
    $query->addExpression('MAX(m.timestamp)', 'last_updated');
    if ($scope == 'prev') {
      $query->condition('m.timestamp', $thread['last_updated'], '>');
      $query->orderBy('last_updated', 'ASC');
    }
    else {
      $query->condition('m.timestamp', $thread['last_updated'], '<');
      $query->orderBy('last_updated', 'DESC');
    }

    $query->groupBy('mi.thread_id');
    $query->range(0, 1);

    $result = $query->execute()->fetchObject();
    return isset($result->thread_id) ? $result->thread_id : NULL;
  }

  /**
   * Load all thread messages.
   */
  public function loadMessages($threads, $account = NULL) {
    $query = db_select('message_index', 'mi');
    $query->addField('mi', 'mid');
    $query->join('message', 'm', 'm.mid = mi.mid');

    $query
            ->condition('mi.thread_id', $threads)
            ->groupBy('m.timestamp')
            ->groupBy('mi.mid')
            // Order by timestamp first.
            ->orderBy('m.timestamp', 'ASC')
            ->orderBy('mi.mid', 'ASC');
    // Always check that user it's owner and have rights for actions.
    if ($account) {
      $query->condition('mi.recipient', $account->uid);
    }

    return $query;
  }

  public function countUnread($account, $scope) {
    $trash_status = ($scope == 'trash') ? 1 : 0;

    $query = db_select('message_index', 'mi');
    $query->addExpression('COUNT(DISTINCT thread_id)', 'unread_count');

    $query->condition('mi.archived', $trash_status);
    // We count all messages in trash and unreaded in inbox.
    if (!$trash_status) {
      $query->condition('mi.is_new', 1);
    }

    $query->condition('mi.recipient', $account->uid);
    return $query->execute()->fetchField();
  }

  /**
   * Delete or Archive a message.
   * 
   * @todo 
   *   Delete messages on fly, if $delete == 0
   *
   * @param $mid
   *   Message id, m.mid field.
   * @param $delete
   *   Either deletes or archive the thread (1 => archive, 0 => delete)
   * @param $account
   *   User account for which the delete action should be carried out.
   *   Set to NULL to delete for all users.
   */
  public function changeMsg($mid, $delete, $account = NULL) {

    // Archive message.
    if ($delete) {
      $update = db_update('message_index')
              ->fields(array('archived' => 1))
              ->condition('mid', $mid);
      if ($account) {
        $update->condition('recipient', $account->uid);
      }
      $update->execute();
    }
    // Delete message.
    else {
      $delete = db_delete('message_index')
              ->condition('mid', $mid);
      if ($account) {
        $delete->condition('recipient', $account->uid);
      }
      $delete->execute();

      // Check if this message is attached to other users.
      $query = db_select('message_index', 'mi');
      $query->addField('mi', 'mid');
      $query->join('message', 'm', 'm.mid = mi.mid');
      $query->condition('m.mid', $mid);

      $exist = $query->execute()->fetchCol();

      if (empty($exist)) {
        // Delete message, if there are not references.
        $delete = db_delete('message')
                ->condition('mid', $mid)
                ->execute();

        return FALSE;
      }
    }

    return TRUE;
  }

  /**
   * Changes the read/new status of a single message.
   *
   * @param $mid
   *   Message id
   * @param $status
   *   Either UMSG_READ or UMSG_UNREAD
   * @param $account
   *   User object, defaults to the current user
   */
  function changeMsgReadStatus($mid, $status, $account = NULL) {
    $this->setUmsgDbActive();

    if (!$account) {
      $account = $this->current_user;
    }

    db_update('message_index')
            ->fields(array('is_new' => $status))
            ->condition('mid', $mid)
            ->condition('recipient', $account->uid)
            ->execute();

    $this->setDefaultDbActive();
  }

  /**
   * Internal function to save a message.
   *
   * @param $message
   *   A $message array with the data that should be saved. If a thread_id exists
   *   it will be created as a reply to an existing thread. If not, a new thread
   *   will be created.
   *
   * @return
   *   The updated $message array.
   */
  public function send($message) {
    
    $transaction = db_transaction();
    try {

      $query = db_insert('message_index')->fields(array('mid', 'thread_id', 'recipient', 'is_new', 'archived'));

      // 1) Save the message body first.
      $args = array();
      $args['author'] = $message->author->uid;
      $args['body'] = $message->body;
      $args['subject'] = $message->subject;
      $args['timestamp'] = $message->timestamp;
      $mid = db_insert('message')
              ->fields($args)
              ->execute();
      $message->mid = $mid;

      // Thread ID is the same as the mid if it's the first message in the thread.
      if (!isset($message->thread_id)) {
        $message->thread_id = $mid;
      }

      // 2) Save message to recipients.
      // Each recipient gets a record in the message_index table.
      foreach ($message->recipients as $recipient) {
        $query->values(array(
          'mid' => $mid,
          'thread_id' => $message->thread_id,
          'recipient' => $recipient->uid,
          'recipient_name' => (string) $recipient->name,
          'is_new' => UMSG_UNREAD,
          'archived' => 0,
        ));
      }

      // We only want to add the author to the message_index table, if the message has
      // not been sent directly to him.
      if (!isset($message->recipients[$message->author->uid])) {
        $query->values(array(
          'mid' => $mid,
          'thread_id' => $message->thread_id,
          'recipient' => $message->author->uid,
          'recipient_name' => (string) $message->author->name,
          'is_new' => UMSG_READ,
          'archived' => 0,
        ));
      }
      $query->execute();
    }
    catch (Exception $exception) {
      $transaction->rollback();
      watchdog_exception('umsg', $exception);
      throw $exception;
    }

    // If we reached here that means we were successful at writing all messages to db.
    return $message;
  }

  private function getCurrentUser() {
    global $user;
    // Disallow anonymous access.
    return !$user->uid ? NULL : $user;
  }

  private function setUmsgDbActive() {
    db_set_active($this->umsg_db_name);
  }

  public function setDefaultDbActive() {
    db_set_active();
  }

  function __destruct() {
    $this->setDefaultDbActive();
  }

}
