# Full-Duplex-IR-Send-Receiver



Basic Implementations
1. The “IR Connection” (IRC) sends and receives data over IR with another IRC.
2. The serial data format for the IR link is 300 baud 8E1.
3. The serial data format for UART0 is 115200 baud, 8N1.
4. Each message shall end with a 0 byte. This is included in the 64 bytes.
5. The only required command is “send XXXXX”, where the information after the send
   (in this case XXXXX) is sent over the IR link.
6. Data received via IR is sent to the user (uart0) unaltered.


Advanced Implementations
1. The IRC sends and receives at the same time (Full Duplex)
2. Messages are able to send up to 64bytes 
3. On a data error, the on-board RED LED is illuminated for 1 second and then
   cleared.
4. On entering an ISR, the BLUE LED is illuminated. On exiting the ISR, the LED
   is extinguished. 
