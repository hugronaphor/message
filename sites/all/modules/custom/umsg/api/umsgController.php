<?php

/**
 * UmsgBasicControllerInterface definition. *
 */
//interface UmsgControllerInterface {
//
//  public function load($ids = array(), $conditions = array());
//
//  public function save($entity);
//
//  function setUmsgDbActive();
//
//  function setDefaultDbActive();
//}

class UmsgController { // implements UmsgControllerInterface {

  function __construct() {
    $this->current_user = $this->getCurrentUser();
    $this->umsg_db_name = variable_get('umsg_db_name');
    $this->setUmsgDbActive();
  }

  public function load($scope = 'list', $ids = array(), $account = NULL) {

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
        $query = $this->list_threads($scope, $account);
        break;
      case 'thread_messages':
        $query = $this->loadMessages($passed_ids);
        break;

      default:
        break;
    }


    return $query;
  }

  private function list_threads($scope, $account) {

    $trash_status = ($scope == 'list_trash') ? 1 : 0;
    $sent = ($scope == 'list_sent') ? 1 : 0;

    $query = db_select('message', 'm')->extend('PagerDefault');
    $query->join('message_index', 'mi', 'mi.mid = m.mid');

    // Create count query;
    $count_query = db_select('message', 'm');
    $count_query->addExpression('COUNT(DISTINCT mi.thread_id)', 'count');
    $count_query->join('message_index', 'mi', 'mi.mid = m.mid');
    // Sent messages.
    if ($sent) {
      $count_query->condition('m.author', $account->uid);
    }

    $count_query->condition('mi.recipient', $account->uid);
    // Trash messages
    $count_query->condition('mi.archived', $trash_status);

    $count_query->condition('mi.deleted', 0);
    $query->setCountQuery($count_query);

    // Required columns
    $query->addField('mi', 'thread_id');

    $query->addExpression('SUM(mi.archived)', 'archivedd');
    // Strip message field in order to be used as short teaser for thread.
    $query->addExpression('SUBSTRING(m.body, 1, 50)', 'subject');
    $query->addExpression('MAX(m.timestamp)', 'last_updated');
    $query->addExpression('SUM(mi.is_new)', 'is_new');

    // Load enabled columns
    $fields = array('participants', 'subject', 'archived');

    if (in_array('participants', $fields)) {
      // We deal only with one-to-one message (keep this structure for future implementation).
      $query->addExpression("(SELECT GROUP_CONCAT(DISTINCT CONCAT(mia.recipient, '_', mia.recipient_name))
                                     FROM {message_index} mia
                                     WHERE mia.thread_id = mi.thread_id AND mia.recipient <> :current)", 'participants', array(':current' => $account->uid));
    }

    if ($sent) {
      $query->condition('m.author', $account->uid);
    }

    $query->condition('mi.recipient', $account->uid);
    $query->condition('mi.deleted', 0);
    // Trash messages.
    $query->condition('archived', $trash_status);
    $query->groupBy('mi.thread_id');
    $query->orderBy('last_updated', 'DESC');
    $query->limit(variable_get('umsg_per_page', 25));

    return $query;
  }

  /**
   * Load a thread with all the messages and participants.
   *
   * This function is called by the menu system through the %privatemsg_thread
   * wildcard.
   *
   * @param $thread_id
   *   Thread id, pmi.thread_id or pm.mid of the first message in that thread.
   * @param $account
   *   User object for which the thread should be loaded, defaults to
   *   the current user.
   * @param $start
   *   Message offset from the start of the thread.
   * @param $useAccessDenied
   *   Set to TRUE if the function should forward to the access denied page
   *   instead of not found. This is used by the menu system because that does
   *   load arguments before access checks are made. Defaults to FALSE.
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
  public function threadLoad($thread_id, $account = NULL, $start = NULL, $useAccessDenied = FALSE) {
    //$threads = &drupal_static(__FUNCTION__, array());
    $thread_id = (int) $thread_id;
    if ($thread_id > 0) {
      $thread = array('thread_id' => $thread_id);

      if (is_null($account)) {
        $account = $this->current_user;
      }

      if (!isset($threads[$account->uid])) {
        $threads[$account->uid] = array();
      }

      if (!array_key_exists($thread_id, $threads[$account->uid])) {
        // Load the list of participants.
        $thread['participants'] = $this->getThreadParticipants($thread_id, $account, FALSE, 'view');

        // Load messages returned by the messages query with privatemsg_message_load_multiple().
        // $query = _privatemsg_assemble_query('messages', array($thread_id), $thread['read_all'] ? NULL : $account);

        $query = $this->loadMessages(array($thread_id), $account);
        // Use subquery to bypass group by since it is not possible to alter
        // existing GROUP BY statements.
        $countQuery = db_select($query);
        $countQuery->addExpression('COUNT(*)');
        $thread['message_count'] = $thread['to'] = $countQuery->execute()->fetchField(); // WRONG !!!

        $thread['from'] = 1;
        // Check if we need to limit the messages.
        $max_amount = variable_get('umsg_view_max_amount', 20);

        // If there is no start value, select based on get params.
        if (is_null($start)) {
          if (isset($_GET['start']) && $_GET['start'] < $thread['message_count']) {
            $start = $_GET['start'];
          }
          elseif (!variable_get('privatemsg_view_use_max_as_default', FALSE) && $max_amount == UMSG_UNLIMITED) {
            $start = UMSG_UNLIMITED;
          }
          else {
            $start = $thread['message_count'] - (variable_get('privatemsg_view_use_max_as_default', FALSE) ? variable_get('privatemsg_view_default_amount', 10) : $max_amount);
          }
        }

        if ($start != UMSG_UNLIMITED) {
          if ($max_amount == UMSG_UNLIMITED) {
            $last_page = 0;
            $max_amount = $thread['message_count'];
          }
          else {
            // Calculate the number of messages on the "last" page to avoid
            // message overlap.
            // Note - the last page lists the earliest messages, not the latest.
            $paging_count = variable_get('privatemsg_view_use_max_as_default', FALSE) ? $thread['message_count'] - variable_get('privatemsg_view_default_amount', 10) : $thread['message_count'];
            $last_page = $paging_count % $max_amount;
          }

          // Sanity check - we cannot start from a negative number.
          if ($start < 0) {
            $start = 0;
          }
          $thread['start'] = $start;

          //If there are newer messages on the page, show pager link allowing to go to the newer messages.
          if (($start + $max_amount + 1) < $thread['message_count']) {
            $thread['to'] = $start + $max_amount;
            $thread['newer_start'] = $start + $max_amount;
          }
          if ($start - $max_amount >= 0) {
            $thread['older_start'] = $start - $max_amount;
          }
          elseif ($start > 0) {
            $thread['older_start'] = 0;
          }

          // Do not show messages on the last page that would show on the page
          // before. This will only work when using the visual pager.
          if ($start < $last_page && $max_amount != UMSG_UNLIMITED && $max_amount < $thread['message_count']) {
            unset($thread['older_start']);
            $thread['to'] = $thread['newer_start'] = $max_amount = $last_page;
            // Start from the first message - this is a specific hack to make sure
            // the message display has sane paging on the last page.
            $start = 0;
          }
          // Visual counts start from 1 instead of zero, so plus one.
          $thread['from'] = $start + 1;
          $query->range($start, $max_amount);
        }


        $thread['messages'] = $this->message_load_multiple($query->execute()->fetchCol(), $account);

        // If there are no messages, don't allow access to the thread.
        if (empty($thread['messages'])) {
          $thread = FALSE;
        }
        else {
          // General data, assume subject is the same.
          $thread['user'] = $account;
          $message = current($thread['messages']);
          $thread['subject'] = $thread['subject-original'] = $this->buildSubject($message->body);
        }
        $threads[$account->uid][$thread_id] = $thread;
      }
      return $threads[$account->uid][$thread_id];
    }
    return FALSE;
  }
  
  /**
 * Load all participants of a thread.
 *
 */
function getThreadParticipants($thread_id, $account = NULL) {
  $query = db_select('message_index', 'mi');
  $query
    ->fields('mi', array('recipient', 'recipient_name'))
    ->condition('mi.thread_id', $thread_id);

  return $query->groupBy('mi.recipient')->execute()->fetchAll();
}

  private function buildSubject($string) {
    return t('Subject') . ': ' . substr($string, 0, UMSG_STRIP_BODY) . '...';
  }

  /**
   * Load multiple messages.
   *
   * @param $mids
   *   Array of Message ids, m.mid field
   * @param $account
   *   For which account the message should be loaded.
   *   Defaults to the current user.
   *
   * @ingroup api
   */
  public function message_load_multiple($thread, $account = NULL) {

    $query = db_select('message_index', 'mi');
    $query->leftJoin('message', 'm', 'm.mid = mi.mid');
    $query->fields('mi');
    $query->fields('m');

    // Exclude archived/deleted.
    $query->condition('mi.archived', 0);
    $query->condition('mi.deleted', 0);

    $query
      ->condition('mi.thread_id', $thread)
      //->groupBy('m.timestamp')
      //->groupBy('mi.mid')
      // Order by timestamp first.
      ->orderBy('m.timestamp', 'ASC')
      ->orderBy('mi.mid', 'ASC');
    if ($account) {
      $query->condition('mi.recipient', $account->uid);
    }

    return $query->execute()->fetchAll();
  }

  /**
   * Load all thread messages.
   * 
   * @param type $threads
   * @param type $account
   * @param type $load_all
   * @return type
   */
  public function loadMessages($threads, $account = NULL, $load_all = FALSE) {
    $query = db_select('message_index', 'mi');
    $query->addField('mi', 'mid');
    $query->join('message', 'm', 'm.mid = mi.mid');
    if (!$load_all) {
      $query->condition('mi.archived', 0);
    }

    $query
      ->condition('mi.thread_id', $threads)
      ->groupBy('m.timestamp')
      ->groupBy('mi.mid')
      // Order by timestamp first.
      ->orderBy('m.timestamp', 'ASC')
      ->orderBy('mi.mid', 'ASC');
    if ($account) {
      $query->condition('mi.recipient', $account->uid);
    }

    return $query;
  }

  public function countUnread($account, $scope) {
    $trash_status = ($scope == 'trash') ? 1 : 0;

    $query = db_select('message_index', 'mi');
    $query->addExpression('COUNT(DISTINCT thread_id)', 'unread_count');

    $query->condition('mi.deleted', 0);
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
   * !At this time we just mark message as deleted,
   * may be it's better to delete them on cron job?
   *
   * @param $pmid
   *   Message id, pm.mid field.
   * @param $delete
   *   Either deletes or restores the thread (1 => archive, 0 => delete)
   * @param $account
   *   User account for which the delete action should be carried out.
   *   Set to NULL to delete for all users.
   */
  public function changeMsg($mid, $delete, $account = NULL) {
    $delete_value = 1;

    if ($delete) {
      $affected_field = 'archived';
    }
    else {
      $affected_field = 'deleted';
    }

    $update = db_update('message_index')
      ->fields(array($affected_field => $delete_value))
      ->condition('mid', $mid);
    if ($account) {
      $update->condition('recipient', $account->uid);
    }
    $update->execute();
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

    dsm($message, 'controller message obj');
//  return;
    $transaction = db_transaction();
    try {
      //drupal_alter('privatemsg_message_presave', $message);
      //field_attach_presave('privatemsg_message', $message);

      $query = db_insert('message_index')->fields(array('mid', 'thread_id', 'recipient', 'is_new', 'archived', 'deleted'));
      if (isset($message->thread_id)) {
        // The message was sent in read all mode, add the author as recipient to all
        // existing messages.
        dsm('to do !!!');
//      $query_messages = _privatemsg_assemble_query('messages', array($message->thread_id), NULL);
//      foreach ($query_messages->execute()->fetchCol() as $mid) {
//        $query->values(array(
//          'mid' => $mid,
//          'thread_id' => $message->thread_id,
//          'recipient' => $message->author->uid,
//          'type' => 'user',
//          'is_new' => 0,
//          'deleted' => 0,
//        ));
//      }
      }

      // 1) Save the message body first.
      $args = array();
      $args['author'] = $message->author->uid;
      $args['body'] = $message->body;
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
        dsm($recipient);
        $query->values(array(
          'mid' => $mid,
          'thread_id' => $message->thread_id,
          'recipient' => $recipient->uid,
          'recipient_name' => (string) $recipient->name,
          'is_new' => UMSG_UNREAD,
          'archived' => 0,
          'deleted' => 0,
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
          'deleted' => 0,
        ));
      }
      $query->execute();

//    module_invoke_all('privatemsg_message_insert', $message);
//    field_attach_insert('privatemsg_message', $message);
    } catch (Exception $exception) {
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

  private function setDefaultDbActive() {
    db_set_active();
  }

  function __destruct() {
    $this->setDefaultDbActive();
  }

}
