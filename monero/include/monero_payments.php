<?php
/* Main Gateway of Monero using a daemon online */

class Monero_Gateway extends WC_Payment_Gateway
{
    private $monero_daemon;

    private function _run($method,$params = null) {
      $result = $this->monero_daemon->_run($method, $params);
       return $result;
    }
    private function _print($json){
        $json_encoded = json_encode($json,  JSON_PRETTY_PRINT | JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES);
        echo $json_encoded;
    }		
				function __construct()
				{
								
								$this->id                 = "monero_gateway";
								$this->method_title       = __("Monero GateWay", 'monero_gateway');
								$this->method_description = __("Monero Payment Gateway Plug-in for WooCommerce. You can find more information about this payment gateway in our website. WARN: You'll need a daemon online for your address.", 'monero_gateway');
								$this->title              = __("Monero Gateway", 'monero_gateway');
								//
								$this->icon               = apply_filters('woocommerce_offline_icon', '');
								$this->has_fields         = false;
								
								
								$this->init_form_fields();
								$this->host = $this->get_option('daemon_host');
								$this->port = $this->get_option('daemon_port');
								$this->address = $this->get_option('monero_address');
								
								// After init_settings() is called, you can get the settings and load them into variables, e.g:
								// $this->title = $this->get_option('title' );
								$this->init_settings();
								
								// Turn these settings into variables we can use
								foreach ($this->settings as $setting_key => $value) {
												$this->$setting_key = $value;
								}
								
								
								add_action('admin_notices', array(
												$this,
												'do_ssl_check'
								));
								add_action('admin_notices', array(
												$this,
												'validate_fields'
								));
					
								
								
								
								
       								add_action('woocommerce_thankyou_' . $this->id, array( $this, 'instruction' ) );

								if (is_admin()) {
												/* Save Settings */
												add_action('woocommerce_update_options_payment_gateways_' . $this->id, array(
																$this,
																'process_admin_options'
												));
								}
					$this->monero_daemon = new jsonRPCClient($this->host . ':' . $this->port . '/json_rpc');
				}
				
				public function admin_options()
				{
								echo "<h1>Monero Payment Gateway</h1>";
								echo "<p>Welcome to Monero Extension for WooCommerce. Getting started: Make a connection with daemon <a href='https://reddit.com/u/serhack'>Contact Me</a>";
								echo "<table class='form-table'>";
								$this->generate_settings_html();
								echo "</table>";
								
				}
				
				
				
				public function init_form_fields()
				{
								$this->form_fields = array(
												'enabled' => array(
																'title' => __('Enable / Disable', 'monero_gateway'),
																'label' => __('Enable this payment gateway', 'monero_gateway'),
																'type' => 'checkbox',
																'default' => 'no'
												),
												
												'title' => array(
																'title' => __('Title', 'monero_gateway'),
																'type' => 'text',
																'desc_tip' => __('Payment title the customer will see during the checkout process.', 'monero_gateway'),
																'default' => __('Monero XMR Payment', 'monero_gateway')
												),
												'description' => array(
																'title' => __('Description', 'monero_gateway'),
																'type' => 'textarea',
																'desc_tip' => __('Payment description the customer will see during the checkout process.', 'monero_gateway'),
																'default' => __('Pay securely using XMR.', 'monero_gateway')
																
												),
												'monero_address' => array(
																'title' => __('Monero Address', 'monero_gateway'),
																'label' => __('Useful for people that have not a daemon online'),
																'type' => 'text',
																'desc_tip' => __('Monero Wallet Address', 'monero_gateway')
												),
												        'daemon_host' => array(
												'title'     => __('Daemon Host/ IP', 'monero_gateway'),
												'type'      => 'text',
												'desc_tip'  => __('This is the Daemon Host/IP to authorize the payment with port', 'monero_gateway'),
												'default'   => 'localhost',
												),
												'daemon_port' => array(
												'title'     => __('Daemon PORT', 'monero_gateway'),
												'type'      => 'text',
												'desc_tip'  => __('This is the Daemon Host/IP to authorize the payment with port', 'monero_gateway'),
												'default'   => '18080',
												), 
												
												'environment' => array(
																'title' => __(' Test Mode', 'monero_gateway'),
																'label' => __('Enable Test Mode', 'monero_gateway'),
																'type' => 'checkbox',
																'description' => __('Place the payment gateway in test mode.', 'monero_gateway'),
																'default' => 'no'
												)
								);
				}
				
				
				public function retriveprice($currency)
				{
								$xmr_price = file_get_contents('https://min-api.cryptocompare.com/data/price?fsym=XMR&tsyms=BTC,USD,EUR&extraParams=your_app_name');
								$l         = json_decode($xmr_price, TRUE);
								if ($currency == 'USD') {
												return $l['USD'];
								}
								if ($currency == 'EUR') {
												return $l['EUR'];
								}
				}
				
				public function changeto($amount, $currency)
				{
								$xmr_live_price = $this->retriveprice($currency);
								$new_amount     = $amount / $xmr_live_price;
								return $new_amount;
				}
				
				
				// Submit payment and handle response
				public function process_payment($order_id)
				{
								$order = wc_get_order($order_id);
								$order->update_status('on-hold', __('Awaiting offline payment', 'monero_gateway'));
								// Reduce stock levels
								$order->reduce_order_stock();
								
								// Remove cart
								WC()->cart->empty_cart();
								
								// Return thankyou redirect
								return array(
												'result' => 'success',
												'redirect' => $this->get_return_url($order)
								);
								
				}
				
				
				
				// Validate fields
				public function validate_fields()
				{
								if ($this->check_monero() != TRUE) {
												echo "<div class=\"error\"><p>Your Monero Address Seems not valid. Have you checked it?</p></div>";
								}
								
				}
				
				
				public function check_monero()
				{
								$monero_address = $this->settings['monero_address'];
								if (strlen($monero_address) == 95 && substr($monero_address, 1)) {
												return true;
								}
								return false;
				}
				
				public function make_integrated_address($payment_id)
				{
					$integrate_address_parameters = array('payment_id' => $payment_id);
					$integrate_address_method = $this->_run('make_integrated_address', $integrate_address_parameters);
					return $integrate_address_method;
				}

				public function instruction($order_id)
				{
								$order       = wc_get_order($order_id);
								$amount      = floatval(preg_replace('#[^\d.]#', '', $order->order_total));
								$currency    = $order->currency;
								$amount_xmr2 = $this->changeto($amount, $currency);
								$address     = $this->address;
								$payment_id  = bin2hex(openssl_random_pseudo_bytes(8));
								$uri         = "monero:$address?amount=$amount?payment_id=$payment_id";
								$array_integrated_address = $this->make_integrated_address($payment_id);
								// Generate a QR code
								echo "<link rel='stylesheet' href='https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css'>";
								
								echo "<div class='row'>
				
									<div class='col-sm-12 col-md-12 col-lg-12'>
				<div class='panel panel-default' id='PaymentBox_de3a227fb470475'>
        			<div class='panel-body'>
				<div class='row'>
					<div class='col-sm-12 col-md-12 col-lg-12'>
						<h3><span class='text text-warning'><img src='https://pbs.twimg.com/profile_images/473825289630257152/PzHu2yli.png' width='32px' height='32px'></span> Monero Payment Box</h3>
					</div>
					<div class='col-sm-3 col-md-3 col-lg-3'>
						<img src='https://chart.googleapis.com/chart?cht=qr&chs=150x150&chl=" . $uri . "' class='img-responsive'>
					</div>
					<div class='col-sm-9 col-md-9 col-lg-9' style='padding:10px;'>
						Send <b>" . $amount_xmr2 . " XMR</b> to<br/><input type='text'  class='form-control' value='" . $array_integrated_address["integrated_address"] . "'>
						or scan QR Code with your mobile device<br/><br/>
						<small>If you don't know how to pay with monero, click instructions button. </small>
					</div>
					<div class='col-sm-12 col-md-12 col-lg-12'>
				
						<input type='hidden' id='payment_boxID' value='de3a227fb470475'>
					</div>
				</div>
			</div>
            <div class='panel-footer'>
                        <a  class='btn btn-info btn-lg' style='width: 100%; font-size: 14px; ' data-toggle='modal' data-target='#myModal'>Instructions</a>
                    </div>
		</div></div></div>
        
         <div class='modal fade' id='myModal' role='dialog'>
    <div class='modal-dialog'>
    
      <!-- Modal content-->
      <div class='modal-content'>
        <div class='modal-header'>
          <h4 class='modal-title'>How to pay with Monero</h4>
        </div>
        <div class='modal-body container'>
           <b>Paying with Monero</b>
	   <p>If you don't have Monero, you can buy it at a trusted exchange. If you already have some, please follow instructions</p>
	   <p>Scan the QR code into your monero app or copy and paste the address above into your Monero Wallet</p>
        </div>
        <div class='modal-footer'>
          <button type='button' class='btn btn-default' data-dismiss='modal'>Close</button>
        </div>
      </div>
      </div>
      </div>
        ";
				}
				
				
				// Check if we are forcing SSL on checkout pages
				// Custom function not required by the Gateway
				public function do_ssl_check()
				{
								if ($this->enabled == "yes") {
												if (get_option('woocommerce_force_ssl_checkout') == "no") {
																echo "<div class=\"error\"><p>" . sprintf(__("<strong>%s</strong> is enabled and WooCommerce is not forcing the SSL certificate on your checkout page. Please ensure that you have a valid SSL certificate and that you are <a href=\"%s\">forcing the checkout pages to be secured.</a>"), $this->method_title, admin_url('admin.php?page=wc-settings&tab=checkout')) . "</p></div>";
												}
								}
				}
				
				public function connect_daemon(){
      					  $host = $this->settings['daemon_host'];
      					  $port = $this->settings['daemon_port'];
        				  $monero_library = new Monero($host, $port);
      					  if( $monero_library->works() == true){
         				   echo "<div class=\"notice notice-success is-dismissible\"><p>Everything works! Congratulations and Welcome aboard Monero. <button type=\"button\" class=\"notice-dismiss\">
						<span class=\"screen-reader-text\">Dismiss this notice.</span>
						</button></p></div>";
         
        				}
        				else{
          				  echo "<div class=\" notice notice-error\"><p>Error with connection of daemon, see documentation!</p></div>";
      					  } }
					
					
					
					 public function verify_payment(){
      /* 
       * fucntion for verifying payments
       * 1. Get the latest block height available
       * 2. Get_Bulk_payments with the first payment id generated 
       * 3. Verify that a payment has been made with the given payment id
       * 
      
      
      $balance_method = $monero_library->_run('getbalance');
      $balance = $balance_method['balance'];
      $height_method = $monero_library->_run('getheight');
      $height = $height_method['height'];
      $payment_id = ''; //Payment ID
      $address = '';
      $get_bulk_params = array('payment_ids' => array( $payment_id), 'min_block_height' => $height);
      $get_bulk_methods = $monero_library->_run('get_bulk_payments', $get_bulk_params);
      if( $get_bulk_methods['payments']['payment_id'] == $payment_id && $get_bulks_methods['payments']['amount'] >= $amount){
          $transaction_hash = $get_bulk_methods['payments']['tx_hash'];
          $transaction_height = $get_bulk_methods['payments']['block_height'];
          $confermations = $height - $transaction_height;
          if($height < ($transaction_height + $confermations)){
              echo "Payment has been received. We need a confirm from system.";
          }
          if($height >= ($transaction_heigh + $confermations)){
              echo "Payment has been received and confirmed. Thanks!";
          }
          if(){}
          $paid = true;
          // Email merchant
          // Notify him that someone transfer a payment
          return $transaction_hash;
      }
  }*/ 
       
        
    }
								
}
