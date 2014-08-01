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

  public function load($scope = 'all', $ids = array(), $account = NULL) {

    // Get current user by default.
    if ($account === NULL && $this->current_user !== NULL) {

      if ($this->current_user === NULL) {
        return new stdClass();
      }

      $account = $this->current_user;
    }

    $passed_ids = !is_array($ids) ? $ids : array();

    switch ($scope) {
      case 'all':
        $query = $this->list_all_threads($account);

        break;

      default:
        break;
    }


    return $query;

    //$query = Database::getConnection('default', 'msgdb'); // dont allow joins
    $query = db_select('message', 'm');
    $query->leftJoin('message_index', 'mi', 'mi.mid=m.mid');
    $query->fields('m');
    $query->fields('mi');
    $query->condition('m.author', $account->uid);
    if (!empty($passed_ids)) {
      $query->condition('m.mid', $passed_ids, 'IN');
    }
    $entities = $query->execute()->fetchAll();

    // To think about structure of hoe the data should be recived.
    // message can be only one, but inde seems to be only 2.
    // so: {id, field, [index1, index2]}


    return !empty($entities) ? $entities : array();
  }

  private function list_all_threads($account) {
    $query = db_select('message', 'm')->extend('PagerDefault');
    $query->join('message_index', 'mi', 'mi.mid = m.mid');

    // Create count query;
    $count_query = db_select('message', 'm');
    $count_query->addExpression('COUNT(DISTINCT mi.thread_id)', 'count');
    $count_query->join('message_index', 'mi', 'mi.mid = m.mid');
    $count_query
      ->condition('mi.recipient', $account->uid)
      ->condition('mi.deleted', 0);
    $query->setCountQuery($count_query);
//
//    dsm($query, 'test');
//    return;
    // Required columns
    $query->addField('mi', 'thread_id');
    //$query->addExpression('MIN(m.subject)', 'subject');
    $query->addExpression('MAX(m.timestamp)', 'last_updated');
    $query->addExpression('SUM(mi.is_new)', 'is_new');

    // Load enabled columns
    $fields = array('participants', 'count', 'thread_started');

    if (in_array('count', $fields)) {
      // We only want the distinct number of messages in this thread.
      $query->addExpression('COUNT(distinct mi.mid)', 'count');
    }
//    if (in_array('participants', $fields)) {
//
//      $query->addExpression("(SELECT GROUP_CONCAT(DISTINCT CONCAT(pmia.type, '_', pmia.recipient))
//                                     FROM {pm_index} pmia
//                                     WHERE pmia.type <> 'hidden' AND pmia.thread_id = pmi.thread_id AND pmia.recipient <> :current)", 'participants', array(':current' => $account->uid));
//    }
//    if (in_array('thread_started', $fields)) {
//      $query->addExpression('MIN(pm.timestamp)', 'thread_started');
//    }
    return $query
        ->condition('mi.recipient', $account->uid)
        ->condition('mi.deleted', 0)
        ->groupBy('mi.thread_id')
        //->orderByHeader(_privatemsg_list_headers(array_merge(array('subject', 'last_updated'), $fields)))
        ->limit(variable_get('umsg_per_page', 25));
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
