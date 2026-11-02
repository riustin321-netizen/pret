# Pachete HellBoost pentru dezvoltare

- `HellBoost-gameui-test-x86.zip`: **pentru Options HellBoost în interiorul CS**, kit experimental cu DLL proprie, meniu și instrucțiuni. Necesită biblioteca originală din copia ta de joc, care nu este inclusă; instalează numai într-o copie de test separată de Steam.
- `HellBoost-dev-windows-x86.zip`: **pachetul recomandat**, cu aplicația C++/Win32 `HellBoost.exe`, biblioteca de dezvoltare, testele și sursele. Nu necesită Python.
- `HellBoost-dev-source.zip`: sursele aplicației independente și ale bibliotecii native, fără binare Windows.
- `HellBoost-native-dev-x86.zip`: surse plus DLL și teste de diagnostic, fără EXE-ul browserului; destinat dezvoltării.

Pentru descărcarea unui ZIP din GitHub, deschide fișierul și apasă **Download raw file**. Fiecare arhivă include un manifest SHA-256.

Pachetul Windows include browserul extern propriu, nu clientul final CS 1.6. Nu include jocul, un browser integrat în motor sau raportarea statisticilor live. Nu copia DLL-ul peste bibliotecile CS. `native/abi_test.exe` este numai un test al bibliotecii cu evenimente sintetice, nu un test de conectare în joc.

Extrage pachetul Windows într-un folder separat și pornește `HellBoost/HellBoost.exe`. Apasă **REINCARCA LISTA**, alege un server și testează pornirea jocului. Steam are prioritate; dacă lipsește, aplicația caută sau cere `hl.exe`. Configurația și adaptorul NextClient vechi sunt excluse din aceste arhive.
