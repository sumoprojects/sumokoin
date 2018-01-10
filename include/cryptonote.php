<?php

require_once __DIR__ . '/keccak256.php';
require_once __DIR__ . '/base58.php';

class Cryptonote
{
	public static function SplitAddress($addr)
	{
		$full = Base58::decode($addr);
		$parts = substr($full, 0, -4);
		$csum = substr($full, -4);
		
		$keys = [
			substr($parts, -64, -32),
			substr($parts, -32)
		];
		
		$prefix = substr($parts, 0, -64);
		
		return (object)[
			'body'		=> $prefix . $keys[0] . $keys[1],
			'prefix'	=> $prefix,
			'spendkey'	=> $keys[0],
			'viewkey'	=> $keys[1],
			'checksum'	=> $csum
		];
	}
	
	public static function VerifyAddress($addr)
	{
		if (function_exists('bcadd'))
		{
			$key = self::SplitAddress($addr);
			$csum = substr(Keccak256::hashcn($key->body), 0, 8);
			return bin2hex($key->checksum) == $csum;
		}
		// if we do not have bcadd then fall back on regular checks..
		else
		{
			switch(strlen($addr))
			{
				case 99: return substr($addr, 0, 4) == "Sumo";
				case 98: return substr($addr, 0, 4) == "Subo";
				default: return false;
			}
		}
	}
}