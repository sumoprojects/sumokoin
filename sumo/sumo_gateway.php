<?php
/*
Plugin Name: Monero - WooCommerce Gateway
Plugin URI: http://monerointegrations.com
Description: Extends WooCommerce by Adding the Monero Gateway
Version: 1.0
Author: SerHack
Author URI: http://monerointegrations.com
*/

// This code isn't for Dark Net Markets, please report them to Authority!
if (!defined('ABSPATH')) {
    exit; // Exit if accessed directly
}
// Include our Gateway Class and register Payment Gateway with WooCommerce
add_action('plugins_loaded', 'monero_init', 0);
function monero_init()
{
    /* If the class doesn't exist (== WooCommerce isn't installed), return NULL */
    if (!class_exists('WC_Payment_Gateway')) return;


    /* If we made it this far, then include our Gateway Class */
    include_once('include/monero_payments.php');
    require_once('library.php');

    // Lets add it too WooCommerce
    add_filter('woocommerce_payment_gateways', 'monero_gateway');
    function monero_gateway($methods)
    {
        $methods[] = 'Monero_Gateway';
        return $methods;
    }
}

/*
 * Add custom link
 * The url will be http://yourworpress/wp-admin/admin.php?=wc-settings&tab=checkout
 */
add_filter('plugin_action_links_' . plugin_basename(__FILE__), 'monero_payment');
function monero_payment($links)
{
    $plugin_links = array(
        '<a href="' . admin_url('admin.php?page=wc-settings&tab=checkout') . '">' . __('Settings', 'monero_payment') . '</a>',
    );

    return array_merge($plugin_links, $links);
}

add_action('admin_menu', 'monero_create_menu');
function monero_create_menu()
{
    add_menu_page(
        __('Monero', 'textdomain'),
        'Monero',
        'manage_options',
        'admin.php?page=wc-settings&tab=checkout&section=monero_gateway',
        '',
        plugins_url('monero/assets/icon.png'),
        56 // Position on menu, woocommerce has 55.5, products has 55.6

    );
}


