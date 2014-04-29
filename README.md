drush-deploy
============

Drush Deploy is a command to handle Drupal deployments with simple Drush commands. As each project is different, it also adds some functionality to be able to customize the different websites and environments. The structure and process for the deployment is based on Capistrano.

Samples
-------

To create a copy of the code of a site:
    drush @alias site-set-up

To do a deploy of a site:
    drush @alias site-deploy

Configuration
-------------

Drush Deploy uses drush aliases to keep the configuration specific of each site, so you can keep the alias on a version control system and customize the different environments.

Here is a sample of an alias:

    $aliases['local'] = array(
      'uri' => 'sample.local',
      'root' => '/var/www/sample/current',
      'remote-user' => 'deploy',
      'make file' => '/opt/makes/sample/local.make',
      'files folder' => '/var/www/sample/files',
      'backup folder' => '/home/sample/.drush/backups',
      'databases' => array(
        'default' => array(
          'default' => array(
            'driver' => 'mysql',
            'username' => 'drupal',
            'password' => 'drupal',
            'host' => 'localhost',
            'database' => 'drupal',
          ),
        ),
      ),
    );

