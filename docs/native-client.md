# Biblioteca nativă HellBoost

`native/hellboost_client.h` definește ABI C versiunea 1 pentru o bibliotecă Win32. CS 1.6 este un proces x86 chiar și pe Windows x64. Codul este propriu; nu sunt incluse surse, DLL-uri sau mecanisme de autentificare NextClient/Steam.

Un adaptor încă neimplementat trebuie să încarce biblioteca și să apeleze `hb_begin` la inițierea conectării din motor, apoi `hb_complete` la un eveniment confirmat pentru aceeași încercare. Callbackul trebuie să păstreze ID-ul încercării. `hb_begin` nu trebuie apelat doar pentru că launcherul a pornit un proces. `hb_complete(HB_ENTERED_GAME)` nu trebuie apelat pentru un răspuns A2S sau un nickname găsit pe server.

## Ciclul de viață

1. Verifică `hb_abi_version() == 1`, creează contextul cu `hb_create(0)`.
2. Activează colectarea numai după alegerea utilizatorului, prin `hb_set_enabled(context, 1)`.
3. La începutul conectării, transmite endpointul, sursa și nickname-ul opțional UTF-8 la `hb_begin`. ID-ul returnat este unic numai în contextul curent; transportul trebuie să îl asocieze cu un UUID de sesiune nou pentru fiecare context/proces.
4. Încheie încercarea cu `entered_game`, `failed` sau `cancelled`. O încercare nouă încheie automat tentativa anterioară ca `superseded`.
5. Consumatorul citește `hb_peek_result` și confirmă prin `hb_ack_result` numai după prelucrarea durabilă. Citirea repetată returnează aceeași înregistrare. Nu există încă transport HTTP, persistență pe disc sau endpoint de statistici nou.
6. Dezactivarea golește încercarea curentă și coada. Distruge contextul numai după oprirea tuturor apelurilor concurente; nu îl utiliza după `hb_destroy`.

Coada are 128 de rezultate. Când este plină, o confirmare întoarce `HB_QUEUE_FULL` fără să consume tentativa; adaptorul trebuie să golească și să reîncerce. O tentativă care ar înlocui-o pe cea curentă este de asemenea refuzată dacă nu există spațiu pentru rezultatul vechi. În lipsa unui transport funcțional, nu activa această colectare în producție.

Apelurile nu fac rețea sau acces la disc; starea este protejată de mutex. Erorile sunt coduri C, nu excepții C++ propagate către consumator. Durata se măsoară prin `steady_clock`, nu prin ceasul calendaristic. Nu există posibilitatea de a transmite tokenuri Steam sau o cheie API în acest ABI.

## Compilare și validare

Pe Windows, rulează `build-native.cmd`. Exporturile C fără decorare sunt declarate în `native/hellboost_client.def`. `abi_test.exe` se leagă de DLL-ul rezultat și verifică interfața reală. Testul trackerului verifică separat durate determinate și respinge întoarcerea ceasului.

Testele ABI acoperă validarea adreselor și UTF-8, dezactivarea colectării, dubluri, confirmări întârziate, suprascrierea tentativei, umplerea cozii și accesul concurent. Nu reprezintă dovada integrării cu jocul.

În mediul Linux a fost compilată și varianta PE32/i386 a DLL-ului și a testului ABI, folosind MinGW GCC 14-posix din pachetele Debian verificate prin APT. Importurile DLL-ului sunt numai `KERNEL32.dll` și `msvcrt.dll`; cele opt exporturi `hb_*` au fost verificate static. Binarele Windows nu au fost executate aici. Testele aceleiași surse au fost executate pe Linux.

Comenzi de cross-compilare, cu toolchain-ul MinGW x86 disponibil în PATH:

```sh
mkdir -p build
i686-w64-mingw32-g++-posix -std=c++17 -Wall -Wextra -Werror -pedantic -DHB_BUILD -shared -static -static-libgcc -static-libstdc++ native/hellboost_client.cpp native/hellboost_client.def -o build/HellBoostClient.dll
i686-w64-mingw32-g++-posix -std=c++17 -Wall -Wextra -Werror -pedantic -static -static-libgcc -static-libstdc++ native/tests/abi_test.cpp build/HellBoostClient.dll -o build/abi_test.exe
```

Pe Windows, `native\abi_test.exe` din pachetul nativ verifică DLL-ul aflat alături și afișează rezultatul în terminal. Acesta este un test cu evenimente sintetice, nu un test al jocului; nu trimite statistici către HellBoost.
