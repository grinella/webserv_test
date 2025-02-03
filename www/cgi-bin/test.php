#!/usr/bin/php
<?php
header("Content-Type: text/html");
header("X-Powered-By: PHP/CGI");

echo "<html><head><title>PHP CGI Test</title></head><body>";

if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    echo "<h2>POST Data Received:</h2>";
    echo "<pre>";
    print_r($_POST);
    echo "</pre>";
} else {
    echo "<h2>GET Data Received:</h2>";
    echo "<pre>";
    print_r($_GET);
    echo "</pre>";
}

echo "<h2>Server Environment:</h2>";
echo "<pre>";
print_r($_SERVER);
echo "</pre>";
echo "</body></html>";
?>