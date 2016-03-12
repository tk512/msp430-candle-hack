# msp430-candle-hack
Controlling a flameless candle remotely using a MSP430 microcontroller

So I got this flameless candle for Xmas. It fits rather nicely inside a little candle housing that I have, so why not put this in the living room where it can be remotely controlled?

The cool thing about these flameless candles is that they're pretty realistic looking and they're certainly a lot more convenient than normal candles!

It's connected via a MSP430 Launchpad as I had a spare one.  

I use a cron job to switch it on automatically at 8PM and it switches off at midnight.

```
0 20 * * * echo "1\n" > /dev/cu.uart-DAFF426C5015332F
0 0 * * * echo "0\n" > /dev/cu.uart-DAFF426C5015332F
```
 

