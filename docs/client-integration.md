# HellBoost: integrare exclusiv din client

## Ce există în arhiva primită

- `monitor/master/master.php`: răspuns UDP GoldSrc, ordonat prin regulile platformei; include separat recepție de rapoarte de la plugin.
- `monitor/launcher.php`: listă JSON și raport de intenție înainte de lansarea jocului.
- `monitor/cron/poll.php`: asociază intențiile cu jucători observați ulterior prin interogări ale serverului. Aceasta este o estimare temporală, nu identificare confirmată.
- `CS16_Kit/launcher/Launcher.cs`: launcher C#, închide fereastra după pornirea `hl.exe`.
- `CS16_Kit/client/CITESTE.txt`: solicită explicit o bază non-Steam; nu corespunde cerinței actuale Steam.

Arhiva a fost extrasă separat pentru inspecție, fără importarea configurației, executabilelor sau datelor sale în Git și fără accesarea platformei live.

## Modelul nou

Fiecare încercare are un UUID unic. Un adaptor din joc observă începerea conectării și confirmarea intrării în joc pe același server. Durata se calculează cu un ceas monoton, în milisecunde. Pornirea procesului, clicul pe Connect, deschiderea unui port, răspunsul A2S sau apariția unui nickname nu confirmă singure intrarea.

`native/connection_tracker.hpp` implementează numai această mașină de stări. Rezultatele sunt `entered_game`, `failed`, `cancelled`, `superseded`. Callbackurile întârziate pentru alte încercări și rezultatele duplicate sunt ignorate. Schimbarea serverului în timpul conectării încheie încercarea anterioară separat. Schimbarea hărții după intrare nu trebuie raportată drept o nouă conectare adusă de platformă.

## Contract propus, încă neimplementat în API

Un rezultat va include versiunea protocolului, UUID-ul încercării, adresa serverului, sursa (`internet`, `favorites`, `history`, `console`), rezultatul, durata în milisecunde și nickname-ul opțional. Backendul determină țara din adresa cererii prin infrastructura proxy de încredere; nu din limba Windows. Tierul și apartenența serverului se verifică în baza de date, nu se acceptă din client.

Backendul trebuie să deduplice atomic UUID-ul în cadrul identității instalării, să limiteze rapoartele și dimensiunile lor și să păstreze telemetria nouă distinctă de estimările vechi. Numai `entered_game` se numără ca intrare. Încercările eșuate au propria durată. Numărul total de jucători online rămâne separat de conectările atribuite HellBoost.

Rapoartele sunt observații declarate de client; nu reprezintă automat identitate Steam verificată sau dovadă imposibil de falsificat. Un secret global în executabil nu rezolvă autentificarea. Transportul HTTPS, înregistrarea instalării și mecanismul de autentificare trebuie implementate înainte de expunerea endpointului. Oprirea telemetriei sau indisponibilitatea API nu trebuie să împiedice jocul. Nickname-ul și telemetria necesită informare clară în client și o politică de retenție.

## Verificarea Steam încă necesară

Nu există în arhivă adaptorul nativ, sursele unui browser integrat sau un build Steam de test. Ulterior, accesul Git cu permisiunea de rețea necesară a reușit. Au fost inspectate sursele oficiale Valve (`b1b5cf5892918535619b2937bb927e46cb097ba1`) și sursele publice NextClient (`2b39c3e494c8643636365bbf449d8e1f746f9929`). Constatările despre masterul HTTP și semnificația evenimentelor NextClient sunt în [nextclient-master.md](nextclient-master.md).

SDK-ul Half-Life expune callbackuri ale DLL-ului de mod și `net_api_t::Status`, dar acestea nu stabilesc singure un mecanism de extensie al clientului CS închis. NextClient folosește propriile biblioteci și integrare în motor. Urmează implementarea și verificarea adaptorului pe Windows; nu înlocuim automat DLL-uri Steam și nu folosim patch-uri binare neverificate.

Browserul din joc trebuie verificat separat: ordinea adreselor din răspunsul master nu garantează ordinea afișării după sortarea browserului. Există acum configurație pentru un NextClient existent și un endpoint PHP compatibil, dar nu au fost instalate sau testate în joc. Browserul extern al launcherului păstrează ordinea API-ului HellBoost. Nu sunt implementate încă înlocuirea browserului Steam sau colectarea evenimentelor reale.

## Validare locală

Din `/workspace/pret`:

```sh
c++ -std=c++17 -Wall -Wextra -Werror -pedantic native/tests/connection_tracker_test.cpp -o /tmp/hellboost-tracker-test
/tmp/hellboost-tracker-test
```

Testele verifică numai logica duratei și a stărilor, nu integrarea cu jocul. Testarea Windows trebuie să acopere conectare reușită, server plin, parolă greșită, anulare, reconectare, schimbare server, descărcări de resurse, schimbare hartă și API indisponibil.
