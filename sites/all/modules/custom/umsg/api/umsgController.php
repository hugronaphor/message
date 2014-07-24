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
      $account = $this->current_user;
    }

    $passed_ids = !is_array($ids) ? $ids : array();

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
