<?php
/**
 * @file
 * Define Linkit entity plugin.
 */
class LinkitPluginEntity extends LinkitPlugin {
  /**
   * Entity field query instance.
   *
   * @var Resource
   */
  var $query;

  /**
   * The entity info array of an entity type.
   *
   * @var array
   */
  var $entity_info;

  /**
   * The name of the property that contains the entity label.
   *
   * @var string
   */
  var $entity_field_label;

  /**
   * The name of the property of the bundle object that contains the name of
   * the bundle object.
   *
   * @var string
   */
  var $entity_key_bundle;

  /**
   * Plugin specific settings.
   *
   * @var arrayu
   */
  var $conf;

  /**
   * Initialize this plugin with the plugin, profile, and entity specific
   * variables.
   *
   * @param object $profile
   *   The Linkit profile to use.
   * @param array $plugin
   *   The plugin array.
   */
  function __construct($plugin, $profile) {
    parent::__construct($plugin, $profile);

    // Load the corresponding entity.
    $this->entity_info = entity_get_info($this->plugin['entity_type']);

    // Set bundle key name.
    if (isset($this->entity_info['entity keys']['bundle'])) {
      $this->entity_key_bundle = $this->entity_info['entity keys']['bundle'];
    }

    // Set the label field name.
    if (!isset($this->entity_field_label)) {
      $this->entity_field_label = $this->entity_info['entity keys']['label'];
    }

    // Make a shortcut for the profile data settings for this plugin.
    $this->conf = isset($this->profile->data[$this->plugin['name']]) ?
            $this->profile->data[$this->plugin['name']] : array();
  }

  /**
   * Build the label that will be used in the search result for each row.
   */
  function buildLabel($entity) {
    return entity_label($this->plugin['entity_type'], $entity);
  }

  /**
   * Build an URL based in the path and the options.
   */
  function buildPath($entity, $options = array()) {
    // Create the URI for the entity.
    $uri = entity_uri($this->plugin['entity_type'], $entity);

    // We have to set alias to TRUE as we don't want an alias back.
    $options += array('alias' => TRUE);

    return parent::buildPath($uri['path'], $options);
  }

  /**
   * Build the search row description.
   *
   * If there is a "result_description", run it thro token_replace.
   *
   * @param object $data
   *   An entity object that will be used in the token_place function.
   *
   * @see token_replace()
   */
  function buildDescription($data) {
    return token_replace(check_plain($this->conf['result_description']), array(
      $this->plugin['entity_type'] => $data,
    ));
  }

  /**
   * When "group_by_bundle" is active, we need to add the bundle name to the
   * group, else just return the entity label.
   *
   * @return a string with the group name.
   */
  function buildGroup($entity) {
    // Get the entity label.
    $group = $this->entity_info['label'];

    // If the entities by this entity should be grouped by bundle, get the
    // name and append it to the group.
    if (isset($this->conf['group_by_bundle']) && $this->conf['group_by_bundle'] && $this->conf['bundles']) {
      $bundles = $this->entity_info['bundles'];
      $bundle_name = $bundles[$entity->{$this->entity_key_bundle}]['label'];
      $group .= ' · ' . check_plain($bundle_name);
    }
    return $group;
  }

  /**
   * Start a new EntityFieldQuery instance.
   */
  function getQueryInstance() {
    $this->query = new EntityFieldQuery();
    $this->query->entityCondition('entity_type', $this->plugin['entity_type']);

    // Add the default sort on the enity label.
    $this->query->propertyOrderBy($this->entity_field_label, 'ASC');
  }

  /**
   * The autocomplete callback function for the Linkit Entity plugin.
   *
   * @return
   *   An associative array whose values are an
   *   associative array containing:
   *   - title: A string to use as the search result label.
   *   - description: (optional) A string with additional information about the
   *     result item.
   *   - path: The URL to the item.
   *   - group: (optional) A string with the group name for the result item.
   *     Best practice is to use the plugin name as group name.
   *   - addClass: (optional) A string with classes to add to the result row.
   */
  function autocomplete_callback() {
    $matches = array();
    // Get the EntityFieldQuery instance.
    $this->getQueryInstance();

    // Add the search condition to the query object.
    $this->query->propertyCondition($this->entity_field_label,
            '%' . db_like($this->serach_string) . '%', 'LIKE')
        ->addTag('linkit_entity_autocomplete')
        ->addTag('linkit_' . $this->plugin['name'] . '_autocomplete');

    // Bundle check.
    if (isset($this->entity_key_bundle) && isset($this->conf['bundles']) ) {
      if ($bundles = array_filter($this->conf['bundles'])) {
        $this->query->propertyCondition($this->entity_key_bundle, $bundles, 'IN');
      }
    }

    // Execute the query.
    $result = $this->query->execute();

    if (!isset($result[$this->plugin['entity_type']])) {
      return array();
    }

    $ids = array_keys($result[$this->plugin['entity_type']]);

    // Load all the entities with all the ids we got.
    $entities = entity_load($this->plugin['entity_type'], $ids);

    foreach ($entities AS $entity) {
      // If we have the entity module enabled, we check the access againt the
      // definded entity access callback.
      if (module_exists('entity') && !entity_access('view', $this->plugin['entity_type'], $entity)) {
       // continue;
      }

      $matches[] = array(
        'title' => $this->buildLabel($entity),
        'description' => $this->buildDescription($entity),
        'path' => $this->buildPath($entity),
        'group' => $this->buildGroup($entity),
        'addClass' => $this->buildRowClass($entity),
      );

    }
    return $matches;
  }

  /**
   * Generate a settings form for this handler.
   * Uses the standard Drupal FAPI.
   * The element will be attached to the "data" key.
   *
   * @return
   *   An array containing any custom form elements to be displayed in the
   *   profile editing form
   */
  function buildSettingsForm() {
    $form[$this->plugin['name']] = array(
      '#type' => 'fieldset',
      '#title' => t('!type plugin settings', array('!type' => $this->ui_title())),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
      '#tree' => TRUE,
      '#states' => array(
        'invisible' => array(
          'input[name="data[plugins][' . $this->plugin['name'] . '][enabled]"]' => array('checked' => FALSE),
        ),
      ),
    );

    // Get supported tokens for the entity type.
    $tokens = linkit_extract_tokens($this->plugin['entity_type']);

    // A short description in within the serach result for each row.
    $form[$this->plugin['name']]['result_description'] = array(
      '#title' => t('Result format'),
      '#type' => 'textfield',
      '#default_value' => isset($this->conf['result_description']) ? $this->conf['result_description'] : '',
      '#size' => 120,
      '#maxlength' => 255,
      '#description' => t('Available tokens: %tokens.', array('%tokens' => implode(', ', $tokens))),
    );

    // If the token module is installed, lets make some fancy stuff with the
    // token chooser.
    if (module_exists('token')) {
      // Unset the regular description if token module is enabled.
      unset($form[$this->plugin['name']]['result_description']['#description']);

      // Display the user documentation of placeholders.
      $form[$this->plugin['name']]['token_help'] = array(
        '#title' => t('Replacement patterns'),
        '#type' => 'fieldset',
        '#collapsible' => TRUE,
        '#collapsed' => TRUE,
      );
      $form[$this->plugin['name']]['token_help']['help'] = array(
        '#theme' => 'token_tree',
        '#token_types' => array($this->plugin['entity_type']),
      );
    }

    // If there is bundles, add some default settings features.
    if (count($this->entity_info['bundles']) > 1) {
      $bundles = array();
      // Extract the bundle data.
      foreach ($this->entity_info['bundles'] as $bundle_name => $bundle) {
        $bundles[$bundle_name] = $bundle['label'];
      }

      // Filter the possible bundles to use if the entity has bundles.
      $form[$this->plugin['name']]['bundles'] = array(
        '#title' => t('Type filter'),
        '#type' => 'checkboxes',
        '#options' => $bundles,
        '#default_value' => isset($this->conf['bundles']) ? $this->conf['bundles'] : array(),
        '#description' => t('If left blank, all types will appear in autocomplete results.'),
      );

      // Group the results with this bundle.
      $form[$this->plugin['name']]['group_by_bundle'] = array(
        '#title' => t('Group by bundle'),
        '#type' => 'checkbox',
        '#default_value' => isset($this->conf['group_by_bundle']) ? $this->conf['group_by_bundle'] : 0,
      );
    }
    return $form;
  }
}