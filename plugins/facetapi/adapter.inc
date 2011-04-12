<?php

/**
 * @file
 * Classes used by the Facet API module.
 */

/**
 * Facet API adapter for the Apache Solr Search Integration module.
 */
class ApacheSolrFacetapiAdapter extends FacetapiAdapter {

  /**
   * Back-end specific function for adding filters.
   *
   * Typically will call $this->getActiveItems($facet);
   *
   * @param array $facet
   *   The facet definition.
   * @param mixed $query
   *   A mixed value containing the query in the backend's native API.
   *
   * @return FacetapiFacet
   *   The facet rendering object object.
   */
   public function addActiveFilters(array $facet, $query) {

     $active = $this->getActiveItems($facet);
     //$operator = facetapi_get_setting('operator', $searcher, '', $facet['name']);
     $operator = 'and';
     if (FACETAPI_OPERATOR_OR != $operator) {
       foreach ($active as $value => $item) {
         $query->addFilter($facet['field'], $value);
       }
     }
     else {
       // OR facet.
       $local = "tag={$facet['field']}";
       $values = array_keys($active);
       if ($values) {
         $query->addFilter($facet['field'], '(' . implode(' OR ', $values) . ')', FALSE, $local);
       }
     }
   }

  /**
   * Returns a boolean flagging whether $this->_searcher executed a search.
   */
  public function searchExecuted() {
    return apachesolr_has_searched();
  }

  /**
   * Returns the search keys.
   */
  public function getSearchKeys() {
    if (NULL === $this->keys) {
      if ($query = apachesolr_current_query()) {
        return $query->get_query_basic();
      }
    }
    else {
      return $this->keys;
    }
  }

  /**
   *
   */
  public function addFacet(array $facet, $query) {
    parent::addFacet($facet, $query);
    $queries = array();
    foreach ($this->getActiveItems($facet) as $value => $item) {
      $queries[] = $facet['field alias'] . ':' . $value;
    }
    if (!empty($queries)) {
      $query->params['fq'][$facet['field alias']] = $queries;
    }
  }
}
