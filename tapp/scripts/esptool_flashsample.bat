rem copy %2 .\flashtbd\usersamples.tbd
.\flashtbd\esptool -p %1 -b 115200 write_flash --flash_mode dio -fs detect 0xB00000 %2
rem .\flashtbd\esptool -p %1 -b 115200 write_flash --flash_mode dio -fs detect 0xB00000 .\flashtbd\usersamples.tbd