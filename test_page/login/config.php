<?php
session_start();

$host = "127.0.0.1";   // IMPORTANT: use 127.0.0.1 not localhost
$user = "devuser";
$pass = "StrongPassword123!";
$db   = "beautiful_login";

mysqli_report(MYSQLI_REPORT_ERROR | MYSQLI_REPORT_STRICT);

try {
    $conn = new mysqli($host, $user, $pass, $db);
    $conn->set_charset("utf8mb4");
} catch (Exception $e) {
    die("Connection failed: " . $e->getMessage());
}
?>
