<?php
/**
 * @file
 * vhost drush command.
 *
 *  You can copy this file to any of the following
 *    1. A .drush folder in your HOME folder.
 *    2. Anywhere in a folder tree below an active module on your site.
 *    3. /usr/share/drush/commands (configurable)
 *    4. In an arbitrary folder specified with the --include option.
 *    5. Drupal's /drush or /sites/all/drush folders.
 */

/**
 * This is the virtual hosts template file for the apache functions.
 */
define('VHOST_APACHE_TEMPLATE', "
<Directory #CODEPATH#>

  Options FollowSymLinks
  AllowOverride None

  # Protect files and directories from prying eyes.
  <FilesMatch \"\.(engine|inc|info|install|make|module|profile|test|po|sh|.*sql|theme|tpl(\.php)?|xtmpl)$|^(\..*|Entries.*|Repository|Root|Tag|Template)$\">
    Order allow,deny
  </FilesMatch>

  # Hide these from public view.
  <FilesMatch \"(^LICENSE|CHANGELOG|MAINTAINERS|INSTALL|UPGRADE|API|README).*\.txt$\">
    Order deny,allow
    Deny from all 
  </FilesMatch>

  RewriteEngine on
  RewriteRule \"(^|/)\.\" - [F]
  RewriteCond %{REQUEST_FILENAME} !-f
  RewriteCond %{REQUEST_FILENAME} !-d
  RewriteCond %{REQUEST_URI} !=/favicon.ico
  RewriteRule ^ index.php [L]

  RewriteCond %{HTTP:Accept-encoding} gzip
  RewriteCond %{REQUEST_FILENAME}\.gz -s
  RewriteRule ^(.*)\.css $1\.css\.gz [QSA]

  # Serve gzip compressed JS files if they exist and the client accepts gzip.
  RewriteCond %{HTTP:Accept-encoding} gzip
  RewriteCond %{REQUEST_FILENAME}\.gz -s
  RewriteRule ^(.*)\.js $1\.js\.gz [QSA]

  # Serve correct content types, and prevent mod_deflate double gzip.
  RewriteRule \.css\.gz$ - [T=text/css,E=no-gzip:1]
  RewriteRule \.js\.gz$ - [T=text/javascript,E=no-gzip:1]

  <FilesMatch \"(\.js\.gz|\.css\.gz)$\">
    # Serve correct encoding type.
    Header set Content-Encoding gzip
    # Force proxies to cache gzipped & non-gzipped css/js files separately.
    Header append Vary Accept-Encoding
  </FilesMatch>

</Directory>

<VirtualHost *:80>
  ServerName #DOMAIN#
  DocumentRoot #CODEPATH#
  LogLevel warn
  ServerSignature Off
</VirtualHost>
");

/**
 * Implements hook_drush_command().
 */
function vhost_drush_command() {

  $items = array();

  $items['vhost'] = array(
    'description' => "Create a vhost for your drupal site.",
    'arguments' => array(
      'root' => 'Path to Drupal site',
      'uri' => 'Site URL',
    ),
    'options' => array(
      'os' => array(
        'description' => dt('Operating system. Defaults to debian.'),
      ),
      'vhost file' => array(
        'description' => dt('Virtual host file to use instead of default.'),
      ),
    ),
    'examples' => array(
      'drush vhost /var/www/drupal http://example.local' => '',
    ),
    // No bootstrap at all.
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
  );

  // Add options provided by Drush extensions extending buildmanager.
  $build_options = drush_command_invoke_all('vhost');
  $items['vhost']['options'] = array_merge(
    $items['vhost']['options'],
    $build_options
  );

  $items['vhost-destroy'] = array(
    'description' => "Find any make files in the repo.",
    'arguments' => array(
      'uri' => 'Site URL',
    ),
    // No bootstrap at all.
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
  );

  return $items;
}

/**
 * Callback for buildmanager-build command.
 */
function drush_vhost() {
  $root = drush_get_option('root');
  $uri = drush_get_option('uri');
  vhost_create_webserver_config($uri, $root);
}

/**
 * Destroy the DNS.  Edit /etc/hosts file on local machine.
 */
function vhost_destroy_dns($uri) {
  drush_print_r('sudo sed -i \"/ $domain .*#/d\" /etc/hosts');
  drush_shell_exec("sudo sed -i \"/ $uri .*#/d\" /etc/hosts");
}

/**
 * Configure apache virtualhost file using the template.
 */
function vhost_create_webserver_config($uri, $root, $os = 'Linux') {
  if (file_exists("/etc/apache2/sites-available/$uri")) {
    if (drush_confirm(dt('Vhost ' .  $uri . ' already exists. Do you want to keep its contents?'))) {
      return;
    }
  }

  $str_config = VHOST_APACHE_TEMPLATE;

  drush_log("Creating apache config (/etc/apache2/sites-enabled/$uri)", 'ok');

  $str_config = str_replace("#DOMAIN#", $uri, $str_config);
  $str_config = str_replace("#CODEPATH#", $root, $str_config);

  $config_handle = fopen("/etc/apache2/sites-available/$uri", "w+");
  fwrite($config_handle, $str_config);

  if ($os == 'Linux') {
    drush_shell_exec("sudo a2ensite $uri");
  }

  // Restart apache.
  drush_shell_exec("sudo /usr/sbin/apachectl graceful");

  drush_log('... done.', 'ok');
}
