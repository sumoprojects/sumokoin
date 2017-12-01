<?php

/**
 * library.php
 *
 * Written using the JSON RPC specification -
 * http://json-rpc.org/wiki/specification
 *
 * @author Kacper Rowinski <krowinski@implix.com>
 * http://implix.com
 * Modified to work with monero-rpc wallet by Serhack and cryptochangements
 * This code isn't for Dark Net Markets, please report them to Authority!
 */
class Monero_Library
{
    protected $url = null, $is_debug = false, $parameters_structure = 'array';
    protected $curl_options = array(
        CURLOPT_CONNECTTIMEOUT => 8,
        CURLOPT_TIMEOUT => 8
    );
    private $username;
    private $password;
    private $httpErrors = array(
        400 => '400 Bad Request',
        401 => '401 Unauthorized',
        403 => '403 Forbidden',
        404 => '404 Not Found',
        405 => '405 Method Not Allowed',
        406 => '406 Not Acceptable',
        408 => '408 Request Timeout',
        500 => '500 Internal Server Error',
        502 => '502 Bad Gateway',
        503 => '503 Service Unavailable'
    );

    public function __construct($pUrl, $pUser, $pPass)
    {
        $this->validate(false === extension_loaded('curl'), 'The curl extension must be loaded to use this class!');
        $this->validate(false === extension_loaded('json'), 'The json extension must be loaded to use this class!');

        $this->url = $pUrl;
        $this->username = $pUser;
        $this->password = $pPass;
    }

    public function validate($pFailed, $pErrMsg)
    {
        if ($pFailed) {
            echo $pErrMsg;
        }
    }

    public function setDebug($pIsDebug)
    {
        $this->is_debug = !empty($pIsDebug);
        return $this;
    }

    /*  public function setParametersStructure($pParametersStructure)
      {
          if (in_array($pParametersStructure, array('array', 'object')))
          {
              $this->parameters_structure = $pParametersStructure;
          }
          else
          {
              throw new UnexpectedValueException('Invalid parameters structure type.');
          }
          return $this;
      } */

    public function setCurlOptions($pOptionsArray)
    {
        if (is_array($pOptionsArray)) {
            $this->curl_options = $pOptionsArray + $this->curl_options;
        } else {
            echo 'Invalid options type.';
        }
        return $this;
    }

    public function _print($json)
    {
        $json_encoded = json_encode($json, JSON_PRETTY_PRINT | JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES);
        echo $json_encoded;
    }

    public function address()
    {
        $address = $this->_run('getaddress');
        return $address;
    }

    public function _run($method, $params = null)
    {
        $result = $this->request($method, $params);
        return $result; //the result is returned as an array
    }

    private function request($pMethod, $pParams)
    {
        static $requestId = 0;
        // generating uniuqe id per process
        $requestId++;
        // check if given params are correct
        $this->validate(false === is_scalar($pMethod), 'Method name has no scalar value');
        // $this->validate(false === is_array($pParams), 'Params must be given as array');
        // send params as an object or an array
        //$pParams = ($this->parameters_structure == 'object') ? $pParams[0] : array_values($pParams);
        // Request (method invocation)
        $request = json_encode(array('jsonrpc' => '2.0', 'method' => $pMethod, 'params' => $pParams, 'id' => $requestId));
        // if is_debug mode is true then add url and request to is_debug
        $this->debug('Url: ' . $this->url . "\r\n", false);
        $this->debug('Request: ' . $request . "\r\n", false);
        $responseMessage = $this->getResponse($request);
        // if is_debug mode is true then add response to is_debug and display it
        $this->debug('Response: ' . $responseMessage . "\r\n", true);
        // decode and create array ( can be object, just set to false )
        $responseDecoded = json_decode($responseMessage, true);
        // check if decoding json generated any errors
        $jsonErrorMsg = $this->getJsonLastErrorMsg();
        $this->validate(!is_null($jsonErrorMsg), $jsonErrorMsg . ': ' . $responseMessage);
        // check if response is correct
        $this->validate(empty($responseDecoded['id']), 'Invalid response data structure: ' . $responseMessage);
        $this->validate($responseDecoded['id'] != $requestId, 'Request id: ' . $requestId . ' is different from Response id: ' . $responseDecoded['id']);
        if (isset($responseDecoded['error'])) {
            $errorMessage = 'Request have return error: ' . $responseDecoded['error']['message'] . '; ' . "\n" .
                'Request: ' . $request . '; ';
            if (isset($responseDecoded['error']['data'])) {
                $errorMessage .= "\n" . 'Error data: ' . $responseDecoded['error']['data'];
            }
            $this->validate(!is_null($responseDecoded['error']), $errorMessage);
        }
        return $responseDecoded['result'];
    }

    protected function debug($pAdd, $pShow = false)
    {
        static $debug, $startTime;
        // is_debug off return
        if (false === $this->is_debug) {
            return;
        }
        // add
        $debug .= $pAdd;
        // get starttime
        $startTime = empty($startTime) ? array_sum(explode(' ', microtime())) : $startTime;
        if (true === $pShow and !empty($debug)) {
            // get endtime
            $endTime = array_sum(explode(' ', microtime()));
            // performance summary
            $debug .= 'Request time: ' . round($endTime - $startTime, 3) . ' s Memory usage: ' . round(memory_get_usage() / 1024) . " kb\r\n";
            echo nl2br($debug);
            // send output immediately
            flush();
            // clean static
            $debug = $startTime = null;
        }
    }

    protected function & getResponse(&$pRequest)
    {
        // do the actual connection
        $ch = curl_init();
        if (!$ch) {
            throw new RuntimeException('Could\'t initialize a cURL session');
        }
        curl_setopt($ch, CURLOPT_URL, $this->url);
        curl_setopt($ch, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);
        curl_setopt($ch, CURLOPT_USERPWD, $this->username . ":" . $this->password);
        curl_setopt($ch, CURLOPT_POST, 1);
        curl_setopt($ch, CURLOPT_POSTFIELDS, $pRequest);
        curl_setopt($ch, CURLOPT_HTTPHEADER, array('Content-type: application/json'));
        curl_setopt($ch, CURLOPT_ENCODING, 'gzip,deflate');
        curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);

        curl_setopt($ch, CURLOPT_SSL_VERIFYPEER, false);
        if (!curl_setopt_array($ch, $this->curl_options)) {
            throw new RuntimeException('Error while setting curl options');
        }
        // send the request
        $response = curl_exec($ch);
        // check http status code
        $httpCode = curl_getinfo($ch, CURLINFO_HTTP_CODE);
        if (isset($this->httpErrors[$httpCode])) {
            echo 'Response Http Error - ' . $this->httpErrors[$httpCode];
        }
        // check for curl error
        if (0 < curl_errno($ch)) {
            echo 'Unable to connect to ' . $this->url . ' Error: ' . curl_error($ch);
        }
        // close the connection
        curl_close($ch);
        return $response;
    }

    //prints result as json

    function getJsonLastErrorMsg()
    {
        if (!function_exists('json_last_error_msg')) {
            function json_last_error_msg()
            {
                static $errors = array(
                    JSON_ERROR_NONE => 'No error',
                    JSON_ERROR_DEPTH => 'Maximum stack depth exceeded',
                    JSON_ERROR_STATE_MISMATCH => 'Underflow or the modes mismatch',
                    JSON_ERROR_CTRL_CHAR => 'Unexpected control character found',
                    JSON_ERROR_SYNTAX => 'Syntax error',
                    JSON_ERROR_UTF8 => 'Malformed UTF-8 characters, possibly incorrectly encoded'
                );
                $error = json_last_error();
                return array_key_exists($error, $errors) ? $errors[$error] : 'Unknown error (' . $error . ')';
            }
        }

        // Fix PHP 5.2 error caused by missing json_last_error function
        if (function_exists('json_last_error')) {
            return json_last_error() ? json_last_error_msg() : null;
        } else {
            return null;
        }
    }

    /* 
     * The following functions can all be called to interact with the monero rpc wallet
     * They will majority of them will return the result as an array
     * Example: $daemon->address(); where $daemon is an instance of this class, will return the wallet address as string within an array
     */

    public function getbalance()
    {
        $balance = $this->_run('getbalance');
        return $balance;
    }

    public function getheight()
    {
        $height = $this->_run('getheight');
        return $height;
    }

    public function incoming_transfer($type)
    {
        $incoming_parameters = array('transfer_type' => $type);
        $incoming_transfers = $this->_run('incoming_transfers', $incoming_parameters);
        return $incoming_transfers;
    }

    public function get_transfers($input_type, $input_value)
    {
        $get_parameters = array($input_type => $input_value);
        $get_transfers = $this->_run('get_transfers', $get_parameters);
        return $get_transfers;
    }

    public function view_key()
    {
        $query_key = array('key_type' => 'view_key');
        $query_key_method = $this->_run('query_key', $query_key);
        return $query_key_method;
    }

    public function make_integrated_address($payment_id)
    {
        $integrate_address_parameters = array('payment_id' => $payment_id);
        $integrate_address_method = $this->_run('make_integrated_address', $integrate_address_parameters);
        return $integrate_address_method;
    }

    /* A payment id can be passed as a string
       A random payment id will be generatd if one is not given */

    public function split_integrated_address($integrated_address)
    {
        if (!isset($integrated_address)) {
            echo "Error: Integrated_Address mustn't be null";
        } else {
            $split_params = array('integrated_address' => $integrated_address);
            $split_methods = $this->_run('split_integrated_address', $split_params);
            return $split_methods;
        }
    }

    public function make_uri($address, $amount, $recipient_name = null, $description = null)
    {
        // If I pass 1, it will be 0.0000001 xmr. Then
        $new_amount = $amount * 100000000;

        $uri_params = array('address' => $address, 'amount' => $new_amount, 'payment_id' => '', 'recipient_name' => $recipient_name, 'tx_description' => $description);
        $uri = $this->_run('make_uri', $uri_params);
        return $uri;
    }

    public function parse_uri($uri)
    {
        $uri_parameters = array('uri' => $uri);
        $parsed_uri = $this->_run('parse_uri', $uri_parameters);
        return $parsed_uri;
    }

    public function transfer($amount, $address, $mixin = 4)
    {
        $new_amount = $amount * 1000000000000;
        $destinations = array('amount' => $new_amount, 'address' => $address);
        $transfer_parameters = array('destinations' => array($destinations), 'mixin' => $mixin, 'get_tx_key' => true, 'unlock_time' => 0, 'payment_id' => '');
        $transfer_method = $this->_run('transfer', $transfer_parameters);
        return $transfer_method;
    }

    public function get_payments($payment_id)
    {
        $get_payments_parameters = array('payment_id' => $payment_id);
        $get_payments = $this->_run('get_payments', $get_payments_parameters);
        return $get_payments;
    }

    public function get_bulk_payments($payment_id, $min_block_height)
    {
        $get_bulk_payments_parameters = array('payment_id' => $payment_id, 'min_block_height' => $min_block_height);
        $get_bulk_payments = $this->_run('get_bulk_payments', $get_bulk_payments_parameters);
        return $get_bulk_payments;
    }
}
