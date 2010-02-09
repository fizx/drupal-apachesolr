// $Id$

Drupal.behaviors.apachesolr = function(context) {
  $('.apachesolr-hidden-facet').hide();
  $('<a href="#" class="apachesolr-showhide"></a>').text(Drupal.settings.apachesolr.showMore).click(function() {
    if ($(this).parent().find('.apachesolr-hidden-facet:visible').length == 0) {
      $(this).parent().find('.apachesolr-hidden-facet').show();
      $(this).text(Drupal.settings.apachesolr.showFewer);
    }
    else {
      $(this).parent().find('.apachesolr-hidden-facet').hide();
      $(this).text(Drupal.settings.apachesolr.showMore);
    }
    return false;
  }).appendTo($('.block-apachesolr_search:has(.apachesolr-hidden-facet), .block-apachesolr:has(.apachesolr-hidden-facet)'));
}
