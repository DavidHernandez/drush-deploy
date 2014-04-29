<?php

/**
 * @file
 * Contains all the code required for the command disable-ui-modules.
 */

/**
 * Validate callback for the command disable-ui-modules.
 */
function drush_deploy_disable_ui_modules_validate() {
  deploy_validation_callback();
}

/**
 * Callback for the command disable-ui-modules.
 */
function drush_deploy_disable_ui_modules() {
  $alias = deploy_get_alias();
  $ui_modules = array(
    'views_ui',
    'context_ui',
    'field_ui',
  );

  drush_invoke_process($alias, 'dis', $ui_modules);
}