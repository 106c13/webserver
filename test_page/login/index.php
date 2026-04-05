<?php
require "config.php";

$user = null;

if (isset($_GET["id"]) && is_numeric($_GET["id"])) {

    $stmt = $conn->prepare("SELECT username, email, file_path FROM users WHERE id = ?");
    $stmt->bind_param("i", $_GET["id"]);
    $stmt->execute();
    $result = $stmt->get_result();
    $user = $result->fetch_assoc();
}
?>

<!DOCTYPE html>
<html>
<head>
    <title>Webserv</title>
    <link rel="stylesheet" href="style.css">
    <link href="https://fonts.googleapis.com/css2?family=JetBrains+Mono:wght@400;700&display=swap" rel="stylesheet">
</head>
<body>

<div class="page-background">

    <div style="position: relative; z-index: 1; margin-left: 10%;" class="slide-in">

        <?php if ($user): ?>

            <!-- USER PAGE -->

            <h1>Profile</h1>

            <?php if (!empty($user["file_path"])): ?>
                <img src="<?= htmlspecialchars($user["file_path"]) ?>"
                     style="width:120px;height:120px;border-radius:50%;margin:20px 0;border:2px solid #F54320;">
            <?php endif; ?>

            <p><strong>Username:</strong> <?= htmlspecialchars($user["username"]) ?></p>
            <p><strong>Email:</strong> <?= htmlspecialchars($user["email"]) ?></p>

            <br>

            <a href="index.php" class="secondary-link">BACK TO HOME</a>

        <?php else: ?>

            <!-- LANDING PAGE -->

            <h1>Webserv</h1>

            <p style="max-width: 600px; margin-bottom: 20px;">
                Custom HTTP/1.1 server written in C++ using sockets, epoll,
                and CGI support. Built as a demonstration project for 42.
            </p>

            <a href="login.php" class="secondary-link">GET STARTED</a>

        <?php endif; ?>

    </div>

    <!-- Top Right Login Button (only if not logged view) -->
    <?php if (!$user): ?>
        <div style="position: absolute; top: 20px; right: 60px; z-index: 1;">
            <a href="login.php" class="secondary-link">LOGIN</a>
        </div>
    <?php endif; ?>

</div>

</body>
</html>
