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

    $passed_ids = !is_array($ids) ? $ids : array();

    switch ($scope) {
      case 'list':
      case 'list_trash':
        $query = $this->list_threads($scope, $account);
        break;
      case 'list_sent':
        $query = $this->list_threads($scope, $account);
        break;

      default:
        break;
    }


    return $query;
  }

  private function list_threads($scope, $account) {

    // !!! To figure out for SENT messages.
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
    else {
      $count_query->condition('mi.recipient', $account->uid);
    }
    dsm($trash_status, '$trash_status');

    // Trash messages
    $query->condition('mi.archived', $trash_status);
    
    $count_query->condition('mi.deleted', 0);
    $query->setCountQuery($count_query);

    // Required columns
    $query->addField('mi', 'thread_id');
    $query->addExpression('SUBSTRING(m.body, 1, 50)', 'subject');
    $query->addExpression('MAX(m.timestamp)', 'last_updated');
    $query->addExpression('SUM(mi.is_new)', 'is_new');

    // Load enabled columns
    $fields = array('participants', 'subject');

    if (in_array('count', $fields)) {
      // We only want the distinct number of messages in this thread.
      //$query->addExpression('COUNT(distinct mi.mid)', 'count');
    }
    if (in_array('participants', $fields)) {
      // We deal only with one-to-one message (keep this structure for future implementation).
      $query->addExpression("(SELECT GROUP_CONCAT(DISTINCT CONCAT(mia.recipient, '_', mia.recipient_name))
                                     FROM {message_index} mia
                                     WHERE mia.thread_id = mi.thread_id AND mia.recipient <> :current)", 'participants', array(':current' => $account->uid));
    }
//!!    if (in_array('thread_started', $fields)) {
//      $query->addExpression('MIN(m.timestamp)', 'thread_started');
//    }

    if ($sent) {
      $query->condition('m.author', $account->uid);
    }
    else {
      $query->condition('mi.recipient', $account->uid);
    }

    $query->condition('mi.deleted', 0);
    $query->condition('mi.archived', $trash_status);
    $query->groupBy('mi.thread_id');
    $query->orderBy('last_updated', 'DESC');
    $query->limit(variable_get('umsg_per_page', 25));

    //dsm($query->execute());

    return $query;
  }

  public function countUnread($account, $scope) {
    $trash_status = ($scope == 'trash') ? 1 : 0;
    
    $query = db_select('message_index', 'mi');
    $query->addExpression('COUNT(DISTINCT thread_id)', 'unread_count');
    return $query
                    ->condition('mi.deleted', 0)
                    ->condition('mi.archived', $trash_status)
                    ->condition('mi.is_new', 1)
                    ->condition('mi.recipient', $account->uid)
                    ->execute()
                    ->fetchField();
  }

  public function save($entity, DatabaseTransaction $transaction = NULL) {

    dsm('test save');

    if (isset($entity->is_new)) {
      $entity->created = REQUEST_TIME;
    }
    $entity->changed = REQUEST_TIME;
    //return parent::save($entity, $transaction);
  }

  /**
   * Create and return a new entity.
   */
//  public function create(array $values = array()) {
//    
//    dsm('test 2345');
//    
//    $entity = new stdClass();
//    $entity->recipien = 2;
//    $entity->basic_id = 0;
//    $entity->bundle_type = 'first_example_bundle';
//    $entity->item_description = '';
//    return $entity;
//  }

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
