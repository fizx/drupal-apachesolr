// $Id$
(function ($) {

Drupal.behaviors.apachesolr = {
  attach: function(context, settings) {
    $('.apachesolr-hidden-facet', context).hide();
    $('<a href="#" class="apachesolr-showhide"></a>').text(Drupal.t('Show more')).click(function() {
      if ($(this).parent().find('.apachesolr-hidden-facet:visible').length == 0) {
        $(this).parent().find('.apachesolr-hidden-facet').show();
        $(this).text(Drupal.t('Show fewer'));
      }
      else {
        $(this).parent().find('.apachesolr-hidden-facet').hide();
        $(this).text(Drupal.t('Show more'));
      }
      return false;
    }).appendTo($(settings.apachesolr_show_more_blocks, context));

    if (settings.apachesolr_facetstyle == 'checkboxes') {
      // Find all facet links and give them a checkbox
      $('.apachesolr-facet', context).each(Drupal.apachesolr.addCheckbox);
      // Find all unclick links and turn them into checkboxes
      $('.apachesolr-unclick', context).each(Drupal.apachesolr.makeCheckbox);
    }
  }
}

Drupal.apachesolr = {}

Drupal.apachesolr.addCheckbox = function() {
  // Create an unchecked checkbox.
  var checkbox = $('<input type="checkbox" class="facet-checkbox" />');
  // Put href in context scope to be visible in the anonymous function.
  var href = $(this).attr('href');
  checkbox.click(function(){
    window.location.href = href;
  });
  $(this).before(checkbox).before('&nbsp;');
}

Drupal.apachesolr.makeCheckbox = function() {
  // Create a checked checkbox.
  var checkbox = $('<input type="checkbox" class="facet-checkbox" checked="true" />');
  // Put href in context scope to be visible in the anonymous function.
  var href = $(this).attr('href');
  checkbox.click(function(){
    window.location.href = href;
  });
  // Add the checkbox, hide the link.
  $(this).before(checkbox).hide();
}

})(jQuery);