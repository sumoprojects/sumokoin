<?php
/* Main Gateway of Monero using a daemon online */
class Monero_Gateway extends WC_Payment_Gateway {
    function __construct(){
      	$this->id = "monero_gateway";
		    $this->method_title = __("Monero GateWay", 'monero_gateway');
}
