<?php

if ($_SERVER["REQUEST_METHOD"] === "POST") {

    $name = isset($_POST["name"]) ? $_POST["name"] : "";
    $age  = isset($_POST["age"]) ? $_POST["age"] : "";

    echo "<h2>Form Submitted</h2>";
    echo "Name: " . htmlspecialchars($name) . "<br>";
    echo "Age: " . htmlspecialchars($age) . "<br>";

} else {
?>

<!DOCTYPE html>
<html>
<head>
    <title>Simple POST Form</title>
</head>
<body>
    <h2>Send POST Data</h2>
    <form method="POST" action="">
        Name: <input type="text" name="name"><br><br>
        Age: <input type="number" name="age"><br><br>
        <input type="submit" value="Submit">
    </form>
</body>
</html>

<?php
}
?>
