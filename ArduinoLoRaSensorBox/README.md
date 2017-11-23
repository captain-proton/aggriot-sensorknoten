# AggrIoT Sensor Box #  {#mainpage}

## Overview and hardware setup ##

Any used sensor box is build upon an Arduino Uno that uses various sensors to read environment data and broadcasts them via LoRaWAN. On top of the arduino the lora shield ([Dragino LoRa](http://wiki.dragino.com/index.php?title=Lora_Shield)) is placed, with the [groove base shield](https://www.seeedstudio.com/Base-Shield-V2-p-1378.html). The shields can be stacked in any order. There are implementations various sensors. The sensors can be activated with preprocessor instructions inside the `platform.ini`.

| Sensor | `CPPDEFINES` |
| ------ | ------------ |
| Dust | `DUST=8` |
| Sound | `SOUND=A<number>` |
| Loudness | `LOUDNESS=A<number>` |
| Light | `LIGHT=A<number>` |
| Temperature and Humidity | `TEMP_HUM=A<number>` |
| Temperature | `TEMPERATURE=A<number>` |
| Barometer | `BARO` |
| PIR Motion | `PIR=<number>` |
| GPS | `GPS` |

Warning! GPS and Dust sensor can not work simultaneously, as the gps sensor block communication on digital pin 8, which is needed by the dust sensor. If you are using the standard dragino lora shield (without gps) remove jumper `DIO5`, otherwise communication between the dust sensor and the arduino can not function.

## Measuring ##

For each sensor, there is one class that provides measurement of sensor data. All data is calculated for a specific amount of time and can be resetted if a new measurement cycle should be started. To reduce heavy battery drain, sensor data is not calculated on every loop. The library [TaskScheduler](http://platformio.org/lib/show/721/TaskScheduler) is used. Through the use of the scheduler several tasks are executed on defined intervals, ex. light measurement every second. To keep memory usage at a minimum the following data is calculated.

| Sensor | Port | Unit |
| ------ | ---- | ---- |
| Dust | Median | pcs/0.01cf (particles per 0.01 cubic feet) |
| Sound/Loudness | Mean | - |
| Light | Mean | trend of the intensity of light NOT! lumen |
| Temp | Mean | °C |
| Humidity | Mean | % |
| Pressure | Mean | Pa |
| GPS | - | Last read longitude and latitude |
| PIR | - | `true` or `false` if motion was detected inside the measurement cycle |

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
> platformio run --environment uno_box_office_node_1 --target upload

# Enter the upload port if more than one device is connected
> platformio run --environment uno_box_office_node_1 --target upload --upload-port [/dev/...]

# There may be an error on build, if any library is missing and was not automatically loaded
# These libraries must be installed
> platformio lib install <id>

# Monitor device
> platformio device monitor
```
