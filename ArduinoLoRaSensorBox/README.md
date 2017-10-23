# Installation #

1. [PlatformIO installieren](http://docs.platformio.org/page/core.html)
2. Kommandos ausführen:

```bash
# Klonen des Repos
> git clone ssh://git@gitlab.mas.uni-due.de:2022/studenten/PG-aggrIoT/Sensorknoten.git
# alternativ
> git clone https://git@gitlab.mas.uni-due.de/studenten/PG-aggrIoT/Sensorknoten.git

> cd Sensorknoten/ArduinoLoRaDustTransmitter

# Installation der RadioHead Bibliothek
> pio lib install RadioHead
```
Nach Installation der `RadioHead` Bibliothek muss die Frequenz für den Treiber angepasst werden. In der Datei `.piolibdeps/RadioHead_ID124/RH_RF95.cpp` muss in der `init` Methode des Treiber die Frequenz auf 433 gesetzt werden.

```c
// An innocuous ISM frequency, same as RF22's
setFrequency(433.0);
```

```bash
# Bauen und Upload
> platformio run --target upload

# Bei mehreren angeschlossenen Controllern muss der Uploadport angegeben werden
> platformio run --target upload --upload-port [/dev/...]

# Monitoring per PlatformIO
> platformio device monitor
```
