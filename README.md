# keylessDoorAccess
Enter the flat without a key by using predefined pins and/or temporary tans

This project uses a 4x4 keyboard a 12v door opener, two 5v relais and an adruino uno to enable keyless access to our flat. Access is provided via pins and tans. A pin is a predefined digit sequence, which will allow opening the door as well as change settings and add/remove tans. A tan is a temporary pin, it can only be used to open the door. The pin/tan entry is brute force protected by setting forced delays after two unsuccesful pin entries that last longer after each failed attempt.

![4x4 keypad](https://kuenzler.io/img/gh/2019-11-16%2013.43.59.jpg)
![arduino and relais](https://kuenzler.io/img/gh/2019-11-16%2013.42.59.jpg)
