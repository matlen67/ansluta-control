# ansluta-control
webcontrol Ikea Ansluta 2,4Ghz 

This project was created to control the Ikea Omlopp/Utrusta lights by webbrowser or add it to a home automation system (FHEM)

I used folloing components:

## Hardware:
  - Lolin NodeMCU V3 [az-delivery](https://www.az-delivery.de/collections/wifi-module/products/copy-of-nodemcu-lua-amica-v2-modul-mit-esp8266-12e?ls=de)
  - CC2500 Transiver module (WLC24D) [ebay](https://www.ebay.com/itm/2PCS-1-8-3-6V-CC2500-IC-Wireless-RF-2400MHZ-Transceiver-Module-SPI-ISM-Demo-Code/401239287968)

## Software:
  - Arduino IDE [www.arduino.cc](https://www.arduino.cc/en/Main/Software)

  
## Get your Ansluta address bytes
- start Arduion ide, load project files
- edit ansluta.ino -> set your SSID and Wlankey at row 40, 41
- write project to nodeMCU
- determine your nodeMCU IP and connect browser. example: **192.168.178.130/ansluta/getAddress**
- press button on original remote
- note your AddressBytes
- edit ansluta.ino -> set your AddressByteA and AddressByteB at row 54, 55
- write project to nodeMCU

  
