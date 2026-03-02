<?php
require "config.php";

$message = "";

if ($_SERVER["REQUEST_METHOD"] === "POST") {

    $username = trim($_POST["username"]);
    $password = $_POST["password"];

    $stmt = $conn->prepare("SELECT id, username, password FROM users WHERE username = ?");
    $stmt->bind_param("s", $username);
    $stmt->execute();
    $result = $stmt->get_result();

    if ($user = $result->fetch_assoc()) {

        if (password_verify($password, $user["password"])) {
            $_SESSION["user_id"] = $user["id"];
            header("Location: index.php?id=" . $user["id"]);
            exit;
        }
    }

    $message = "Invalid credentials.";
}
?>

<!DOCTYPE html>
<html>
<head>
    <title>Login</title>
    <link rel="stylesheet" href="style.css">
    <link href="https://fonts.googleapis.com/css2?family=JetBrains+Mono:wght@400;700&display=swap" rel="stylesheet">
</head>
<body>

<div class="page-background">

    <div class="login-form slide-in">

        <h1>LOGIN</h1>

        <?php if (!empty($message)): ?>
            <p class="error"><?= htmlspecialchars($message) ?></p>
        <?php endif; ?>

        <form method="POST">

            <input type="text" name="username" placeholder="Username" required>
            <input type="password" name="password" placeholder="Password" required>

            <button type="submit">ENTER</button>

        </form>

        <!-- Navigation Links -->
        <div class="login-actions">

            <a href="index.php" class="secondary-link">HOME</a>
            <a href="register.php" class="secondary-link">REGISTER</a>

        </div>

    </div>

</div>

</body>
</html>
