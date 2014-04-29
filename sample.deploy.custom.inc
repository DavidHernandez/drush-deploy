<?php

/**
 * @file
 * Includes samples of custom commands for custom deploys.
 */

/**
 * Adds custom commands to the deploy script.
 */
function deploy_custom_commands() {
  // This command in order to be called needs to be on disable_ui_modules.deploy.inc
  $commands['disable-ui-modules'] = array(
    'description' => 'Disables some UI modules that are no necessary',
    'aliases' => array(
      'no-ui',
      'dis-ui',
    ),
    'examples' => array(
      'drush @site disable-ui-modules' => 'Disables the UI modules on the specified @site.'
    ),
  );
  return $commands;
}

/**
 * Sample function for a "test_environment".
 */
function deploy_custom_environment_pre_test_environment($alias) {
  drush_invoke('en', array('simpletest'));
}
