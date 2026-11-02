# HellBoost 0.3 — aplicație Windows independentă

HellBoost este un browser de servere și launcher propriu pentru CS 1.6. Aplicația principală este acum **C++/Win32**, fără Python și fără module NextClient. Lista, ordinea boosturilor și informațiile despre servere vin din API-ul existent al platformei HellBoost.

**Stadiu:** aplicație externă în dezvoltare, nu un înlocuitor final pentru clientul CS. Nu modifică motorul, nu înlocuiește „Find Servers” din joc și nu raportează încă intrări confirmate sau timpi reali de conectare.

## Descărcare și pornire

Pachetul pentru Windows este `downloads/HellBoost-dev-windows-x86.zip`. Extrage-l într-un folder separat și deschide `HellBoost/HellBoost.exe` sau `PORNESTE.cmd`. Nu trebuie instalat Python pentru acest pachet. Nu sunt incluse jocul sau bibliotecile lui.

1. Apasă **REINCARCA LISTA** pentru a încărca `https://hellboost.net/api.php`.
2. Alege un server sau introdu o adresă `IP:port`/`domeniu:port`.
3. Introdu numele și apasă **PORNESTE CS 1.6**.

Steam are prioritate dacă este detectat. Folosește build-ul/ramura selectată în Steam; aplicația nu schimbă ramura și nu poate garanta că utilizatorul deține ori are instalat CS. Ținta de integrare este Steam legacy, protocol 48, build 8684.

Dacă Steam lipsește, aplicația caută `hl.exe` lângă EXE, în `client/hl.exe`, apoi în locația salvată. Dacă nu îl găsește, cere selectarea lui. Directorul trebuie să conțină `cstrike`. Aplicația nu instalează un emulator Steam și nu schimbă autentificarea jocului.

## Funcții existente

- Internet, Favorite și Istoric, cu căutare după nume, adresă sau hartă.
- Ordinea din platformă este păstrată; nu sortăm după ping sau număr de jucători. Filtrarea și eliminarea serverelor offline pot reduce lista vizibilă.
- Coloane pentru poziție, nume, jucători, hartă, boost, țara serverului și adresă. Valorile sunt ale API-ului, nu măsurători live ale aplicației.
- Favoritele, ultimele 100 de cereri de conectare, numele, modul fereastră și instalarea locală se salvează sub `HKEY_CURRENT_USER\Software\HellBoost`, în `SettingsJson`.
- Cererea HTTPS rulează în fundal, cu validare de certificat, TLS 1.2, limite de timp și de mărime. Redirecturile sunt respinse. O eroare rămâne vizibilă; nu este înlocuită cu servere demonstrative.
- `HellBoostClient.dll` este o bibliotecă proprie pentru măsurare și evidența tentativelor. Nu este încă încărcată de motor și nu este necesară pentru pornirea EXE-ului.

Istoricul înseamnă **cereri de pornire/conectare**, nu jucători intrați. Țara afișată este a serverului din API, nu țara jucătorului. Aplicația nu colectează sau transmite acum identități de jucători ori evenimente de conectare.

## Ce rămâne pentru clientul integrat în joc

Este necesar un adaptor propriu pentru motorul build 8684: browserul din joc, confirmarea intrării efective, corelarea cu tentativa, transportul statisticilor și integrarea lor în platformă. Compilarea bibliotecii nu rezolvă acest adaptor. Nu copia DLL-ul de dezvoltare peste `client.dll`, `hw.dll` sau `GameUI.dll`.

[Detalii despre arhitectură și limitări](docs/client-integration.md). Codul vechi de compatibilitate este păstrat numai în `legacy/nextclient`, în afara pachetelor și a build-ului principal.

## Compilare Windows

Instalează Visual Studio 2022 Build Tools cu **Desktop development with C++**, apoi rulează:

```bat
build-native.cmd
powershell -File tools\smoke_windows.ps1
```

Scriptul compilează aplicația, DLL-ul și testele x86. Testul verifică inițializarea WinHTTP cu TLS 1.2 și redirecturi dezactivate, apoi deschide și închide fereastra fără să pornească jocul sau să contacteze API-ul. Workflow-ul `.github/workflows/windows.yml` face aceiași pași și pregătește un artifact ZIP.

Binarele incluse în `downloads` au fost compilate pe Linux cu MinGW GCC 14-posix. Aceleași surse au trecut compilarea MSVC, testele native și testul de creare/închidere a ferestrei pe runnerul Windows din GitHub Actions; pentru build-ul `555b08d` a trecut și configurarea WinHTTP fără rețea. Binarele MinGW nu au fost executate direct pe Windows aici. Conectarea live la API și pornirea CS rămân de verificat pe calculatorul de test. Windows 10/11 sunt țintele de test; Windows 7 nu este garantat, iar Steam nu îl mai suportă oficial.

## Teste și pachete

```sh
c++ -std=c++17 -Wall -Wextra -Werror -pedantic native/browser/model.cpp native/tests/browser_test.cpp -o /tmp/hellboost-browser-test
/tmp/hellboost-browser-test
python -m unittest discover -s tests -v
python tools/package.py --windows
```

Testele C++ verifică schema API, ordinea boosturilor, erori și dubluri, Unicode, filtrele, favoritele și comenzile de lansare. Biblioteca de evenimente are [testele ABI și de durată](docs/native-client.md). Launcherul Python vechi este păstrat ca instrument opțional, nu este utilizat de EXE. Cele două teste Tk ale lui necesită display; pe Linux pot rula prin `xvfb-run -a`.

`python tools/package.py` generează pachetul sursă. `--native` adaugă DLL-ul și testele de diagnostic; `--windows` include și aplicația nativă. Pachetele au manifest SHA-256 și exclud fișierele jocului, arhiva originală a platformei și integrarea istorică NextClient.

[Compilare prin MinGW și interfața bibliotecii](docs/native-client.md). Biblioteca JSON standard folosită este nlohmann/json 3.11.3 (MIT), cu sursa și licența incluse în `third_party/nlohmann`.
