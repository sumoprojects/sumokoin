<?php
/*
Plugin Name: Sumokoin - WooCommerce Gateway
Plugin URI: http://www.sumokoin.org
Description: Extends WooCommerce by Adding the Sumo Gateway
Version: 1.1
Author: Phillip Whelan
Author URI: http://github.com/pwhelan
*/

// This code isn't for Dark Net Markets, please report them to Authority!
if (!defined('ABSPATH')) {
    exit; // Exit if accessed directly
}
// Include our Gateway Class and register Payment Gateway with WooCommerce
add_action('plugins_loaded', 'sumo_init', 0);
function sumo_init()
{
    /* If the class doesn't exist (== WooCommerce isn't installed), return NULL */
    if (!class_exists('WC_Payment_Gateway')) return;


    /* If we made it this far, then include our Gateway Class */
    include_once('include/sumo_payments.php');
    require_once('library.php');

    // Lets add it too WooCommerce
    add_filter('woocommerce_payment_gateways', 'sumo_gateway');
    function sumo_gateway($methods)
    {
        $methods[] = 'Sumo_Gateway';
        return $methods;
    }
}

/*
 * Add custom link
 * The url will be http://yourworpress/wp-admin/admin.php?=wc-settings&tab=checkout
 */
add_filter('plugin_action_links_' . plugin_basename(__FILE__), 'sumo_payment');
function sumo_payment($links)
{
    $plugin_links = array(
        '<a href="' . admin_url('admin.php?page=wc-settings&tab=checkout') . '">' . __('Settings', 'sumo_payment') . '</a>',
    );

    return array_merge($plugin_links, $links);
}

add_action('admin_menu', 'sumo_create_menu');
function sumo_create_menu()
{
    add_menu_page(
        __('Sumokoin', 'textdomain'),
        'Sumokoin',
        'manage_options',
        'admin.php?page=wc-settings&tab=checkout&section=sumo_gateway',
        '',
        plugins_url('/assets/sumo-icon.png', __FILE__),
        56 // Position on menu, woocommerce has 55.5, products has 55.6

    );
}


