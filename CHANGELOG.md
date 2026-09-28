v0.3.0

* Feature: Neuer Kanalparameter "Suspendiert" nach OpenKNX-Standard. Damit lässt sich ein Kanal vorübergehend abschalten, ohne ihn zu deaktivieren - die Kommunikationsobjekte und ihre Gruppenadress-Verknüpfungen bleiben erhalten.
* Feature: Suspendierte Kanäle werden in der Baumansicht mit einem Verbotszeichen vor der Beschreibung gekennzeichnet.
* Breaking: Der Parameter "Prognose-Anbieter" belegt nur noch 4 statt 8 Bit, damit "Suspendiert" ohne Vergrößerung des Kanalblocks daneben passt. Bestehende Projekte müssen den Kanal neu parametrieren.


v0.2.0

* Change: Kanalauswahl nach OpenKNX-Standard – eigener Tab "Kanalauswahl" mit einer Zeile je Kanal (Kanal / Prognose-Anbieter / Beschreibung)
* Change: Der Schieberegler "Verfügbare Kanäle" und der Tab "(mehr)" entfallen; ein Kanal wird über "Deaktiviert" beim Prognose-Anbieter abgeschaltet
* Change: Deaktivierte Kanäle erscheinen nicht mehr in der Baumansicht; die Beschreibung bleibt trotzdem eingebbar
* Change: "Bezeichnung" heißt jetzt durchgängig "Beschreibung"
* Breaking: Das Speicherlayout verschiebt sich, da der Kanalzähler entfällt – bestehende Projekte müssen neu parametriert werden
* Fix: Ein neu angelegter Kanal ist standardmäßig "Deaktiviert" statt "forecast.solar"

v0.1.0

* Feature: Initiale Implementierung mit forecast.solar-Integration (weltweit, kein API-Key)
* Feature: Solcast-Integration (weltweit, Hobbyisten-API-Key)
* Feature: Konfigurierbare Anlage (Breitengrad, Längengrad, Neigungswinkel, Azimut, Spitzenleistung)
* Feature: Automatische Aktualisierung (30 min / stündlich / täglich)
* Feature: ETS Kontexthilfe (Baggages, HelpContext in XML)
* Doc: Applikationsbeschreibung mit Inhaltsverzeichnis
