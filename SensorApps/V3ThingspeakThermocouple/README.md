To program the esp-01s module please use the following setup:

Flash size 8Mbit: 512KB+512KB

boot_v1.2+.bin              0x00000
user1.1024.new.2.bin        0x01000
esp_init_data_default.bin   0xfc000 (optional)
blank.bin                   0x7e000 & 0xfe000

After programming the baudrate must be changed from 115200bps to 38400bps using the AT command AT+UART_DEF=38400,8,1,0,0

