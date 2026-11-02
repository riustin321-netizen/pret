<?php
declare(strict_types=1);
require __DIR__ . '/../../integrations/hellboost/inc/master_nextclient_format.php';

function check(bool $condition, string $message): void
{
    if (!$condition) {
        throw new RuntimeException($message);
    }
}

$top = ['ip' => '192.0.2.1', 'port' => '27015', 'online' => '1', 'country' => 'ro', 'game_mod' => 'Public'];
$other = ['ip' => '198.51.100.2', 'port' => 27016, 'online' => 1];
$rows = hellboost_nextclient_entries([$top, $other, $top]);
check(count($rows) === 2, 'Duplicate address was not removed');
check($rows[0]['address'] === '192.0.2.1:27015', 'Platform ranking changed');
check($rows[1]['address'] === '198.51.100.2:27016', 'Second entry changed');
check($rows[0]['country'] === 'RO', 'Country not normalized');
check($rows[0]['game_mode'] === 'Public', 'Game mode missing');
check(!isset($rows[1]['country'], $rows[1]['game_mode']), 'Absent metadata fabricated');
check(json_decode(json_encode($rows, JSON_THROW_ON_ERROR), true, 16, JSON_THROW_ON_ERROR) === $rows, 'JSON roundtrip failed');
check(array_is_list($rows), 'Expected a root JSON array, not an object');

foreach ([
    ['ip', '0.0.0.0'], ['ip', '255.255.255.255'], ['ip', '999.1.1.1'],
    ['ip', '::1'], ['ip', 'example.com'], ['port', 0], ['port', 65536],
    ['port', '27015;quit'], ['port', true], ['online', 0],
] as [$field, $value]) {
    $invalid = $top;
    $invalid[$field] = $value;
    check(hellboost_nextclient_entries([$invalid]) === [], 'Invalid record accepted: ' . $field);
}
$invalid = $top;
$invalid['country'] = "RO\n";
$invalid['game_mod'] = '<script>';
$entry = hellboost_nextclient_entries([$invalid])[0];
check(!isset($entry['country'], $entry['game_mode']), 'Invalid metadata accepted');
check(hellboost_nextclient_entries([]) === [], 'Empty list changed');
echo "HTTP master formatter: ordering, deduplication, metadata, JSON layout and invalid records passed.\n";
