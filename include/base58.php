<?php

class Base58
{
	private static $alphabet = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
	
	// algo: 
	//	* split into 11 char chunks
	//	* then multiply from the last number first
	private static function decode_chunk($dt, $chunk)
	{
		$mul = 1;
		$num = bcadd(0, 0);
		
		for ($i = strlen($chunk) - 1; $i >= 0; $i--) {
			$num = bcadd(bcmul($dt[$chunk[$i]], $mul), $num);
			$mul = bcmul($mul, 58);
		}
		
		$bin = "";
		while ($num > 0) {
			$byte = bcmod($num, 256);
			$bin = pack('C', $byte) . $bin;
			$num = bcdiv($num, 256, 0);
		}
		return $bin;
	}
	
	public static function decode($str)
	{
		$et = str_split(self::$alphabet);
		$dt = array_flip($et);
		
		$nums = [];
		$chunks = str_split($str, 11);
		
		foreach ($chunks as $chunk)
		{
			$nums[] = self::decode_chunk($dt, $chunk);
		}
		
		return implode('', $nums);
	}
}
