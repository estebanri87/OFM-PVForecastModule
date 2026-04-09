<!-- SPDX-License-Identifier: AGPL-3.0-only -->
<!-- Copyright (C) 2026 Steffen Rittmeier -->

# Applikationsbeschreibung PV-Ertragsprognose (PVForecast)

Das Modul stellt je Kanal eine solarbasierte Ertragsprognose als KNX-Gruppenobjekte bereit.  
Die Prognose wird von einem konfigurierbaren Anbieter abgerufen und auf KNX-Bus-Objekte (Ertrag Heute/Morgen, aktuelle Leistung, Spitzenwert) aufgeteilt.  
Automatisierungslogik (z. B. Waschmaschine bei hohem PV-Ertrag starten) wird nicht im Modul abgebildet – dafür ist das Logikmodul zuständig.

Folgende Anbieter stehen zur Auswahl:
* [forecast.solar](#forecastsolar) – weltweit, kein API-Key für Basisnutzung erforderlich
* [Solcast](#solcast) – weltweit, kostenloser Hobbyisten-API-Key erforderlich

---

## Inhaltsverzeichnis

- [Anbieter](#anbieter)
  - [forecast.solar](#forecastsolar)
  - [Solcast](#solcast)
- [ETS-Parameter](#prognose-anbieter)
  - [Prognose-Anbieter](#prognose-anbieter)
  - [Automatische Aktualisierung](#automatische-aktualisierung)
  - [Breitengrad](#breitengrad--100)
  - [Längengrad](#längengrad--100)
  - [Neigungswinkel](#neigungswinkel)
  - [Azimut](#azimut)
  - [Solcast API-Key](#solcast-api-key)
  - [Spitzenleistung](#spitzenleistung-kwp--100)
- [Gruppenobjekte](#gruppenobjekte)

---

# Anbieter

<!-- DOC HelpContext="forecast-solar" -->
## forecast.solar

forecast.solar liefert stündliche PV-Ertragsprognosen basierend auf Standort, Modulneigung, Azimut und installierter Spitzenleistung.  
Die kostenlose Basisnutzung benötigt keinen API-Key, ist aber auf einige Abfragen pro Stunde limitiert.  
Weitere Informationen: https://forecast.solar

<!-- DOCEND -->

---

<!-- DOC HelpContext="Solcast" -->
## Solcast

Solcast liefert hochauflösende PV-Ertragsprognosen in 30-Minuten-Schritten basierend auf Satellitendaten und maschinellem Lernen.  
Für die Nutzung ist ein kostenloser Hobbyisten-API-Key erforderlich (bis zu 10 Abfragen/Tag kostenlos).  
Der API-Key wird im ETS-Parameter **Solcast API-Key** hinterlegt.  
Die Azimut-Konvention entspricht der Kompassrose: 0° = Nord, 90° = Ost, 180° = Süd, 270° = West.  
Weitere Informationen: https://solcast.com

<!-- DOCEND -->

---

<!-- DOC HelpContext="Prognose-Anbieter" -->
## Prognose-Anbieter

Wählt den Datenanbieter für diesen Kanal.

| Wert | Bedeutung |
|------|-----------|
| Deaktiviert | Kanal ist inaktiv |
| forecast.solar | Solarprognose via forecast.solar API |
| Solcast | Solarprognose via Solcast API (API-Key erforderlich) |

<!-- DOCEND -->

<!-- DOC HelpContext="Automatische-Aktualisierung" -->
## Automatische Aktualisierung

Legt fest, in welchem Intervall die Prognose neu abgerufen wird.

| Wert | Intervall |
|------|-----------|
| Keine | Nur manuell über KO |
| 30 Minuten | Alle 30 Minuten |
| Jede Stunde | Stündlich |
| Täglich | Einmal täglich |

Empfohlen: **Jede Stunde**.

<!-- DOCEND -->

<!-- DOC HelpContext="Breitengrad" -->
## Breitengrad (× 100)

Geografischer Breitengrad des Standorts, multipliziert mit 100.  
Beispiel: 48,20° → Wert `4820`.  
Bereich: -90,00 bis +90,00 (Wert -9000 bis 9000).

<!-- DOCEND -->

<!-- DOC HelpContext="Laengengrad" -->
## Längengrad (× 100)

Geografischer Längengrad des Standorts, multipliziert mit 100.  
Beispiel: 16,37° → Wert `1637`.  
Bereich: -180,00 bis +180,00 (Wert -18000 bis 18000).

<!-- DOCEND -->

<!-- DOC HelpContext="Neigungswinkel" -->
## Neigungswinkel

Neigung der PV-Module gegenüber der Horizontalen in Grad.  
0° = waagerecht, 90° = senkrecht.  
Typischer Wert für Schrägdächer: 30–40°.

<!-- DOCEND -->

<!-- DOC HelpContext="Azimut" -->
## Azimut

Ausrichtung der PV-Module als Kompasswinkel in Grad.  
0° = Nord, 90° = Ost, 180° = Süd, 270° = West.  
Typischer Wert für eine südlich ausgerichtete Anlage: **180°**.

<!-- DOCEND -->

<!-- DOC HelpContext="Solcast-API-Key" -->
## Solcast API-Key

Nur relevant bei Solcast: API-Key des Solcast-Hobbyisten-Kontos.  
Der Schlüssel kann kostenlos unter https://toolkit.solcast.com.au/register/hobbyist registriert werden.  
Maximal 40 Zeichen.

<!-- DOCEND -->

<!-- DOC HelpContext="Spitzenleistung" -->
## Spitzenleistung (kWp × 100)

Installierte PV-Spitzenleistung in kWp, multipliziert mit 100.  
Beispiel: 5,00 kWp → Wert `500`.  
Bereich: 0 bis 50,00 kWp (Wert 0 bis 5000).

<!-- DOCEND -->

---

# Gruppenobjekte

| Nr. | Name | DPT | Richtung | Beschreibung |
|-----|------|-----|----------|--------------|
| 0 | PV-Ertrag Heute | 9.x | Ausgang | Prognostizierter Gesamtertrag des heutigen Tages in kWh |
| 1 | PV-Ertrag Morgen | 9.x | Ausgang | Prognostizierter Gesamtertrag des morgigen Tages in kWh |
| 2 | Aktuelle PV-Leistung | 9.x | Ausgang | Prognostizierte Leistung der aktuellen Stunde in W |
| 3 | PV-Leistung nächste Stunde | 9.x | Ausgang | Prognostizierte Leistung der nächsten Stunde in W |
| 4 | PV-Spitzenleistung Heute | 9.x | Ausgang | Höchste prognostizierte Leistung des heutigen Tages in W |
| 5 | Spitzenzeitpunkt Heute | 10.001 | Ausgang | Uhrzeit der prognostizierten Spitzenleistung heute |
| – | Aktualisieren | 1.017 | Eingang | Trigger: Prognose sofort neu abrufen |
