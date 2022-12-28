/*
 * clkcnt.h - clock cycle counter driver
 * 07-03-19 E. Brombaugh
 */

#include "clkcnt.h"

/*
 * delay for clocks
 */
void clkcnt_wait(uint32_t clks)
{
	clkcnt_reg = 0;
	
	while(clkcnt_reg < clks);
}

/*
 * delay for exactly 1 millisecond
 */
void clkcnt_1ms(void)
{
	clkcnt_wait(24000);
}

/*
 * delay for number of milliseconds
 */
void clkcnt_delayms(uint32_t ms)
{
	while(ms--)
		clkcnt_1ms();
}
