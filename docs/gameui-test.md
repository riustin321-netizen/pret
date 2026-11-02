# Opțiuni HellBoost în interiorul CS — kit experimental x86

Acest kit adaugă intrarea **Options HellBoost** în meniul jocului. Ea deschide un dialog în procesul CS, cu bifa **Integrare Steam**. **Options CS** deschide în continuare opțiunile clasice. Nu este o pagină Misc copiată din alt client și nu necesită NextClient.

## Ce face bifa

Preferința este salvată în profilul utilizatorului, `HKEY_CURRENT_USER\Software\HellBoost\PreferSteam`, și este citită de launcherul HellBoost la următoarea pornire. Bifată acordă prioritate instalării Steam oficiale; debifată selectează instalarea locală. Nu schimbă autentificarea unui proces deja pornit și nu transformă jocul Steam într-un client fără autentificare. Nu există emulator Steam în kit. Jocul nu este repornit automat.

## Instalare numai într-o copie de test

Nu modifica folderul administrat de Steam. Fă o copie separată a instalării tale pentru test; compatibilitatea în joc și cu VAC nu a fost validată. Închide jocul înainte de a schimba fișierele. Kitul nu include jocul sau DLL-ul Valve original.

1. În copia de test, găsește biblioteca originală `GameUI.dll`. Ținta analizată are SHA-256 `684fb5615f0c3ab7d07fcf5503eb214d064c70e25eb0eb22bbf043b4d73c162f`. Dacă ai altă bibliotecă, nu presupune compatibilitatea doar din numele `GameUI007`.
2. Pregătește folderul `cstrike/cl_dlls` în copia de test. Dacă există deja `GameUI.dll` aici, păstreaz-o separat și redenumește copia din acest folder în **HellBoost.GameUI.original.dll**. Dacă biblioteca originală se află numai în `valve/cl_dlls`, copiaz-o în `cstrike/cl_dlls` cu numele **HellBoost.GameUI.original.dll**, lăsând originalul Valve neatins. Nu suprascrie un backup existent.
3. Copiază DLL-ul HellBoost din `experimental/gameui/GameUI.dll` în `cstrike/cl_dlls/GameUI.dll`.
4. Păstrează separat fișierul existent `cstrike/resource/GameMenu.res`, dacă există. Copiază `experimental/gameui/GameMenu.res` în acel loc, creând directorul dacă lipsește. Acesta este un override în copia de test, nu o modificare a resurselor Valve originale.
5. Pornește copia de test prin metoda pe care o foloseai deja. În meniu selectează **Options HellBoost**, modifică bifa și apasă **Salvează**. La următoarea pornire prin noul `HellBoost.exe`, preferința va fi respectată. **Anulează** nu o salvează.

Pornirea din Steam prin App ID 10 selectează instalarea administrată de Steam, nu copia separată de test. Acest kit nu redirecționează automat Steam către copia modificată și nu furnizează un sistem nou de autentificare. Dacă treci la o altă instalare, meniul ei este cel instalat în acea copie.

## Revenire

Cu jocul închis, elimină DLL-ul HellBoost din `cstrike/cl_dlls/GameUI.dll`. Dacă aveai un GameUI original în acel folder, restaurează-l din backup sub numele original. Dacă originalul exista numai în `valve/cl_dlls`, nu modifica acel fișier: elimină override-ul și copia `HellBoost.GameUI.original.dll` creată pentru test. Restaurează `GameMenu.res` păstrat separat sau elimină override-ul dacă înainte nu exista. Păstrează backupurile până ai verificat revenirea.

## Verificări și limitări

Puntea HellBoost expune `CreateInterface`, păstrează interfața `GameUI007` și transmite cele 21 de metode către biblioteca originală, inclusiv validarea/autentificarea originală. Adaugă numai comanda `hellboost_options`. Nu modifică instrucțiuni din `hw.dll` și nu înlocuiește logica de autentificare.

Testele Windows folosesc o bibliotecă originală simulată, creată din cod propriu, pentru a verifica toate metodele, convențiile de apel MSVC x86 și dialogul cu checkbox. Sunt testate separat varianta construită cu MSVC și cea MinGW distribuită. Panoul este modeless: comanda revine fără o buclă modală care ar bloca frame-urile și rețeaua motorului. Aceste teste nu înlocuiesc pornirea CS cu DLL-ul Valve real; pomparea mesajelor, încadrarea și focusul dialogului în fullscreen trebuie verificate pe calculatorul de test. În caz de blocare sau meniu absent, revino la backup, nu înlocui alte DLL-uri și nu dezactiva protecțiile sistemului.

Lista HellBoost în Find Servers și raportarea timpului real de conectare nu sunt implementate de acest kit. Dialogul este o extensie proprie în procesul jocului, nu o modificare a paginilor private ale dialogului clasic Options.
