# Master HellBoost în browserul NextClient

## Ce a fost verificat în surse

Referință: repository-ul public [CS-NextClient/NextClient](https://github.com/CS-NextClient/NextClient), commit `2b39c3e494c8643636365bbf449d8e1f746f9929`. README-ul confirmă engine build 8684 și necesitatea unei copii cumpărate de CS și a clientului Steam pornit. Versiunea publică nu include emulator Steam; nu poate fi prezentată drept o bază non-Steam verificată.

`docs/master-server-http-protocol.md` și `HttpMasterClient.cpp` descriu un GET HTTPS cu un corp JSON opțional, iar `HttpMasterResponse.cpp` acceptă o listă JSON directă sau text cu o adresă IPv4:port pe linie. `monitor/launcher.php` din arhiva HellBoost returnează un obiect cu cheia `servers`, care nu are acest format. Endpointul nou rezolvă numai această diferență.

`MasterClientFactory.cpp` citește `platform/config/MasterServer.vdf`. La o adresă care începe cu `http` folosește masterul HTTP. Configurarea unui NextClient existent este astfel posibilă fără inventarea unui hook în Steam. Nu afirmăm că browserul Steam nemodificat citește această configurație.

## Fișiere pregătite pentru instalare

Pe o copie de test a platformei, așază următoarele lângă fișierele din arhiva primită:

- `integrations/hellboost/master_nextclient.php` → `monitor/master_nextclient.php`;
- `integrations/hellboost/inc/master_nextclient_format.php` → `monitor/inc/master_nextclient_format.php`.

Endpointul reutilizează `inc/db.php` și `ranked_servers('s.online = 1')`, păstrând aprobarea serverelor, boosturile și expirarea lor în logica platformei. Nu introduce credențiale noi și nu schimbă schema bazei de date. Nu îl publica într-un director fără protecțiile pentru `inc` deja descrise de platformă.

Verifică răspunsul HTTPS înainte să schimbi clientul. Trebuie să fie HTTP 200 și o listă precum `[{"address":"192.0.2.1:27015","country":"RO"}]`, cu adresele reale ale serverelor aprobate. O listă goală se întoarce ca `[]`; o eroare de bază de date ca HTTP 503, nu ca succes gol. Exemplul folosește o adresă rezervată pentru documentație, nu un server real.

Pe o instalare NextClient care suportă protocolul verificat, salvează configurația existentă separat și aplică `integrations/nextclient/platform/config/MasterServer.vdf` în copia de test. URL-ul din fișier presupune instalarea endpointului la `https://hellboost.net/master_nextclient.php`; dacă testarea se face pe alt domeniu, actualizează URL-ul înainte de utilizare. Endpointul nu a fost instalat de acest task.

README-ul upstream solicită o copie separată a jocului, în afara folderului administrat de Steam, și avertizează asupra riscului VAC dacă bibliotecile modificate sunt instalate în folderul Steam. Păstrează instalarea Steam originală. Acest proiect nu instalează sau înlocuiește automat biblioteci de joc.

## Clasarea boosturilor

Adaptorul păstrează ordinea `ranked_servers`; nu acceptă tierul de la client. Codul NextClient inspectat leagă răspunsurile de `server_index`, iar comparația `ServerIdCompare` sortează după ID. `CInternetGames::GetNewServerList` selectează această sortare inițială. Acestea sunt dovezi din surse, nu o validare în joc: filtrele, serverele fără răspuns și alegerea altei coloane pot modifica lista vizibilă. Nu sunt implementate blocarea sortării utilizatorului, etichete HellBoost pentru pachete sau o promisiune de poziție absolută pe orice versiune NextClient.

## Conectare și timp

Sursele publice expun `connect`, `disconnect` și `putinserver` în API-ul CEF. Implementarea `ExtensionConnectionApi.cpp` emite `putinserver` când crește sign-on-ul, fără să includă stadiul în eveniment. `connectedHost.state` folosește `ca_active || signon == 1`. Nu este suficient pentru a demonstra încărcarea completă a jocului.

Pentru cerința HellBoost, adaptorul nativ trebuie să stabilească un prag verificat de intrare în joc, să îl coreleze cu aceeași tentativă și să înregistreze timpul înainte de trecerea evenimentului pe firul UI/rețea. Nu utilizăm timpii de livrare ai evenimentelor JavaScript drept timp exact al motorului. Acest adaptor, transportul și integrarea în statisticile platformei nu sunt încă implementate.

## Testare și limite

Formatterul PHP și ruta HTTP au teste cu date de test, inclusiv GET cu corpul folosit de NextClient, ordine, metadate, dubluri, erori 503 și metode respinse. Testul folosește un adaptor DB de test, nu MySQL-ul live. Platforma live, biblioteca GameUI și intrarea reală într-un server nu au fost testate aici.

Pentru distribuirea unei versiuni derivate trebuie stabilite separat drepturile de utilizare ale tuturor componentelor; în checkout-ul public inspectat nu am identificat o licență generală în rădăcină. Sursele publice au fost inspectate ca referință și nu sunt copiate în proiect sau în pachetele sale. Configurația și adaptorul PHP din acest proiect sunt cod propriu.
