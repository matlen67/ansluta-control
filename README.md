# ansluta-control
#### Ikea Ansluta remote control by nodeMCU and CC2500 2.4Ghz transceiver via arduino

This project was created to control Ikea Omlopp/Utrusta lights by webbrowser or add it to a home automation system (FHEM)
The original remote has one Button. With every push the lights cycle -> 50% -> 100% -> 50% -> off

The remote send a series of data icluded two own address bytes. 
- example: 0x55 0x01 0xD0 0x9A 0x02 0xAA (0xD0 & 0x9A my addressbytes)

<img src="https://github.com/matlen67/ansluta-control/blob/master/pictures/ansluta_original.jpg" width="128"> <img src="https://github.com/matlen67/ansluta-control/blob/master/pictures/leiterplatte.jpg" width="128">



## Hardware:
  - Lolin NodeMCU V3 [az-delivery](https://www.az-delivery.de/collections/wifi-module/products/copy-of-nodemcu-lua-amica-v2-modul-mit-esp8266-12e?ls=de)
  - CC2500 Transiver module (WLC24D) [ebay](https://www.ebay.com/itm/2PCS-1-8-3-6V-CC2500-IC-Wireless-RF-2400MHZ-Transceiver-Module-SPI-ISM-Demo-Code/401239287968)

<img src="https://github.com/matlen67/ansluta-control/blob/master/pictures/NodeMCU_V3.jpg" width="128">       <img src="https://github.com/matlen67/ansluta-control/blob/master/pictures/WLC-24D.png" width="128">

## Software:
  - Arduino IDE [www.arduino.cc](https://www.arduino.cc/en/Main/Software)


#### Setup Arduino IDE

- Go to Tools -> Boards Manager and seach 'nodemcu' 
- install -> 'esp8266 by ESP8266 Community'
<img src="https://github.com/matlen67/ansluta-control/blob/master/pictures/arduino_boardmanager.png" width="64">

- Select Board:  NodeMCU 1.0 (ESP.12E Module)
- Set CPU Frequency to 160 Mhz
- Select com port
<img src="https://github.com/matlen67/ansluta-control/blob/master/pictures/arduino_esp8266_config.png" width="64">


## Get the Ansluta address bytes from your remote
- start Arduion ide, load project files
- edit ansluta.ino -> set your SSID and Wlankey at row 40, 41
- write project to nodeMCU
- determine your nodeMCU IP and connect browser. example: **192.168.178.130/ansluta/getAddress**
- press button on original remote
- note your AddressBytes
- edit ansluta.ino -> set your AddressByteA and AddressByteB at row 54, 55
- write project to nodeMCU


## Control the lights via webbrowser
connect to nodeMCU IP
press button: off, 50% or 100% to toggle the lights

<img src="https://github.com/matlen67/ansluta-control/blob/master/pictures/webcontrol.jpg" width="128">

## Control the lights via FHEM
create dummy switch and notify to call the url

```
define AnslutaButton0 dummy
attr AnslutaButton0 alias Ansluta lights off
attr AnslutaButton0 devStateIcon .*:FS20.on@grey
attr AnslutaButton0 room Ansluta
attr AnslutaButton0 webCmd off

define anslutaButton0_notify notify AnslutaButton0:off  { system("curl 192.168.178.130/ansluta/0") }

define AnslutaButton50 dummy
attr AnslutaButton50 alias Ansluta lights 50%
attr AnslutaButton50 devStateIcon .*:FS20.on@orange
attr AnslutaButton50 room Ansluta
attr AnslutaButton50 webCmd on

define ansluta50_notify notify AnslutaButton50:on { system("curl 192.168.178.130/ansluta/50") }

define AnslutaButton100 dummy
attr AnslutaButton100 alias Ansluta light 100%
attr AnslutaButton100 devStateIcon .*:FS20.on@yellow
attr AnslutaButton100 room Ansluta
attr AnslutaButton100 webCmd on

define ansluta100_notify notify AnslutaButton100:on { system("curl 192.168.178.130/ansluta/100") }
```

example pictures FHEM and Tablet-UI:

<img src="https://github.com/matlen67/ansluta-control/blob/master/pictures/fhem_dummy.png" width="128">  <img src="https://github.com/matlen67/ansluta-control/blob/master/pictures/fhem-tablet-ui.png" width="128">





  
