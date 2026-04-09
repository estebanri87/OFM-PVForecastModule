# OFM-PVForecastModule

OpenKNX Modul zur Abfrage von PV-Ertragsprognosen über Online-Dienste.

Das Modul stellt stündliche Leistungs- und Tagesertragsprognosen der eigenen Photovoltaikanlage über Gruppenobjekte im KNX-Bus bereit. Damit lassen sich Verbraucher (z. B. Wärmepumpe, Waschmaschine, Geschirrspüler) automatisch dann einschalten, wenn hohe Eigenerzeugung erwartet wird.

## Features

- Unterstützung mehrerer Anbieter je Kanal:
  - **forecast.solar** – kostenlose Prognose ohne API-Schlüssel (Standort, Neigung, Azimut und Spitzenleistung werden übergeben)
  - **Solcast** – präzise Prognose mit kostenlosem API-Schlüssel (max. 10 Abfragen/Tag im Free-Tier)
- Gruppenobjekte je Kanal:
  - Erwarteter PV-Ertrag heute (kWh, DPT 13.013)
  - Erwarteter PV-Ertrag morgen (kWh, DPT 13.013)
  - Aktuelle Leistung (W, DPT 14.056)
  - Erwartete Leistung nächste Stunde (W, DPT 14.056)
  - Erwartete Spitzenleistung heute (W, DPT 14.056)
  - Zeitpunkt der Spitzenleistung heute (DPT 10.001)
- Konfigurierbare Anlagenparameter je Kanal:
  - Standort (Breiten-/Längengrad – aus Geräteeinstellungen oder individuell)
  - Neigungswinkel der Solarmodule (0–90°)
  - Azimut (0–359°, Kompassrichtung)
  - Spitzenleistung (kWp)
- Automatische Aktualisierung (30 Minuten / stündlich / täglich)
- Manuelle Aktualisierung per KNX-Gruppenobjekt
- Mehrere unabhängige Kanäle möglich (z. B. mehrere Dachanlagen)

## Abhängigkeiten

Das Modul setzt [OFM-Network](https://github.com/OpenKNX/OFM-Network) oder [OFM-WLANModule](https://github.com/OpenKNX/OFM-WLANModule) für die Internetverbindung voraus.

## Konfiguration (ETS)

| Parameter | Beschreibung |
|---|---|
| Prognose-Anbieter | forecast.solar oder Solcast |
| Automatische Aktualisierung | Keine / 30 Min / Stündlich / Täglich |
| Ort für PV-Anlage | Gerätestandort aus Allgemein oder individueller Ort |
| Breitengrad / Längengrad | Koordinaten der PV-Anlage (bei individuellem Ort) |
| Neigungswinkel | Neigung der Solarmodule in Grad (0–90°) |
| Azimut | Ausrichtung der Solarmodule in Grad (0=Nord, 90=Ost, 180=Süd, 270=West) |
| Spitzenleistung | Installierte Spitzenleistung (× 10 Wp) |
| API-Schlüssel | Solcast API-Schlüssel (nur bei Anbieter Solcast) |
| Site Resource ID | Solcast Site Resource ID aus dem Dashboard (nur bei Anbieter Solcast) |

### Solcast-Konto

Ein kostenloser API-Schlüssel kann unter [https://solcast.com](https://solcast.com) erstellt werden. Im Free-Tier sind bis zu 10 API-Abfragen pro Tag möglich. Die Site Resource ID wird nach dem Anlegen der PV-Anlage im Solcast-Dashboard angezeigt (Format z. B. `47d5-0628-9935-7493`).

## Hardware Unterstützung

| Prozessor | Status | Anmerkung |
|-----------|--------|-----------|
| RP2040    | Beta   |           |
| ESP32     | Beta   |           |

Getestete Hardware:
- [OpenKNX REG1 Basismodul LAN+TP](http://device.openknx.de/REG1-LAN-TP-Base)

## Einbindung in die Anwendung

In das Anwendungs-XML muss OFM-PVForecastModule aufgenommen werden:

```xml
<op:define prefix="PVF" ModuleType="31"
  share=   "../lib/OFM-PVForecastModule/src/PVForecastModule.share.xml"
  template="../lib/OFM-PVForecastModule/src/PVForecastModule.templ.xml"
  NumChannels="3"
  KoOffset="850">
  <op:verify File="../lib/OFM-PVForecastModule/library.json" ModuleVersion="0.1" />
</op:define>
```

**Hinweis:** Pro Kanal werden 6 KO's benötigt. Dies muss bei nachfolgenden Modulen bei `KoOffset` entsprechend berücksichtigt werden.

In `main.cpp` muss das PVForecastModule hinzugefügt werden:

```cpp
#include "PVForecastModule.h"
// ...

void setup()
{
    // ...
    openknx.addModule(5, openknxPVForecastModule);
    // ...
}
```

## Lizenz

[GNU GPL v3](LICENSE)
