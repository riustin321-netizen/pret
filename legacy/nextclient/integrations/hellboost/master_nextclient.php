<?php
declare(strict_types=1);

// Deploy beside the existing launcher.php; this is not the telemetry endpoint.
header('Content-Type: application/json; charset=utf-8');
header('Cache-Control: no-store');
header('X-Content-Type-Options: nosniff');

if (($_SERVER['REQUEST_METHOD'] ?? '') !== 'GET') {
    http_response_code(405);
    header('Allow: GET');
    echo '{"error":"method_not_allowed"}';
    exit;
}

try {
    if (!defined('DB_THROW')) {
        define('DB_THROW', true);
    }
    require __DIR__ . '/inc/db.php';
    require __DIR__ . '/inc/master_nextclient_format.php';

    // ranked_servers owns approval, expiry and tier ordering. No client tier input.
    $entries = hellboost_nextclient_entries(ranked_servers('s.online = 1'));
    echo json_encode($entries, JSON_UNESCAPED_SLASHES | JSON_THROW_ON_ERROR);
} catch (Throwable $error) {
    error_log('[HellBoost HTTP master] ' . get_class($error));
    http_response_code(503);
    header('Retry-After: 30');
    echo '{"error":"temporarily_unavailable"}';
}
