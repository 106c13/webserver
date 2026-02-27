<?php

if ($_SERVER["REQUEST_METHOD"] === "GET") {

    $name = isset($_GET["name"]) ? $_GET["name"] : "";
    $age  = isset($_GET["age"])  ? $_GET["age"]  : "";

    echo "Method: GET\n";
    echo "Name: " . htmlspecialchars($name) . "\n";
    echo "Age: " . htmlspecialchars($age) . "\n";

} else {
    echo "Only GET method is allowed.";
}

?>
