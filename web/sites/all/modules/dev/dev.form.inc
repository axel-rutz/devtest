<?php

function dev_settings_form($form, &$form_state) {
  form_load_include($form_state, 'inc', 'dev', 'dev.form');

  // Prepare data.
  $ssh_pubkey = dev_get_ssh_pubkey();
  $has_git = $ssh_pubkey ? dev_check_git() : FALSE;
  $git_diff = $has_git ? dev_get_git_diff() : FALSE;

  $remote_url = variable_get('dev_git_remote_url');
  $git_local_path = variable_get('dev_git_local_path');
  $has_settings = $remote_url && $git_local_path;

  // Git settings: remote url, local path, relative drupal root path
  $form['settings']['dev_enable'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable'),
    '#description' => t('This can be locked to off in settings.php if needed.'),
    '#default_value' => variable_get('dev_enable'),
  );
  $form['settings']['dev_git_remote_url'] = array(
    '#type' => 'textfield',
    '#title' => t('Git remote URL'),
    '#default_value' => $remote_url
  );
  $form['settings']['dev_git_local_path'] = array(
    '#type' => 'textfield',
    '#title' => t('Git local repository path'),
    '#default_value' => $git_local_path
  );
  $form['settings']['dev_relative_drupal_root_path'] = array(
    '#type' => 'textfield',
    '#title' => t('Drupal root relative to local repository'),
    '#default_value' => variable_get('dev_relative_drupal_root_path'),
  );
  $form['settings']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save configuration'),
    '#submit' => array('dev_settings_form_submit_settings'),
  );

  // Generate or show pubkey to make authorized key.
  if ($ssh_pubkey) {
    $form['ssh']['show'] = array(
      '#title' => t('SSH public key'),
      '#type' => 'item',
      '#markup' => check_plain($ssh_pubkey),
    );
  }
  else {
    $form['ssh']['generate'] = array(
      '#title' => t('Generate SSH key'),
      '#type' => 'submit',
      '#submit' => array('dev_settings_ssh_generate'),
    );
  }

  // Clone git repo.
  $form['clone']['git_clone'] = array(
    '#title' => t('Git clone'),
    '#type' => 'submit',
    '#submit' => array('dev_settings_git_clone'),
  );

  // Commit.
  $form['commit']['git_commit_message'] = array(
    '#type' => 'textfield',
    '#title' => t('Commit message'),
  );
  $form['commit']['git_commit'] = array(
    '#title' => t('Commit changes'),
    '#type' => 'submit',
    '#submit' => array('dev_settings_git_commit'),
  );
  $form['commit']['diff'] = array(
    '#title' => t('Diff'),
    '#type' => 'item',
    '#markup' => check_plain($git_diff),
  );

  // Reset.
  $form['reset']['git_reset'] = array(
    '#title' => t('Reset uncommitted changes'),
    '#type' => 'submit',
    '#submit' => array('dev_settings_git_reset'),
  );

  // Push.
  $form['push']['git_push'] = array(
    '#title' => t('Git push'),
    '#type' => 'submit',
    '#submit' => array('dev_settings_git_push'),
  );

  // Vertical tabs.
  if (!$has_settings) {
    $default_tab = 'settings';
  }
  elseif (!$ssh_pubkey) {
    $default_tab = 'ssh';
  }
  elseif (!$has_git) {
    $default_tab = 'clone';
  }
  elseif ($git_diff) {
    $default_tab = 'commit';
  }
  else {
    $default_tab = 'push';
  }

  foreach (array('settings', 'ssh', 'clone', 'commit', 'reset', 'push') as $key) {
    $form[$key]['#type'] = 'fieldset';
    $form[$key]['#group'] = 'vtabs';
  }
  $form['vtabs'] = array(
    '#type' => 'vertical_tabs',
    '#default_tab' => $default_tab,
  );

  return $form;
}

function dev_settings_form_submit_settings($form, &$form_state) {
  // Save settings, @see system_settings_form_submit().
  foreach ($form_state['values']['settigns'] as $key => $value) {
    variable_set($key, $value);
  }
}

// @todo All the shell stuff should be robustified.

/**
 * @return int
 */
function dev_check_git() {
  $return = dev_dir_exec(function() {
    exec('git rev-parse', $output, $return);
    return $return;
  });
  return $return;
}

/**
 * @return string
 */
function dev_get_git_diff() {
  $return = dev_dir_exec(function() {
    exec('git diff', $output, $return);
    return $output;
  });
  return $return;
}

/**
 * @return string
 */
function dev_get_ssh_pubkey() {
  exec('echo $HOME', $output, $return);
  $home = trim($output);
  if ($ssh_pubkey = file_get_contents("$home/.ssh/id_rsa.pub")) {
    return $ssh_pubkey;
  }
}

/**
 * Form callback: Generate ssh kea.
 */
function dev_settings_ssh_generate($form, &$form_state) {
  exec('ssh-keygen -t rsa', $output, $return);
  drupal_set_message(t('Command output: <br><pre>@output</pre>', array('@output' => $output)));
}

/**
 * Form callback: Git clone.
 */
function dev_settings_git_clone($form, &$form_state) {
  $remote_url = variable_get('dev_git_remote_url');
  $local_path = variable_get('dev_git_local_path');
  exec('git clone ' . escapeshellarg($remote_url) . escapeshellarg($local_path), $output, $return);
  drupal_set_message(t('Command output: <br><pre>@output</pre>', array('@output' => $output)));

  system_rebuild_module_data();
  system_rebuild_theme_data();
}

/**
 * Form callback: Git commit.
 */
function dev_settings_git_commit($form, &$form_state) {
  $message = $form_state['git_commit_message'];
  if (!$message) {
    $message = 'Some hacking';
  }
  dev_dir_exec(function() use($message) {
    exec('git commit -m ' . escapeshellarg($message), $output, $return);
    drupal_set_message(t('Command output: <br><pre>@output</pre>', array('@output' => $output)));
  });
}

/**
 * Form callback: Git reset.
 */
function dev_settings_git_reset($form, &$form_state) {
  dev_dir_exec(function() {
    exec('git reset --hard', $output, $return);
    drupal_set_message(t('Command output: <br><pre>@output</pre>', array('@output' => $output)));
  });
}

/**
 * Form callback: Git push.
 */
function dev_settings_git_push($form, &$form_state) {
  dev_dir_exec(function() {
    exec('git push', $output, $return);
    drupal_set_message(t('Command output: <br><pre>@output</pre>', array('@output' => $output)));
  });
}

/**
 * Execute function in local dir if set.
 */
function dev_dir_exec(Callable $function) {
  $old_dir = getcwd();
  $dir = variable_get('dev_git_local_path');
  if ($dir && is_dir($dir)) {
    chdir($dir);
    $return = $function();
    chdir($old_dir);
    return $return;
  }
  else {
    drupal_set_message(t('Command not executed. Local path is not a valid diractory.'), 'error');
  }
}
