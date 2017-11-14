# AggrIoT Sensor Box #  {#mainpage}

## Overview and hardware setup ##

The sensor box is build upon an Arduino Uno that uses various sensors to read environment data and broadcasts them via LoRaWAN. The hardware setup is fairly strict at the moment. On top of the arduino the lora shield ([Dragino LoRa](http://wiki.dragino.com/index.php?title=Lora_Shield)) is placed, with the [groove base shield](https://www.seeedstudio.com/Base-Shield-V2-p-1378.html). All sensors are connected to the shield on the ports:

| Sensor | Port |
| ------ | ---- |
| Dust | D8 |
| Sound | A2 |
| Light | A1 |
| Temp and Humidity | A0 |

Remove jumper `DIO5` of the lora shield, otherwise communication between the dust sensor and the arduino can not function.

## Measuring ##

For each sensor, there is one class that provides measurement of sensor data. All data is calculated for a specific amount of time and can be resetted if a new measurement cycle should be started. To reduce heavy battery drain, sensor data is not calculated on every loop. The library [TaskScheduler](http://platformio.org/lib/show/721/TaskScheduler) is used. Through the use of the scheduler several tasks are executed on defined intervals, ex. light measurement every second. To keep memory usage at a minimum the following data is calculated.

| Sensor | Port | Unit |
| ------ | ---- | ---- |
| Dust | Median | pcs/0.01cf (particles per 0.01 cubic feet) |
| Sound | Mean | - |
| Light | Mean | trend of the intensity of light NOT! lumen |
| Temp and Humidity | Mean | °C and % |

The median can not be used for sound, light, temperature and humidity as to much memory is used!

## Communication ##

As a LoRa shield is mounted on the arduino, [LoRaWAN](https://www.thethingsnetwork.org/wiki/LoRaWAN/Home) is used to communicate with the aggregator instances. The library [RadioHead](http://platformio.org/lib/show/124/RadioHead) provides functionality to send and receive data via radio. The radio operates on a frequency of 433 MHz. Security is a big subject of the project, therefore a [custom library](https://gitlab.mas.uni-due.de/studenten/PG-aggrIoT/Sensorknoten/tree/master/Protokoll/Referenzimplementierung) is used. With this library encryption and reliable message send is implemented.

To deliver sensor data, a handshake is done with the aggregator. If it is successful data will be send, handshakes are done on specific intervals. The task scheduler comes into play again. The \ref communicationmsc "main sketch file" contains a message sequence chart that shows communication between sensor box and aggregator in detail.

## Installation ##

1. [Install PlatformIO](http://docs.platformio.org/en/latest/installation.htmll)
2. Execute:

```bash
# Clone repo
> git clone ssh://git@gitlab.mas.uni-due.de:2022/studenten/PG-aggrIoT/Sensorknoten.git
# or
> git clone https://git@gitlab.mas.uni-due.de/studenten/PG-aggrIoT/Sensorknoten.git

> cd Sensorknoten/ArduinoLoRaSensorBox

# Build and upload
> platformio run --target upload

# Enter the upload port if more than one device is connected
> platformio run --target upload --upload-port [/dev/...]

# Monitor device
> platformio device monitor
```
