# Pachete HellBoost pentru dezvoltare

- `HellBoost-dev-source.zip`: sursele launcherului, bibliotecii native, adaptorului PHP și configurației masterului.
- `HellBoost-native-dev-x86.zip`: aceleași surse plus DLL Windows x86 și executabilul de diagnostic ABI. Launcherul rămâne Python; necesită Python 3.11+ cu Tcl/Tk.

Pentru descărcarea unui ZIP din GitHub, deschide fișierul și apasă **Download raw file**. Fiecare arhivă include un manifest SHA-256.

Acestea nu sunt clientul final CS 1.6. Nu includ jocul, un EXE al launcherului, un browser integrat în motor sau raportarea statisticilor live. Nu copia DLL-ul peste bibliotecile CS. `native/abi_test.exe` este numai un test al bibliotecii cu evenimente sintetice, nu un test de conectare în joc.

Pentru launcher, extrage arhiva într-un folder separat și rulează `HellBoost/PORNESTE.cmd`. Citește `README.md` din arhivă înainte de a instala adaptorul PHP; el nu a fost publicat pe hellboost.net.
