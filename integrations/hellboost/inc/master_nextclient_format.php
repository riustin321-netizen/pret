<?php
declare(strict_types=1);

/** Format approved, ranked database rows for NextClient's HTTP master parser. */
function hellboost_nextclient_entries(iterable $rows): array
{
    $entries = [];
    $seen = [];
    foreach ($rows as $row) {
        if (!is_array($row) || !in_array($row['online'] ?? null, [true, 1, '1'], true)) {
            continue;
        }
        $ip = $row['ip'] ?? null;
        $port = $row['port'] ?? null;
        if (!is_string($ip) || filter_var($ip, FILTER_VALIDATE_IP, FILTER_FLAG_IPV4) === false
            || $ip === '0.0.0.0' || $ip === '255.255.255.255') {
            continue;
        }
        if ((!is_int($port) && !is_string($port)) || !preg_match('/^[0-9]{1,5}$/D', (string)$port)
            || (int)$port < 1 || (int)$port > 65535) {
            continue;
        }
        $address = $ip . ':' . (int)$port;
        if (isset($seen[$address])) {
            continue;
        }
        $seen[$address] = true;
        $entry = ['address' => $address];
        $country = $row['country'] ?? '';
        if (is_string($country) && preg_match('/^[A-Za-z]{2}$/D', $country)) {
            $entry['country'] = strtoupper($country);
        }
        $mode = $row['game_mod'] ?? '';
        if (is_string($mode) && preg_match('/^[A-Za-z0-9_]{1,31}$/D', $mode)) {
            $entry['game_mode'] = $mode;
        }
        $entries[] = $entry;
        if (count($entries) > 5000) {
            throw new LengthException('HTTP master exceeds configured server limit');
        }
    }
    return $entries;
}
