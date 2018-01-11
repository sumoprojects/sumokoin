<?php

class Base58
{
	private static $alphabet = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
	
	// algo: 
	//	* split into 11 char chunks
	//	* then multiply from the last number first
	private static function decode_chunk($dt, $chunk, $last = false)
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
		if (!$last && strlen($bin) < 8)
		{
			$bin = str_repeat("\x00", 8 - strlen($bin)).$bin;
		}
		return $bin;
	}
	
	public static function decode($str)
	{
		$et = str_split(self::$alphabet);
		$dt = array_flip($et);
		
		$nums = [];
		$chunks = str_split($str, 11);
		
		for($i = 0; $i < count($chunks); $i++)
		{
			$chunk = $chunks[$i];
			$nums[] = self::decode_chunk($dt, $chunk, $i+1 == count($chunks));
		}
		
		return implode('', $nums);
	}
}
