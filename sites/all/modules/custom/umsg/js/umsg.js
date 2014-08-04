(function($) {

  Drupal.behaviors.checkAllUmsg = {
    attach: function(context, settings) {

      // To improve
      $('#edit-check-all').click(function() {
        if (this.checked) {
          $('.umsg-list td input:checkbox').attr('checked', true).parents('tr').addClass('selected');
        } else {
          $('.umsg-list td input:checkbox').attr('checked', false).parents('tr').removeClass('selected');
        }
      });

    }

  };

}(jQuery));