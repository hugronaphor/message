(function($) {

  Drupal.behaviors.checkAllUmsg = {
    attach: function(context, settings) {

// Select the inner-most table in case of nested tables.
      $('th.select-all', context).closest('table').once('table-select', Drupal.tableSelect);


    }

  };

}(jQuery));