<?php

class Sumo_Payment
{
    private $payment_id;
    private $gw;
    private $subaddress;
    
    public function __construct($gw, $order_id)
    {
        global $wpdb;
        
        $this->gw = $gw;
        if ($gw->settings['subaddress_payments'] == "no")
        {
            // TODO: make a UNIQUE PRIMARY KEY on both order_id and payment_id
            $wpdb->query("
                CREATE TABLE IF NOT EXISTS {$wpdb->prefix}sumo_payments (
                    order_id INT,
                    payment_id char(16) UNIQUE PRIMARY KEY
                )
            ");
            
            $sql = $wpdb->prepare("
                SELECT payment_id
                FROM {$wpdb->prefix}sumo_payments
                WHERE order_id = %s
            ", [$order_id]);
            $rows = $wpdb->get_results($sql);
            
            if (count($rows) <= 0) {
                $this->payment_id = bin2hex(openssl_random_pseudo_bytes(8));
                $sql = $wpdb->prepare("
                    INSERT INTO {$wpdb->prefix}sumo_payments
                    (order_id, payment_id)
                    VALUES (%s, %s)
                ", [$order_id, $this->payment_id]);
                $wpdb->get_results($sql);
            }
            else{
                $this->payment_id = $this->sanitize_id($rows[0]->payment_id);
            }
        } else {
            // TODO: make a UNIQUE PRIMARY KEY on both order_id and payment_id
            $wpdb->query("
                CREATE TABLE IF NOT EXISTS {$wpdb->prefix}sumo_address_payments (
                    order_id INT,
                    address_index INT,
                    address char(98) UNIQUE PRIMARY KEY
                )
            ");
            
            $sql = $wpdb->prepare("
                SELECT address, address_index
                FROM {$wpdb->prefix}sumo_address_payments
                WHERE order_id = %s
            ", [$order_id]);
            $rows = $wpdb->get_results($sql);
            
            if (count($rows) <= 0) {
                $resp = $gw->sumo_daemon->create_address('order_'.$order_id);
                $sql = $wpdb->prepare("
                    INSERT INTO {$wpdb->prefix}sumo_address_payments
                    (order_id, address, address_index)
                    VALUES (%d, %s, %d)
                ", [$order_id, $resp['address'], $resp['address_index']]);
                $wpdb->get_results($sql);
                $this->address = $resp['address'];
                $this->address_index = $resp['address_index'];
            }
            else{
                $this->address = $rows[0]->address;
                $this->address_index = $rows[0]->address_index;
            }
        }
    }
    
    private function sanitize_id($payment_id)
    {
        // Limit payment id to alphanumeric characters
        $sanatized_id = preg_replace("/[^a-zA-Z0-9]+/", "", $payment_id);
        return $sanatized_id;
    }
    
    public function get_uri($amount)
    {
        if ($this->gw->settings["subaddress_payments"] == "no") {
            $address = $this->gw->settings["sumo_address"];
            if (!isset($address)) {
                $gw->log->add('Sumo_Gateway', '[ERROR] No SUMO address set for payments');
                echo "ERROR: Unable to receive payments, please contact administrator.<br/>";
                return false;
            }
            return "sumo:$address?amount=$amount?payment_id=$this->payment_id";
        } else {
            return "sumo:$this->subaddress?amount=$amount";
        }
    }
    
    public function get_address()
    {
        if ($this->gw->settings["subaddress_payments"] == "no") {
            $integrated = $this->gw->sumo_daemon->make_integrated_address($this->payment_id);
            if (!isset($integrated)) {
                $this->log->add('Sumo_Gateway', '[ERROR] Unable to get integrated address');
                return $this->gw->settings["sumo_address"];;
            }
            return $integrated["integrated_address"];
            
            $address = $this->gw->settings["sumo_address"];
            $address = $this->address;
            if (!isset($address)) {
                $gw->log->add('Sumo_Gateway', '[ERROR] No SUMO address set for payments');
                echo "ERROR: Unable to receive payments, please contact administrator.<br/>";
                return false;
            }
            return "sumo:$address?amount=$amount?payment_id=$this->payment_id";
        } else {
            return $this->address;
        }
    }
    
    public function verify($order_id, $amount)
    {
        /*
         * function for verifying payments
         * Check if a payment has been made with this payment id then notify the merchant
         */
        $amount_atomic_units = $amount * 1000000000;
        if ($this->gw->settings["subaddress_payments"] == "no") {
            $get_payments_method = $this->gw->sumo_daemon->get_payments($this->payment_id);
            if (isset($get_payments_method["payments"][0]["amount"])) {
                if ($get_payments_method["payments"][0]["amount"] >= $amount_atomic_units) {
                    return true;
                }
            }
        } else {
            $payments = $this->gw->sumo_daemon->get_transfers("in", true, ["subaddr_indices" => [(int)$this->address_index]]);
            $owed = $amount_atomic_units;
            //print "OWING: ".($owed / 1000000000)."<br/>";
            if (!isset($payments["in"])) return false;
            foreach ($payments["in"] as $payment)
            {
                //print "AMOUNT: {$payment["amount"]}<br/>\n";
                $owed -= $payment["amount"];
            }
            //print "OWED: ".($owed / 1000000000)."<br/>";
            return $owed <= 0;
        }
        return false;
    }
}
