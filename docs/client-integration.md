# HellBoost independent: arhitectură și integrare

## Componente proprii

`native/browser` este aplicația principală C++/Win32. `model.cpp` validează schema API-ului, păstrează ordinea platformei, filtrează lista și construiește argumentele de pornire. `windows.cpp` implementează HTTPS prin WinHTTP, salvarea unui singur document de setări în profilul utilizatorului și pornirea procesului fără shell. `main.cpp` implementează fereastra, listele și operațiile utilizatorului.

Nu importăm surse, biblioteci, hook-uri sau build-uri NextClient. nlohmann/json 3.11.3 este o dependență standard MIT pentru JSON, inclusă în surse. Codul vechi de interoperabilitate este arhivat în `legacy/nextclient` și nu intră în pachetele principale.

## Integrarea cu platforma existentă

Arhiva HellBoost furnizată conține `monitor/api.php`, care întoarce un obiect cu `servers`. Fiecare server are `rank`, `address`, `hostname`, `map`, `players`, `max_players`, `online`, `country` și `promo`, printre alte câmpuri. Aplicația consumă acest endpoint direct, fără un format de master specific altui client.

Precedența TOP/BOOST/VIP, expirarea serviciilor și aprobarea serverelor rămân în `ranked_servers()` pe backend. Clientul păstrează ordinea răspunsului și afișează rank-ul primit; nu decide singur pachetul plătit și nu acceptă o instrucțiune de sistem de la un server. Serverele offline sunt omise, iar duplicatele după adresă sunt eliminate. Căutarea păstrează ordinea relativă.

TLS și certificatele Windows sunt verificate implicit. Corpul este limitat la 2 MiB și 5000 de servere, cu limite de adâncime și număr de noduri JSON. Câmpurile duplicate și adresele care ar injecta comenzi sunt respinse. Cererea nu trimite date despre utilizator. Nu există un endpoint de evenimente nou publicat de acest task.

## Browserul în joc: etapă încă lipsă

Aplicația externă nu poate determina starea internă a motorului doar prin pornirea `hl.exe` sau Steam. Există o punte experimentală proprie `GameUI007` în `native/gameui`, care transmite apelurile către biblioteca originală și adaugă comanda `hellboost_options`. DLL-ul de evidență nu se încarcă automat în CS. Puntea actuală implementează numai panoul de opțiuni; nu înlocuiește încă „Find Servers” și nu confirmă intrarea în server.

SDK-ul oficial Half-Life inspectat (`b1b5cf5892918535619b2937bb927e46cb097ba1`) descrie callbackurile DLL-ului de mod și `net_api_t::Status`. Aceste declarații nu reprezintă singure un mecanism de extensie al clientului Counter-Strike închis. Nu inventăm offseturi sau metode de încărcare și nu instalăm automat DLL-uri peste jocul Steam.

Exporturile bibliotecilor încărcate `hw.dll` și `GameUI.dll` au fost inspectate static. Puntea a fost verificată pe Windows cu un contract MSVC x86 și o bibliotecă simulată, pentru toate cele 21 de metode și pentru dialog. Pachetul distribuie numai DLL-ul propriu, care cere biblioteca originală păstrată separat sub numele `HellBoost.GameUI.original.dll`. Biblioteca Valve nu este inclusă. Testarea cu jocul real rămâne necesară pe o copie separată.

## Evenimentele conectării

Biblioteca proprie `HellBoostClient.dll` urmărește fiecare tentativă separat. Un adaptor de motor trebuie să emită începutul și rezultatul pentru același ID. Durata este diferența ceasului monoton, în milisecunde; nu diferența dintre primirea a două cereri HTTP și nu timpul de pornire al procesului.

Rezultatele existente sunt `entered_game`, `failed`, `cancelled`, `superseded`. Semnalul de succes trebuie verificat în motor pe build-ul țintă. Schimbarea hărții, reinițializarea HUD-ului sau răspunsul A2S nu sunt automat o nouă intrare. Dacă un callback sosește târziu pentru o altă tentativă, este ignorat. Modificarea setării de colectare golește logic datele în așteptare.

Transportul către backend și tabloul statisticilor nu sunt implementate. Contractul viitor trebuie să includă sesiune UUID + ID tentativă, server, sursă, rezultat, durată și nickname opțional. Tierul se determină în baza de date. Țara jucătorului se estimează pe backend din IP prin infrastructura de încredere, separat de țara serverului afișată în browser. Un nickname raportat nu este o identitate Steam verificată. Numărul total online și conectările atribuite HellBoost se păstrează distinct.

## Dovezi și limite

Nucleul browserului și biblioteca de evenimente au teste executabile pe Linux. Binarele PE32 din pachetele locale se compilează prin MinGW. În GitHub Actions, aceleași surse au trecut compilarea MSVC, testele native și verificarea reală a configurării WinHTTP și a creării/închiderii ferestrei, fără a trimite cereri HTTPS sau a porni jocul. Binarele MinGW nu au fost executate direct pe Windows în această sesiune.

Platforma de producție nu a fost modificată. Conexiunea live către HellBoost a fost blocată de politica/proxy-ul mediului, deci nu este declarată funcțională. Compatibilitatea non-Steam înseamnă aici lansarea unui `hl.exe` ales de utilizator, nu furnizarea unui emulator sau compatibilitatea garantată a unui adaptor de motor cu orice client non-Steam.
