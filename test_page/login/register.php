<?php
require "config.php";

$message = "";

if ($_SERVER["REQUEST_METHOD"] === "POST") {

    $username = trim($_POST["username"]);
    $email = trim($_POST["email"]);
    $password = password_hash($_POST["password"], PASSWORD_DEFAULT);

    $uploadDir = "uploads/";
    if (!is_dir($uploadDir)) {
        mkdir($uploadDir, 0755, true);
    }

    $filePath = null;

    if (!empty($_FILES["avatar"]["name"])) {

        if ($_FILES["avatar"]["error"] === 0) {

            if ($_FILES["avatar"]["size"] > 2 * 1024 * 1024) {
                $message = "Image too large (max 2MB).";
            } else {

                $allowedTypes = ["image/jpeg", "image/png", "image/webp"];
                $fileTmp = $_FILES["avatar"]["tmp_name"];
                $fileType = mime_content_type($fileTmp);

                if (in_array($fileType, $allowedTypes)) {

                    $extension = pathinfo($_FILES["avatar"]["name"], PATHINFO_EXTENSION);
                    $newFileName = uniqid("avatar_", true) . "." . $extension;
                    $targetFile = $uploadDir . $newFileName;

                    if (move_uploaded_file($fileTmp, $targetFile)) {
                        $filePath = $targetFile;
                    } else {
                        $message = "Upload failed.";
                    }

                } else {
                    $message = "Only JPG, PNG, WEBP allowed.";
                }
            }
        }
    }

    if (!$message) {
        $stmt = $conn->prepare("INSERT INTO users (username, email, password, file_path) VALUES (?, ?, ?, ?)");
        $stmt->bind_param("ssss", $username, $email, $password, $filePath);

        if ($stmt->execute()) {
            header("Location: login.php");
            exit;
        } else {
            $message = "Username or email already exists.";
        }
    }
}
?>

<!DOCTYPE html>
<html>
<head>
    <title>Register</title>
    <link rel="stylesheet" href="style.css">
    <link href="https://fonts.googleapis.com/css2?family=JetBrains+Mono:wght@400;700&display=swap" rel="stylesheet">
</head>
<body>

<div class="page-background">

    <div class="auth-left slide-in" style="position: relative; z-index: 1;">

        <h1>REGISTER</h1>

        <?php if (!empty($message)): ?>
            <p class="error"><?= htmlspecialchars($message) ?></p>
        <?php endif; ?>

        <form method="POST" enctype="multipart/form-data">

            <input type="text" name="username" placeholder="Username" required>
            <input type="email" name="email" placeholder="Email" required>
            <input type="password" name="password" placeholder="Password" required>
            <input type="file" name="avatar" accept="image/*" required>

            <button type="submit" class="primary-btn">
                CREATE ACCOUNT
            </button>

        </form>

        <div class="login-actions">

            <a href="index.php" class="secondary-link">HOME</a>
            <a href="login.php" class="secondary-link">LOGIN</a>

        </div>

    </div>

</div>

</body>
</html>
