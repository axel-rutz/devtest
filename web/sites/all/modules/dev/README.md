How to use
==========

* Install this module alongside with features module.
* Go to admin/structure/features/settings and change "Default export path" to a location that is writable.
(If your sites/all/modules directory is already writable, you don't need this module.)
* Update a feature, which will then be written to the configured (writable) location
* Drupal will recognize the feature at that location from then. 
(This will not work for newly created features that are not yet in sites/all/modules.)
