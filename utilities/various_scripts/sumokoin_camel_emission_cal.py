#!/bin/python

EMISSION_SPEED_FACTOR = 19
DIFFICULTY_TARGET = 240 # seconds
MONEY_SUPPLY         = 88888888000000000
GENESIS_BLOCK_REWARD     = 8800000000000000
FINAL_SUBSIDY = 4000000000
COIN_EMISSION_MONTH_INTERVAL = 6 #months
COIN_EMISSION_HEIGHT_INTERVAL  = int(COIN_EMISSION_MONTH_INTERVAL * 30.4375 * 24 * 3600 / DIFFICULTY_TARGET)
HEIGHT_PER_YEAR = int((12*30.4375*24*3600)/DIFFICULTY_TARGET)
PEAK_COIN_EMISSION_YEAR = 4
PEAK_COIN_EMISSION_HEIGHT = HEIGHT_PER_YEAR * PEAK_COIN_EMISSION_YEAR


def get_block_reward(height, coins_already_generated):
    if height < (PEAK_COIN_EMISSION_HEIGHT + COIN_EMISSION_HEIGHT_INTERVAL):
        interval_num = height/COIN_EMISSION_HEIGHT_INTERVAL
        money_supply_pct = 0.1888 + interval_num*(0.023 + interval_num*0.0032)
        cal_block_reward = (MONEY_SUPPLY * money_supply_pct)/(2**EMISSION_SPEED_FACTOR)
    else:
        cal_block_reward = (MONEY_SUPPLY - coins_already_generated)/(2**EMISSION_SPEED_FACTOR)
    return cal_block_reward


def calculate_emssion_speed(print_by_year = False):
    coins_already_generated = 0
    height = 0
    total_time = 0
    block_reward = 0
    cal_block_reward = 0
    count = 0
    round_factor = 10000000
    
    print "Height\t\tB.Reward\tCoin Emitted\tEmission(%)\tDays\tYears"
    f.write("Height\tB.Reward\tCoin Emitted\tEmission(%)\tDays\tYears\n")
    while coins_already_generated < MONEY_SUPPLY - FINAL_SUBSIDY:
        emission_speed_change_happened = False
        if height % COIN_EMISSION_HEIGHT_INTERVAL == 0:
            cal_block_reward = get_block_reward(height, coins_already_generated)
            emission_speed_change_happened = True
            count += 1
        
        if height == 0:
            block_reward = GENESIS_BLOCK_REWARD
        else:
            block_reward = int(cal_block_reward) / round_factor * round_factor
        
        if block_reward < FINAL_SUBSIDY:
            if MONEY_SUPPLY > coins_already_generated:
                block_reward = FINAL_SUBSIDY
            else:
                block_reward = FINAL_SUBSIDY/2
        
        coins_already_generated += block_reward
        total_time += DIFFICULTY_TARGET
        
        if emission_speed_change_happened and (count % 2 if print_by_year else True):
            print format(height, '07'), "\t", '{0:.10f}'.format(block_reward/1000000000.0), "\t", coins_already_generated/1000000000.0, "\t", str(round(coins_already_generated*100.0/MONEY_SUPPLY, 2)), "\t\t", format(int(total_time/(60*60*24.0)), '04'), "\t", total_time/(60*60*24)/365.25
            f.write(format(height, '07') + "\t" + '{0:.8f}'.format(block_reward/1000000000.0) + "\t" + str(coins_already_generated/1000000000.0) + "\t" + '%05.2f'%(coins_already_generated*100.0/MONEY_SUPPLY) + "\t" + format(int(total_time/(60*60*24.0)), '04') + "\t" + str(round(total_time/(60*60*24)/365.25, 2)) + "\n")
        
        height += 1
        
    print format(height, '07'), "\t", '{0:.10f}'.format(block_reward/1000000000.0), "\t", coins_already_generated/1000000000.0, "\t", str(round(coins_already_generated*100.0/MONEY_SUPPLY, 2)), "\t\t", format(int(total_time/(60*60*24.0)), '04'), "\t", total_time/(60*60*24)/365.
    f.write(format(height, '07') + "\t" + '{0:.8f}'.format(block_reward/1000000000.0) + "\t" + str(coins_already_generated/1000000000.0) + "\t" + '{0:.2f}'.format(round(coins_already_generated*100.0/MONEY_SUPPLY, 2)) + "\t" + format(int(total_time/(60*60*24.0)), '04') + "\t" + str(round(total_time/(60*60*24)/365.25, 2)) + "\n")
    
if __name__ == "__main__":
    f = open("sumokoin_camel_emmission.txt", "w")
    calculate_emssion_speed()
    if COIN_EMISSION_MONTH_INTERVAL == 6:
        print "\n\n\n"
        f.write("\n\n\n")
        calculate_emssion_speed(True)
        f.close()