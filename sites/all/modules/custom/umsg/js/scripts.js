(function($) {

  Drupal.behaviors.aboutPager = {
    attach: function(context, settings) {

      // Set views pager links as node titles.
      var dSettings = Drupal.settings;
      if (dSettings.aboutPager != undefined) {
        var titles = (dSettings.aboutPager != undefined) ? dSettings.aboutPager.titles : '';

        if (titles != '') {



        }
      }
    }

  }; 

}(jQuery));