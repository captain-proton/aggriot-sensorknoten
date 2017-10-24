# Installation #

1. [PlatformIO installieren](http://docs.platformio.org/page/core.html)
2. Kommandos ausführen:

```bash
# Klonen des Repos
> git clone ssh://git@gitlab.mas.uni-due.de:2022/studenten/PG-aggrIoT/Sensorknoten.git
# alternativ
> git clone https://git@gitlab.mas.uni-due.de/studenten/PG-aggrIoT/Sensorknoten.git

> cd Sensorknoten/ArduinoLoRaReceiver

# Bauen und Upload
> platformio run --target upload

# Bei mehreren angeschlossenen Controllern muss der Uploadport angegeben werden
> platformio run --target upload --upload-port [/dev/...]

# Monitoring per PlatformIO
> platformio device monitor
```
