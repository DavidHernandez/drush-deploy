<?php

/**
 * @file
 * 
 * Deployment commands for Drush.
 * 
 * Required values on site alias:
 * 
 *   $alias = array(
 *     'make file' => '/path/to/file.make',
 *     'root' => '/path/to/drupal/installation',
 *     'backup folder' => '/path/to/backup/folder',
 *     'files folder' => '/path/to/files/folder',
 *     'environment' => 'test_environment',
 *     'databases' => array(
 *       'default' => array(
 *         'default' => array(
 *           'driver' => 'mysql',
 *           'username' => 'username',
 *           'password' => 'password',
 *           'port' => '',
 *           'host' => 'localhost',
 *           'database' => 'dbname',
 *         ),
 *       ),
 *     ),
 *     // Optional
 *     'extra settings' = array(
 *       '',
 *     ),
 *   );
 */

/**
 * Implements hook_drush_help().
 */
function deploy_drush_help($section) {
  switch ($section) {
    case 'site-deploy':
      return dt('Does a full deploy to a new environment.');
    case 'site-set-up':
      return dt('Creates a copy of the site in the specified environment.');
  }
}

/**
 * Implements hook_drush_command().
 */
function deploy_drush_command() {
  $command['site-set-up'] = array(
  	'description' => 'Sets up a copy of the site.',
    'aliases' => array(
  	  'site-setup',
      'ssu',
  	),
    'examples' => array(
      'drush @alias site-set-up' => 'Creates a copy of the site in the specified environment.',
    ),
  );
  $command['site-deploy'] = array(
  	'description' => 'Deploys a site',
    'aliases' => array(
  	  'deploy',
      'sd',
  	),
    'examples' => array(
      'drush @alias deploy' => 'Deploys a site to a new environment.',
    ),
  );
  if (drush_include(DRUSH_BASE_PATH . '/commands/deploy', 'deploy.custom') == TRUE) {
    $command += deploy_custom_commands();
  }
  return $command;
}

/**
 * Validates drush site-set-up.
 */
function drush_deploy_site_set_up_validate() {
  deploy_validation_callback();
}

/**
 * Drush callback for site-set-up command.
 */
function drush_deploy_site_set_up() {
  $alias = deploy_get_alias();
  deploy_site_from_make($alias);
  deploy_prepare_settings($alias);
  deploy_restore_files_folder($alias);
  deploy_fix_permissions($alias);
}

/**
 * Validates drush site-deploy.
 */
function drush_deploy_site_deploy_validate() {
  deploy_validation_callback();
}

/**
 * Drush callback for site-deploy command.
 */
function drush_deploy_site_deploy() {
  $alias = deploy_get_alias();
  drush_deploy_site_set_up();
  deploy_backup_site($alias);
  deploy_enable_site($alias);
  if (drush_include(DRUSH_BASE_PATH . '/commands/deploy', 'deploy.custom') == TRUE) {
    deploy_custom_environment_commands($alias);
  }
  deploy_update_database($alias);
  deploy_revert_all_features($alias);
  deploy_clear_caches($alias);
}

/**
 * Deployment steps.
 */

/**
 * Wrapper for make command.
 */
function deploy_site_from_make($alias) {
  $make = deploy_get_make_from_alias();
  $buildpath = deploy_get_buildpath_from_alias();
  $arguments = array(
    'makefile' => $make,
    'build path' => $buildpath,
  );
  $options = array(
    'working-copy' => TRUE,
    'no-patch-txt' => TRUE,
    'force-complete' => TRUE,
  );
  drush_invoke_process(deploy_get_alias_name($alias), 'make', $arguments, $options);
}

/**
 * Creates the settings.php.
 */
function deploy_prepare_settings($alias) {
  $buildpath = deploy_get_buildpath_from_alias($alias);
  $settings = $buildpath . '/sites/default/settings.php';
  $default_settings = $buildpath . '/sites/default/default.settings.php';
  $command = array(
    "sudo cp $default_settings $settings",
    "sudo chmod 777 $settings",
  );
  deploy_execute_drush_commands($command);
  deploy_append_settings_data($alias, $settings);
}

/**
 * Restores the files folder.
 * Requires 'files folder' to be defined on the alias.
 */
function deploy_restore_files_folder($alias) {
  $files_destination = deploy_get_files_folder($alias);
  $files_source = $alias['files folder'];
  $command = "sudo ln -s $files_source $files_destination";
  drush_shell_exec($command);
}

/**
 * Fixes the Drupal and files folder permissions.
 */
function deploy_fix_permissions($alias) {
  deploy_fix_drupal_permissions($alias);
  deploy_fix_files_permissions($alias);
}

/**
 * Makes a backup of the current site.
 * Requires 'backup folder' to be defined on the $alias.
 */
function deploy_backup_site($alias) {
  $backup_folder = deploy_get_backups_folder($alias);
  $release = deploy_get_release_name();
  $backup_file = $backup_folder . '/' . $release . '.sql';

  $arguments = array();
  $options = array();
  
  $options['structure-tables-list'] = 'cache,cache_filter,cache_menu,cache_page,history,search_*,sessions,watchdog';
  $options['result-file'] = $backup_file;
  // $options['data-only'];

  // drush_invoke_process(deploy_get_alias_name($alias), 'sql-dump', $arguments, $options);
  drush_invoke_process('@local', 'sql-dump', $arguments, $options);
}

/**
 * Enables the new release.
 */
function deploy_enable_site($alias) {
  $destination = deploy_get_current_path_from_alias($alias);
  $source = deploy_get_buildpath_from_alias($alias);

  $commands = array(
    "sudo rm $destination",
  	"sudo ln -s $source $destination",
  	//"sudo ln -s $ $current /var/www/current",
  );
  deploy_execute_drush_commands($commands);
}

/**
 * Executes the updates on the Database.
 */
function deploy_update_database($alias) {
  drush_invoke_process(deploy_get_alias_name($alias), 'updb', array(), array('y'));
}

/**
 * Reverts all the features.
 */
function deploy_revert_all_features($alias) {
  drush_invoke_process(deploy_get_alias_name($alias), 'fra', array(), array('y'));
}

/**
 * Clears o rebuilds the caches.
 */
function deploy_clear_caches($alias) {
  if (drush_drupal_major_version() >= 8) {
    drush_invoke_process(deploy_get_alias_name($alias), 'cr');
  }
  else {
    drush_invoke_process(deploy_get_alias_name($alias), 'cc', array('all'));
  }
}

/**
 * Helpers.
 */

/**
 * Returns the release name.
 */
function deploy_get_release_name($release_name = NULL) {
  static $release = '';
  if (empty($release)) {
    $release = deploy_generate_release_name($release_name);
  }
  return $release;
}

/**
 * Generates a name for the release.
 */
function deploy_generate_release_name($release_name = NULL) {
  if (!is_null($release_name)) {
    $release = $release_name;
  }
  else {
    $release = date('Ymd_His');
  }
  return $release;
}

/**
 * Gets the list of all aliases defined in the system.
 */
function deploy_get_defined_aliases() {
  drush_sitealias_load_all();
  $aliases = drush_get_context('site-aliases');
  return $aliases;
}

/**
 * Gets the full definition of the alias.
 */
function deploy_get_alias($alias = NULL) {
  static $aliases = array();
  if (is_null($alias)) {
    $alias = drush_get_context('DRUSH_TARGET_SITE_ALIAS');
  }
  if (empty($aliases)) {
    $aliases = deploy_get_defined_aliases();
  }
  $data = FALSE;
  if (isset($aliases[$alias])) {
    $data = $aliases[$alias];
  }
  return $data;
}

/**
 * Gets the alias name from the alias full definition.
 */
function deploy_get_alias_name($alias) {
  $alias_name = '@' . $alias['#name'];
  return $alias_name;
}

/**
 * Gets the make file from the alias.
 * Requires 'make file' to be defined on the $alias.
 */
function deploy_get_make_from_alias($alias = NULL) {
  if (is_null($alias)) {
    $alias = deploy_get_alias();
  }
  return $alias['make file'];
}

/**
 * Gets the system user of the remote server.
 */
function deploy_get_remote_user($alias = NULL) {
  if (is_null($alias)) {
    $alias = deploy_get_alias();
  }
  $remote_user = 'www-data';
  if (isset($alias['remote-user'])) {
    $remote_user = $alias['remote-user'];
  }
  return $remote_user;
}

/**
 * Gets the backups folder for the alias site.
 */
function deploy_get_backups_folder($alias = NULL) {
  if (is_null($alias)) {
    $alias = deploy_get_alias();
  }
  $backup_folder = $alias['backup folder'];
  return $backup_folder;
}

/**
 * Gets the buildpath from the alias, including the release name.
 * Requires 'root' to be defined on the $alias.
 */
function deploy_get_buildpath_from_alias($alias = NULL, $release_name = NULL) {
  static $buildpath = '';
  if (empty($buildpath)) {
    $root = deploy_get_current_path_from_alias($alias);
    $length = strrpos($root, '/') + 1;
    $path = substr($root, 0, $length);
    $buildpath = $path . deploy_get_release_name($release_name);
  }
  return $buildpath;
}

/**
 * Gets the buildpath from the alias, including the release name.
 * Requires 'root' to be defined on the $alias.
 */
function deploy_get_current_path_from_alias($alias = NULL, $release_name = NULL) {
  static $current_path = '';
  if (empty($current_path)) {
    if (is_null($alias)) {
      $alias = deploy_get_alias();
    }
    $current_path = $alias['root'];
  }
  return $current_path;
}

/**
 * Gets the files folder of the Drupal installation of the given alias.
 */
function deploy_get_files_folder($alias) {
  $buildpath = deploy_get_buildpath_from_alias($alias);
  $files_folder = $buildpath . '/sites/default/files';
  return $files_folder;
}

/**
 * Get all the relevant settings to be added to the settings file.
 */
function deploy_get_data_for_settings_file($alias = NULL) {
  if (is_null($alias)) {
    $alias = deploy_get_alias();
  }
  $settings = FALSE;
  if (isset($alias['extra settings'])) {
    $settings = $alias['extra settings'];
  }
  return $settings;
}

/**
 * Creates a string with the extra settings to be added to the settings file.
 */
function deploy_format_extra_settings($settings) {
  $output = '';
  foreach ($settings as $var_name => $setting) {
    if (is_numeric($var_name)) {
      $output .= "\n$setting";
    }
    else {
      $output .= "\n$var_name = $setting";
    }
  }
  return $output;
}

/**
 * Adds the string of custom settings to the settings file.
 * Requires 'databases' to be defined on the alias.
 */
function deploy_append_settings_data($alias, $settings) {
  $db_info = $alias['databases'];
  $db_string = "\n" . '$databases = ' . var_export($db_info, 'TRUE') . ';';

  $settings_string = '';
  if ($settings_data = deploy_get_data_for_settings_file()) {
    $settings_string = deploy_format_extra_settings($settings_data);
  }

  $data = $db_string . $settings_string;

  file_put_contents($settings, $data, FILE_APPEND);
}

/**
 * Executes multiple bash commands through drush_shell_exec().
 */
function deploy_execute_drush_commands($commands) {
  foreach ($commands as $command) {
    drush_print("Executing shell command: $command");
    $result = drush_shell_exec($command);

    $returned_content = drush_shell_exec_output();
    foreach ($returned_content as $line) {
      drush_print($line);
    }

    if ($result === FALSE) {
      exit(1);
    }
  }
}

/**
 * Fixes Drupal installation permissions.
 * Adds the optional setting 'remote-user' in the alias.
 */
function deploy_fix_drupal_permissions($alias) {
  $buildpath = deploy_get_buildpath_from_alias($alias);
  $remote_user = deploy_get_remote_user($alias);
  $commands = array(
    "sudo chown $remote_user:www-data $buildpath -R",
    "sudo chmod u-x,g-x $buildpath -R",
    "sudo chmod u=rwX,g=rX,o= $buildpath -R",
  );
  deploy_execute_drush_commands($commands);
}

/**
 * Fixes files folder permissions.
 */
function deploy_fix_files_permissions($alias) {
  $files = deploy_get_files_folder($alias);
  $commands = array(
    "sudo chmod 2770 $files",
    "sudo chown www-data:www-data $files -R",
    "sudo chmod u=rwX,g=rwX,o= $files -R",
  );
  deploy_execute_drush_commands($commands);
}

/**
 * Validation function for all the deployment commands.
 */
function deploy_validation_callback() {
  if (!$alias = drush_get_context('DRUSH_TARGET_SITE_ALIAS')) {
    drush_set_error(DRUSH_FRAMEWORK_ERROR, dt('The alias is required: drush @alias [command].'));
  }
}
