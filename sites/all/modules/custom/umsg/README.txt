README.txt
____________________


DESCRIPTION
____________________

The User Message module is designed for sending and receiving messages.

PERMISSIONS
____________________

Administer umsg
Read all user messages
Read own user messages
Write new user messages
Delete own user messages


INSTALLATION
____________________

To install this module, do the following:

1. Configure message database in settings.php

ex:
$databases['msgdb']['default'] = array(
  'database' => 'message2',
  'username' => 'root',
  'password' => '',
  'host' => 'localhost',
  'port' => '',
  'driver' => 'mysql',
  'prefix' => '',
);

2. Edit constants for db identifier in umsg.install.

3. Attention. If you generate dummy content, all the users will be deleted (unless admin)