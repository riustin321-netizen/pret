# HellBoost client development

Țintă: CS 1.6 Steam legacy, protocol 48, build 8684. Proiectul este în dezvoltare; nu este un client final și nu conține jocul sau fișierele NextClient.

## Ce poți rula acum

- **Browser extern HellBoost:** lista Internet din API-ul existent, în ordinea primită, Favorite și Istoric. Detectează Steam; dacă lipsește, folosește instalarea locală selectată. Istoricul reprezintă cereri de pornire, nu intrări confirmate.
- **Bibliotecă nativă C++:** evenimentele unei încercări de conectare, durată monotonă, deduplicare, anulare, schimbare server și coadă limitată. Biblioteca nu este încă încărcată de joc și nu trimite statistici.
- **Adaptor de master pentru NextClient:** endpoint PHP cu formatul HTTP verificat în sursele publice NextClient și configurație VDF. Este destinat unui NextClient existent care suportă acest protocol, nu clientului Steam nemodificat. Instalarea și testul în joc sunt încă necesare.

Detalii despre [configurarea masterului](docs/nextclient-master.md), [interfața nativă](docs/native-client.md) și [limitele integrării](docs/client-integration.md).

## Pornirea browserului extern pe Windows

Instalează Python 3.11+ cu Tcl/Tk și Python Launcher, apoi rulează `PORNESTE.cmd` sau:

```bat
py -3 launcher\main.py
```

Apasă **REÎNCARCĂ LISTA** pentru o cerere HTTPS către `https://hellboost.net/launcher?a=list`. În caz de eroare nu se introduc servere demonstrative. Steam are prioritate dacă este instalat; prezența lui nu dovedește că utilizatorul deține sau are instalat CS. Launcherul nu schimbă ramura jocului și nu ocolește autentificarea.

Pentru modul local, alege propriul `hl.exe` sau pune proiectul lângă `hl.exe`/într-un folder care conține `client/hl.exe` și `client/cstrike`. Setările se salvează în `%LOCALAPPDATA%/HellBoost/settings.json`. Favoritele și istoricul sunt ale launcherului; nu se sincronizează cu browserul din joc.

## Compilare Windows x86

`build-native.cmd` necesită Visual Studio 2022 Build Tools cu Desktop development with C++. Compilează biblioteca și rulează testele native, fără să pornească jocul. DLL-ul este un modul de dezvoltare, nu un înlocuitor pentru `client.dll`; nu îl copia peste fișiere Steam.

Workflow-ul `.github/workflows/windows.yml` pregătește un EXE al launcherului, DLL-ul x86 și arhiva `HellBoost-dev-windows-x86.zip`. Workflow-ul trebuie să ajungă în GitHub și să se încheie cu succes înainte să existe un artifact descărcabil în Actions. Nu a fost rulat în acest mediu. EXE-ul folosește Python 3.11 și țintește Windows 10/11; Windows 7 nu este promis.

Pentru pachetul sursă:

```sh
python tools/package.py
```

Pentru un pachet cu EXE și DLL deja compilate:

```sh
python tools/package.py --windows
```

Comanda refuză crearea pachetului Windows dacă lipsesc binarele. `python tools/package.py --native` include DLL-ul și testul ABI pentru Windows împreună cu sursele Python ale launcherului, fără a necesita un EXE al launcherului. Fiecare arhivă conține un manifest SHA-256 al fișierelor.

## Teste

```sh
python -m unittest discover -s tests -v
php -n tests/php/master_format_test.php
c++ -std=c++17 -Wall -Wextra -Werror -pedantic native/tests/connection_tracker_test.cpp -o /tmp/hellboost-tracker-test
/tmp/hellboost-tracker-test
c++ -std=c++17 -Wall -Wextra -Werror -pedantic -fPIC -DHB_BUILD -shared native/hellboost_client.cpp -pthread -o /tmp/libHellBoostClient.so
c++ -std=c++17 -Wall -Wextra -Werror -pedantic native/tests/abi_test.cpp -L/tmp -lHellBoostClient -Wl,-rpath,/tmp -pthread -o /tmp/hellboost-abi-test
/tmp/hellboost-abi-test
```

Cele trei teste HTTP pornesc un server PHP temporar pe loopback și folosesc date de test, nu baza de date de producție. Necesită PHP CLI în PATH sau `HELLBOOST_PHP_EXECUTABLE`. Fără PHP sunt raportate explicit ca skipped. Cele două teste Tk necesită Windows sau un display X11; pe Linux fără display se raportează skipped. Pentru toate testele pe Linux se poate folosi `xvfb-run -a python -m unittest discover -s tests -v`. În medii care interzic socketurile, este necesară rularea testelor HTTP cu acces loopback permis.

Nu sunt încă verificate: interfața și pornirea pe Windows, integrarea reală în CS, masterul live și statisticile end-to-end. Platforma de producție nu a fost modificată.
