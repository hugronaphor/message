(function($) {

  // I suppose we use JQuery 1.4

  Drupal.behaviors.umsgAutosubmit = {
    attach: function(context, settings) {
//      var timerid;
//      $(".form-item-search-string").delegate("input#edit-search-string", "keyup", function() {
//        if ($(this).val().length > 2) {
//
//          clearTimeout(timerid);
//          timerid = setTimeout(function() {
//            $("input#index-search-submit").trigger("click");
//          }, 600);
//        }
//      });

    }
  };

  Drupal.behaviors.umsgCheckAll = {
    attach: function(context, settings) {

      // To improve
      $('#edit-check-all, #edit-check-all--2').live("click", function(e) {
        if (this.checked) {
          $('.umsg-list td input:checkbox').attr('checked', true).parents('tr').addClass('selected');
        } else {
          $('.umsg-list td input:checkbox').attr('checked', false).parents('tr').removeClass('selected');
        }
      });

    }
  };

  Drupal.behaviors.umsgView = {
    attach: function(context, settings) {

      $('.umsg-message-teaser').click(function(e) {

        if (e.target.className.indexOf('umsg-in-thread-link') >= 0) {
          return;
        }

        $parent = $(this).parents('.umsg-message');
        $actionBtns = $parent.find('.umsg-action-btns').html();
        $shortBody = $parent.find('.store-short-body').html();

        if ($(this).hasClass('selected')) {
          $(this).removeClass('selected');
          $parent.find('.umsg-message-additional').slideUp("fast", function() {
            $parent.find('.umsg-message-body-short').html($shortBody);
          });
        } else {
          $(this).addClass('selected');
          $parent.find('.umsg-message-additional').slideDown("fast", function() {
            $parent.find('.umsg-message-body-short').html($actionBtns);
          });
        }

        //return false;
      });

      // Add reply form.
      $('a.reply-umsg-in-thread-link').live("click", function(e) {

        $parent = $(this).parents('.umsg-message');
        $shortBody = $parent.find('.store-short-body').html();
        $fullBody = $parent.find('.umsg-message-body').text();

        $replyForm = $('#umsg-view-base-reply-form form#umsg-new').clone();

        // Add reply form.
        $parent.find('.umsg-message-additional .umsg-reply-form').html($replyForm);
        //attach textarea value.
        $parent.find('.umsg-reply-form form #edit-body').val('\n--------------------\n' + $fullBody);
        e.preventDefault();
      });

      // Clean reply form.
      $('a.cancel-umsg-thread-reply-form').live("click", function(e) {
        $parent = $(this).parents('.umsg-message');
        $parent.find('.umsg-message-additional .umsg-reply-form').html('');
        e.preventDefault();
        return false;
      });

    }
  };

}(jQuery));