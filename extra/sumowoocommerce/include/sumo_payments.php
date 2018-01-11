<?php

/* 
 * Main Gateway of Sumo using a daemon online 
 * Authors: Serhack and cryptochangements
 */


class Sumo_Gateway extends WC_Payment_Gateway
{
    private $reloadTime = 30000;
    private $discount;
    private $confirmed = false;
    public $sumo_daemon;

    function __construct()
    {
        $this->id = "sumo_gateway";
        $this->method_title = __("Sumokoin Gateway", 'sumo_gateway');
        $this->method_description = __("Sumokoin Payment Gateway plugin for WooCommerce. You can find more information about this payment gateway on our website. You'll need a daemon online for your address.", 'sumo_gateway');
        $this->title = __("Sumokoin Gateway", 'sumo_gateway');
        $this->version = "0.3";
        //
        $this->icon = apply_filters('woocommerce_offline_icon', '');
        $this->has_fields = false;

        $this->log = new WC_Logger();

        $this->init_form_fields();
        $this->host = $this->get_option('daemon_host');
        $this->port = $this->get_option('daemon_port');
        $this->address = $this->get_option('sumo_address');
        $this->username = $this->get_option('username');
        $this->password = $this->get_option('password');
        $this->discount = $this->get_option('discount');

        // After init_settings() is called, you can get the settings and load them into variables, e.g:
        // $this->title = $this->get_option('title' );
        $this->init_settings();

        // Turn these settings into variables we can use
        foreach ($this->settings as $setting_key => $value) {
            $this->$setting_key = $value;
        }

        add_action('admin_notices', array($this, 'do_ssl_check'));
        add_action('admin_notices', array($this, 'validate_fields'));
        add_action('woocommerce_thankyou_' . $this->id, array($this, 'instruction'));
        if (is_admin()) {
            /* Save Settings */
            add_action('woocommerce_update_options_payment_gateways_' . $this->id, array($this, 'process_admin_options'));
            add_filter('woocommerce_currencies', [$this, 'add_my_currency']);
            add_filter('woocommerce_currency_symbol', [$this, 'add_my_currency_symbol'], 10, 2);
            add_action('woocommerce_email_before_order_table', array($this, 'email_instructions'), 10, 2);
        }
        $this->sumo_daemon = new Sumo_Library($this->host . ':' . $this->port . '/json_rpc', $this->username, $this->password);
    }

    public function init_form_fields()
    {
        $this->form_fields = array(
            'enabled' => array(
                'title' => __('Enable / Disable', 'sumo_gateway'),
                'label' => __('Enable this payment gateway', 'sumo_gateway'),
                'type' => 'checkbox',
                'default' => 'no'
            ),

            'title' => array(
                'title' => __('Title', 'sumo_gateway'),
                'type' => 'text',
                'desc_tip' => __('Payment title the customer will see during the checkout process.', 'sumo_gateway'),
                'default' => __('Sumokoin Payment', 'sumo_gateway')
            ),
            'description' => array(
                'title' => __('Description', 'sumo_gateway'),
                'type' => 'textarea',
                'desc_tip' => __('Payment description the customer will see during the checkout process.', 'sumo_gateway'),
                'default' => __('Pay securely using Sumo.', 'sumo_gateway')

            ),
            'sumo_address' => array(
                'title' => __('Sumokoin Address', 'sumo_gateway'),
                'label' => __('Useful for people that have not a daemon online'),
                'type' => 'text',
                'desc_tip' => __('Sumo Wallet Address', 'sumo_gateway')
            ),
            'daemon_host' => array(
                'title' => __('Wallet RPC Host/IP', 'sumo_gateway'),
                'type' => 'text',
                'desc_tip' => __('This is the Daemon Host/IP to authorize the payment with port', 'sumo_gateway'),
                'default' => 'localhost',
            ),
            'daemon_port' => array(
                'title' => __('Wallet RPC port', 'sumo_gateway'),
                'type' => 'text',
                'desc_tip' => __('This is the Daemon Host/IP to authorize the payment with port', 'sumo_gateway'),
                'default' => '19733',
            ),
            'username' => array(
                'title' => __('Sumo Wallet username', 'sumo_gateway'),
                'desc_tip' => __('This is the username that you used with your sumo wallet-rpc', 'sumo_gateway'),
                'description' => __('You can leave this field empty if you did not set any username', 'sumo_gateway'),
                'type' => __('text'),
                'default' => __('username', 'sumo_gateway'),

            ),
            'password' => array(
                'title' => __('Sumo wallet RPC password', 'sumo_gateway'),
                'desc_tip' => __('This is the password that you used to secure your sumo wallet-rpc', 'sumo_gateway'),
                'description' => __('You can leave this field empty if you did not set any password', 'sumo_gateway'),
                'type' => __('text'),
                'default' => ''

            ),
            'discount' => array(
                'title' => __('% discount for using SUMO', 'sumo_gateway'),

                'desc_tip' => __('Provide a discount to your customers for making a private payment with SUMO!', 'sumo_gateway'),
                'description' => __('Do you want to spread the word about Sumokoin? Offer a small discount! Leave this empty if you do not wish to provide a discount', 'sumo_gateway'),
                'type' => __('text'),
                'default' => '5%'

            ),
            'environment' => array(
                'title' => __(' Test Mode', 'sumo_gateway'),
                'label' => __('Enable Test Mode', 'sumo_gateway'),
                'type' => 'checkbox',
                'description' => __('Check this box if you are using testnet', 'sumo_gateway'),
                'default' => 'no'
            ),
            'onion_service' => array(
                'title' => __(' Onion Service', 'sumo_gateway'),
                'label' => __('Enable Onion Service', 'sumo_gateway'),
                'type' => 'checkbox',
                'description' => __('Check this box if you are running on an Onion Service (Suppress SSL errors)', 'sumo_gateway'),
                'default' => 'no'
            ),
            'subaddress_payments' => array(
                'title' => __('Subaddress Payments', 'sumo_gateway'),
                'label' => __('Enable payments to subdaddresses', 'sumo_gateway'),
                'type' => 'checkbox',
                'description' => __('Check this box if you wish to receive payments via Subaddresses instead of Payment IDs', 'sumo_gateway'),
                'default' => 'no'
            ),
        );
    }

    public function add_my_currency($currencies)
    {
        $currencies['SUMO'] = __('Sumo', 'woocommerce');
        return $currencies;
    }

    function add_my_currency_symbol($currency_symbol, $currency)
    {
        switch ($currency) {
            case 'SUMO':
                $currency_symbol = 'SUMO';
                break;
        }
        return $currency_symbol;
    }

    public function admin_options()
    {
        $this->log->add('Sumo_gateway', '[SUCCESS] Sumokoin Settings OK');

        echo "<h1>Sumo Payment Gateway</h1>";

        echo "<p>Welcome to Sumokoin payment plugin for WooCommerce. Getting started: Make a connection with Sumokoin daemon. For more info, see <a href=\"https://github.com/sumoweb/sumowp/blob/master/README.md\"> our GitHub page</a>";
        echo "<div style='border:1px solid #DDD;padding:5px 10px;font-weight:bold;color:#223079;background-color:#9ddff3;'>";
        $this->getamountinfo();
        echo "</div>";
        echo "<table class='form-table'>";
        $this->generate_settings_html();
        echo "</table>";
        echo "<h4>Secure your wallet RPC daemon! Learn more about using a password with the Sumokoin wallet RPC <a href=\"https://github.com/sumoweb/sumowp/blob/master/README.md\">here</a></h4>";
    }

    public function getamountinfo()
    {
        $wallet_amount = $this->sumo_daemon->getbalance();
        if (!isset($wallet_amount)) {
            $this->log->add('Sumo_gateway', '[ERROR] No connection with daemon');
            $wallet_amount['balance'] = "0";
            $wallet_amount['unlocked_balance'] = "0";
        }
        else {
            $real_wallet_amount = $wallet_amount['balance'] / 1000000000;
            $real_amount_rounded = round($real_wallet_amount, 6);
    
            $unlocked_wallet_amount = $wallet_amount['unlocked_balance'] / 1000000000;
            $unlocked_amount_rounded = round($unlocked_wallet_amount, 6);
    
            echo "Your balance is: " . $real_amount_rounded . " SUMO </br>";
            echo "Unlocked balance: " . $unlocked_amount_rounded . " SUMO </br>";
        }
    }

    public function process_payment($order_id)
    {
        $order = wc_get_order($order_id);
        $order->update_status('on-hold', __('Awaiting offline payment', 'sumo_gateway'));
        // Reduce stock levels
        $order->reduce_order_stock();

        // Remove cart
        WC()->cart->empty_cart();

        // Return thank you redirect
        return array(
            'result' => 'success',
            'redirect' => $this->get_return_url($order)
        );

    }

    // Submit payment and handle response

    public function validate_fields()
    {
        if ($this->check_sumo() != TRUE) {
            echo "<div class=\"error\"><p>Your Sumokoin address doesn't seem to be valid. Check that you've inserted it correctly.</p></div>";
        }

    }


    // Validate fields

    public function check_sumo()
    {
        require_once __DIR__ . '/cryptonote.php';
        $addr = $this->settings['sumo_address'];
        if (function_exists('bcadd'))
        {
            $prefixes = $this->settings['environment'] == "no" ? 
                ["9ae7ae", "9ae3"] : 
                ["9aeadd", "9aea"];
            return Cryptonote::VerifyAddress($addr, $prefixes);
        }
        if ($this->settings['environment'] == "no")
        {
            switch(strlen($addr))
            {
                case 99: return substr($addr, 0, 4) == "Sumo";
                case 98: return substr($addr, 0, 4) == "Subo";
                default: return false;
            }
        }
        else
        {
            switch(strlen($addr))
            {
                case 99: return substr($addr, 0, 4) == "Suto";
                case 98: return substr($addr, 0, 4) == "Susu";
                default: return false;
            }
        }
    }

    public function instruction($order_id)
    {
        $order = wc_get_order($order_id);
        $amount = floatval(preg_replace('#[^\d.]#', '', $order->get_total()));
        $payment = $this->get_payment($order_id);
        $currency = $order->get_currency();
        $amount_sumo2 = $this->changeto($amount, $currency, $order_id);
        if ($amount_sumo2 <= 0)
        {
            echo "ERROR: Temporarily unable to get exchange rate<br/>";
            echo "
             <script type='text/javascript'>setTimeout(function () { location.reload(true); }, $this->reloadTime);</script>";
            return;
        }
        
        $uri = $payment->get_uri($amount_sumo2);
        $this->confirmed = $payment->verify($order, $amount_sumo2);
        $message = "We are waiting for your payment to be confirmed";
        
        if ($this->confirmed) {
            $message = "Payment has been received and confirmed. Thanks!";
            $this->log->add('Sumo_gateway', '[SUCCESS] Payment has been recorded. Congratulations!');
            $order = wc_get_order($order_id);
            $order->update_status('completed', __('Payment has been received', 'sumo_gateway'));
            $this->reloadTime = 3000000000000; // Greatly increase the reload time as it is no longer needed
            
            $color = "006400";
        } else {
            $color = "DC143C";
        }
        echo "<h4><font color=$color>" . $message . "</font></h4>";
        echo "
        <head>
        <!--Import Google Icon Font-->
        <link href='https://fonts.googleapis.com/icon?family=Material+Icons' rel='stylesheet'>
        <link href='https://fonts.googleapis.com/css?family=Montserrat:400,800' rel='stylesheet'>
        <link href='".plugins_url('/../assets/style.css', __FILE__)."' rel='stylesheet'>
        <!--Let browser know website is optimized for mobile-->
            <meta name='viewport' content='width=device-width, initial-scale=1.0'/>
            </head>
            <body>
            <!-- page container  -->
            <div class='page-container'>
            <!-- sumo container payment box -->
            <div class='container-sumo-payment'>
            <!-- header -->
            <div class='header-sumo-payment'>
            <span class='logo-sumo'><img src='".plugins_url('/../assets/sumo-logo.png', __FILE__ )."'/></span>
            <span class='sumo-payment-text-header'><h2>SUMO PAYMENT</h2></span>
            </div>
            <!-- end header -->
            <!-- sumo content box -->
            <div class='content-xmr-payment'>
            <div class='sumo-amount-send'>
            <span class='sumo-label'>Send:</span>
            <div class='sumo-amount-box'>".$amount_sumo2."</div><div class='sumo-box'>SUMO</div>
            </div>
            <div class='sumo-address'>
            <span class='sumo-label'>To this address:</span>
            <div class='sumo-address-box'>".$payment->get_address()."</div>
            </div>
            <div class='sumo-qr-code'>
            <span class='sumo-label'>Or scan QR:</span>
            <div class='sumo-qr-code-box'><img src='https://api.qrserver.com/v1/create-qr-code/? size=200x200&data=".$uri."' /></div>
            </div>
            <div class='clear'></div>
            </div>
            <!-- end content box -->
            <!-- footer sumo payment -->
            <div class='footer-sumo-payment'>
            <a href='https://www.sumokoin.org' target='_blank'>Help</a> | <a href='https://www.sumokoin.org' target='_blank'>About Sumo</a>
            </div>
            <!-- end footer sumo payment -->
            </div>
            <!-- end sumo container payment box -->
            </div>
            <!-- end page container  -->
            </body>
        ";
	    
	    
	    
        echo "
      <script type='text/javascript'>setTimeout(function () { location.reload(true); }, $this->reloadTime);</script>";
    }

    private function get_payment($order_id)
    {
        require_once __DIR__ . '/sumo_payment.php';
        return new Sumo_Payment($this, $order_id);
    }

    public function changeto($amount, $currency, $order_id)
    {
        global $wpdb;
        // This will create a table named whatever the payment id is inside the database "WordPress"
        $create_table = "
            CREATE TABLE IF NOT EXISTS {$wpdb->prefix}sumo_order_rates (
                order_id char(16) UNIQUE PRIMARY KEY,
                currency char(3) NOT NULL,
                rate DECIMAL(10,4) NOT NULL,
                timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            )
        ";
        
        $wpdb->query($create_table);
        $sql = $wpdb->prepare("
            SELECT count(*) as count 
            FROM {$wpdb->prefix}sumo_order_rates
            WHERE order_id = %s
        ", [$order_id]);
        $rows_num = $wpdb->get_results($sql);
        if ($rows_num[0]->count > 0) // Checks if the row has already been created or not
        {
            $sql = $wpdb->prepare("
                SELECT rate 
                FROM {$wpdb->prefix}sumo_order_rates
                WHERE order_id = %s
            ", [$order_id]);
            $stored_rate = $wpdb->get_results($sql);

            $stored_rate_transformed = $stored_rate[0]->rate; //this will turn the stored rate back into a decimaled number

            if (isset($this->discount)) {
                $discount_decimal = $this->discount / 100;
                $new_amount = $amount / $stored_rate_transformed;
                $discount = $new_amount * $discount_decimal;
                $final_amount = $new_amount - $discount;
                $rounded_amount = round($final_amount, 9);
            } else {
                $new_amount = $amount / $stored_rate_transformed;
                $rounded_amount = round($new_amount, 9); //the sumokoin wallet can't handle decimals smaller than 0.000000000001
            }
        } else // If the row has not been created then the live exchange rate will be grabbed and stored
        {
            $sumo_live_price = $this->retrieveprice($currency);
            if ($sumo_live_price == -1) return -1;
            $new_amount = $amount / $sumo_live_price;
            $rounded_amount = round($new_amount, 9);
            
            $sql = $wpdb->prepare("
                INSERT INTO {$wpdb->prefix}sumo_order_rates (order_id, currency, rate)
                VALUES (%s, %s, %f)
            ", [$order_id, $currency, $sumo_live_price]);
            $wpdb->query($sql);
        }

        return $rounded_amount;
    }


    // Check if we are forcing SSL on checkout pages
    // Custom function not required by the Gateway

    public function retrieveprice($currency)
    {
        $currencies = ['USD', 'EUR', 'CAD', 'GBP', 'INR', 'SUMO'];
        if (!in_array($currency, $currencies)) $currencies[] = $currency;
        
        $sumo_price = file_get_contents('https://min-api.cryptocompare.com/data/price?fsym=SUMO&tsyms='.implode(",", $currencies).'&extraParams=sumo_woocommerce');
        $price = json_decode($sumo_price, TRUE);
        if (!isset($price)) {
            $this->log->add('Sumo_Gateway', '[ERROR] Unable to get the price of SUMO');
            return -1;
        }
        
        if (!isset($price[$currency])) {
            $this->log->add('Sumo_Gateway', '[ERROR] Unable to retrieve SUMO in currency '.$currency);
            return -1;
        }
        
        return $price[$currency];
    }

    public function do_ssl_check()
    {
        if ($this->enabled == "yes" && !$this->get_option('onion_service')) {
            if (get_option('woocommerce_force_ssl_checkout') == "no") {
                echo "<div class=\"error\"><p>" . sprintf(__("<strong>%s</strong> is enabled and WooCommerce is not forcing the SSL certificate on your checkout page. Please ensure that you have a valid SSL certificate and that you are <a href=\"%s\">forcing the checkout pages to be secured.</a>"), $this->method_title, admin_url('admin.php?page=wc-settings&tab=checkout')) . "</p></div>";
            }
        }
    }

    public function connect_daemon()
    {
        $host = $this->settings['daemon_host'];
        $port = $this->settings['daemon_port'];
        $sumo_library = new Sumo($host, $port);
        if ($sumo_library->works() == true) {
            echo "<div class=\"notice notice-success is-dismissible\"><p>Everything works! Congratulations and welcome to Sumo. <button type=\"button\" class=\"notice-dismiss\">
						<span class=\"screen-reader-text\">Dismiss this notice.</span>
						</button></p></div>";

        } else {
            $this->log->add('Sumo_gateway', '[ERROR] Plugin can not reach wallet rpc.');
            echo "<div class=\" notice notice-error\"><p>Error connecting with daemon, see documentation for more info</p></div>";
        }
    }
}
