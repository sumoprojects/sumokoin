<?php
/* Main Gateway of Monero using a daemon online */

class Monero_Gateway extends WC_Payment_Gateway
{
	private $monero_daemon;
				function __construct()
				{
								$this->id                 = "monero_gateway";
								$this->method_title       = __("Monero GateWay", 'monero_gateway');
								$this->method_description = __("Monero Payment Gateway Plug-in for WooCommerce. You can find more information about this payment gateway in our website. WARN: You'll need a daemon online for your address.", 'monero_gateway');
								$this->title              = __("Monero Gateway", 'monero_gateway');
								$this->version		  = "0.2";
								//
								$this->icon               = apply_filters('woocommerce_offline_icon', '');
								$this->has_fields         = false;
								
								$this->log = new WC_Logger();
								
								$this->init_form_fields();
								$this->host = $this->get_option('daemon_host');
								$this->port = $this->get_option('daemon_port');
								$this->address = $this->get_option('monero_address');
								$this->username = $this->get_option('username');
								$this->password = $this->get_option('password');
								
								// After init_settings() is called, you can get the settings and load them into variables, e.g:
								// $this->title = $this->get_option('title' );
								$this->init_settings();
								
								// Turn these settings into variables we can use
								foreach ($this->settings as $setting_key => $value) {
												$this->$setting_key = $value;
								}

								
				                add_action('admin_notices', array($this,'do_ssl_check'));
								add_action('admin_notices', array($this,'validate_fields'));
					            add_action('woocommerce_thankyou_' . $this->id, array($this,'instruction'));

								if (is_admin()) {
												/* Save Settings */
												add_action('woocommerce_update_options_payment_gateways_' . $this->id, array($this,'process_admin_options'
												));
								}
					$this->monero_daemon = new Monero_Library($this->host . ':' . $this->port . '/json_rpc', $this->username, $this->password);
				}
				
				public function admin_options()
				{
								$this->log->add('Monero_gateway', '[SUCCESS] Monero Settings OK');
								echo "<noscript><p><img src='http://monerointegrations.com/stats/piwik.php?idsite=2&rec=1' style='border:0;' alt='' /></p></noscript>";
								echo "<h1>Monero Payment Gateway</h1>";
								echo "<p>Welcome to Monero Extension for WooCommerce. Getting started: Make a connection with daemon <a href='https://reddit.com/u/serhack'>Contact Me</a>";
								echo "<table class='form-table'>";
								$this->generate_settings_html();
								echo "</table>";
								echo "<h4>Learn more about using a password with the monero wallet-rpc <a href=\"https://github.com/cryptochangements34/monerowp/blob/master/README.md\">here</a></h4>";
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
												'username' => array(
													'title' => __('username',  'monero_gateway'),
													'desc_tip' => __('This is the username that you used with your monero wallet-rpc', 'monero_gateway'),
													'type' => __('text'),
													'default' => __('username','monero_gateway'),
													
												),
												'password' => array(
													'title' => __('password',  'monero_gateway'),
													'desc_tip' => __('This is the password that you used with your monero wallet-rpc', 'monero_gateway'),
													'description' => __('you can leave these fields empty if you did not set', 'monero_gateway'),
													'type' => __('text'),
													'default' => ''
													
												),
												'environment' => array(
																'title' => __(' Test Mode', 'monero_gateway'),
																'label' => __('Enable Test Mode', 'monero_gateway'),
																'type' => 'checkbox',
																'description' => __('Check this box if you are using testnet', 'monero_gateway'),
																'default' => 'no'
												),
								);
				}
				
				
				public function retriveprice($currency)
				{
								$xmr_price = file_get_contents('https://min-api.cryptocompare.com/data/price?fsym=XMR&tsyms=BTC,USD,EUR,CAD,INR,GBP&extraParams=monero_woocommerce');
								$price         = json_decode($xmr_price, TRUE);
								if(isset($price)){
									$this->log->add('Monero_Gateway', '[ERROR] Unable to get the price of Monero');
								}
								if ($currency == 'USD') {
												return $price['USD'];
								}
								if ($currency == 'EUR') {
												return $price['EUR'];
								}
								if ($currency == 'CAD'){
												return $price['CAD'];
								}
								if ($currency == 'GBP'){
												return $price['GBP'];
								}
								if ($currency == 'INR'){
												return $price['INR'];
								}
				}
				
				public function changeto($amount, $currency)
				{
							if(!isset($_COOKIE['rate']))
							{
								$xmr_live_price = $this->retriveprice($currency);
								setcookie('rate', $xmr_live_price, time()+2700);
								$new_amount     = $amount / $xmr_live_price;
								$rounded_amount = round($new_amount, 12); //the moneo wallet can't handle decimals smaller than 0.000000000001
								return $rounded_amount;
							}
							else
							{
								$rate_cookie = $_COOKIE['rate'];
								$xmr_live_price = $this->retriveprice($currency);
								if($xmr_live_price - $rate_cookie <= 1) //reset rate if there is a difference of 1 EURO/DOLLAR/ETC between the live rate and the cookie rate
								{										//this is so that the merchant does not lose money from exchange market forces or cookie tampering
									$new_amount     = $amount / $rate_cookie;
									$rounded_amount = round($new_amount, 12);
									return $rounded_amount;
								}
								else
								{
									setcookie('rate', $xmr_live_price, time()+2700);
									$new_amount     = $amount / $xmr_live_price;
									$rounded_amount = round($new_amount, 12);
									return $rounded_amount;
								}	
							}	
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
				
				private function set_paymentid_cookie()
				{
					if(!isset($_COOKIE['payment_id']))
					{ 
						$payment_id  = bin2hex(openssl_random_pseudo_bytes(8));
						setcookie('payment_id', $payment_id, time()+2700);
					}
					else
						$payment_id = $_COOKIE['payment_id'];
					return $payment_id;
				}
				
				public function instruction($order_id)
				{
								$order       = wc_get_order($order_id);
								$amount      = floatval(preg_replace('#[^\d.]#', '', $order->order_total));
								$currency    = $order->currency;
								$amount_xmr2 = $this->changeto($amount, $currency);
								$address     = $this->address;
								$payment_id  = $this->set_paymentid_cookie();
								$uri         = "monero:$address?amount=$amount?payment_id=$payment_id";
								$array_integrated_address = $this->monero_daemon->make_integrated_address($payment_id);
								if(isset($array_integrated_address)){
									$this->log->add('Monero_Gateway', '[ERROR] Unable to getting integrated address ');
								}
								$message = $this->verify_payment($payment_id, $amount_xmr2, $order);
								echo "<h4>".$message."</h4>";
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
						                          Send <b>" . $amount_xmr2 . " XMR</b> to<br/><input type='text'  class='form-control' value='" . $array_integrated_address["integrated_address"]."'>
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
		              </div>
                    </div>
                </div>
        
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
      <script type='text/javascript'>
  setTimeout(function () { location.reload(true); }, 30000);
</script>";
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
						$this->log->add('Monero_gateway','[ERROR] Plugin can not reach wallet rpc.');
          				  echo "<div class=\" notice notice-error\"><p>Error with connection of daemon, see documentation!</p></div>";
      					  } }
					
					
					
	public function verify_payment($payment_id, $amount, $order_id){
      /* 
       * function for verifying payments
       * Check if a payment has been made with this payment id then notify the merchant
       */
       
      $amount_atomic_units = $amount * 1000000000000;
      $get_payments_method = $this->monero_daemon->get_payments($payment_id);
      if(isset($get_payments_method["payments"][0]["amount"]))
      { 
		if($get_payments_method["payments"][0]["amount"] >= $amount_atomic_units)
		{
			$message = "Payment has been received and confirmed. Thanks!";
			$this->log->add('Monero_gateway','[SUCCESS] Payment has been recorded. Congrats!');
			$paid = true;
			$order = wc_get_order($order_id);
			$order->update_status('completed', __('Payment has been received', 'monero_gateway'));	
		}  
	  }
	  else
	  {
		  $message = "We are waiting for your payment to be confirmed";
	  }
	  return $message;  
  }
}						
