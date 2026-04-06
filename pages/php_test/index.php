<?php
echo "<h1>PHP Webserver Test</h1>";

// Show request method
echo "<p><b>Method:</b> " . $_SERVER['REQUEST_METHOD'] . "</p>";

// Handle GET
echo "<h2>GET Data</h2>";
if (!empty($_GET)) {
    foreach ($_GET as $key => $value) {
        echo htmlspecialchars($key) . " = " . htmlspecialchars($value) . "<br>";
    }
} else {
    echo "No GET data<br>";
}

// Handle POST
echo "<h2>POST Data</h2>";
if (!empty($_POST)) {
    foreach ($_POST as $key => $value) {
        echo htmlspecialchars($key) . " = " . htmlspecialchars($value) . "<br>";
    }
} else {
    echo "No POST data<br>";
}
?>

<hr>

<h2>Send GET Request</h2>
<form method="GET">
    Name: <input type="text" name="name">
    Age: <input type="text" name="age">
    <button type="submit">Send GET</button>
</form>

<h2>Send POST Request</h2>
<form method="POST">
    Name: <input type="text" name="name">
    Age: <input type="text" name="age">
    <button type="submit">Send POST</button>
</form>
