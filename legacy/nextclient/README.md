# Integrare istorică, retrasă din pachetul principal

Aceste fișiere au fost pregătite anterior pentru interoperabilitate cu un NextClient existent. Sunt păstrate pentru istoric și nu sunt o dependență HellBoost, nu sunt instalate și nu sunt incluse de `tools/package.py`.

Aplicația principală folosește API-ul HellBoost existent (`/api.php`) direct. Vezi README-ul din rădăcina repository-ului pentru configurația actuală.

Pentru verificarea izolată a vechiului formatter se poate rula `php -n legacy/nextclient/tests/php/master_format_test.php`. Documentația din `reference.md` descrie starea vechii investigații, nu un pachet actual de client.
