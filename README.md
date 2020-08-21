# ESP8266_LoRaGPSBikeTracker
A bike tracker that transmits its GPS coordinates over the LoRa network.
<details>
<summary>What's the LoRa network?</summary>
The LoRa network is essentially a public, free, long-range wifi network.  It has extremely low bandwidth, and is only able to transmit short messages but at the advantage of being able to do so at long distances (~10km-100km range).
</details>

## Hardware
- ESP8266 microcontroller ([Wemos D1 Mini](https://www.exp-tech.de/en/platforms/internet-of-things-iot/8898/wemos-d1-mini-wifi-board-based-on-esp-8266ex?c=1241))
- LoRa module tranceiver ([RFM95W](https://www.hoperf.com/modules/lora/RFM95.html))
- GPS module ([GY-NEO-6MV2](https://www.electroschematics.com/neo-6m-gps-module/))
- Battery (e.g. [LiPo 3.7V battery](https://www.sunderbattery.com/product/3-7v-lipo-battery/))
- [LoRa PCB](https://github.com/hallard/WeMos-Lora)
- Battery PCB (e.g. [Wemos Shield](https://www.exp-tech.de/en/platforms/wemos/9153/wemos-battery-shield))

## Firmware/Software
- (In Progress). Current Status:
  - Able to send messages over TTN
  - Able to receive GPS signal
  - Need to combine, to send GPS signal over TTN and fully test

## Network (Used for receiving LoRa messages and automating tasks)
- [The Things Network (TTN)](https://www.thethingsnetwork.org/)

## Logging/"Database"
- Google Sheets via TTN
- If This Then That (IFTT) via TTN
