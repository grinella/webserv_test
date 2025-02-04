<?php
header("Content-Type: text/html");
error_reporting(E_ALL);
ini_set('display_errors', 1);

if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    if (isset($_FILES['file'])) {
        $uploaddir = '../uploads/';
        if (!is_dir($uploaddir)) {
            mkdir($uploaddir, 0755, true);
        }
        
        $uploadfile = $uploaddir . basename($_FILES['file']['name']);
        
        if (move_uploaded_file($_FILES['file']['tmp_name'], $uploadfile)) {
            echo "<html><head><title>Upload Success</title><link rel='stylesheet' type='text/css' href='/css/style.css'></head>";
            echo "<body><div class='container'><h1>File uploaded successfully</h1>";
            echo "<p>File saved as: " . htmlspecialchars(basename($uploadfile)) . "</p>";
            echo "<a href='/'>Back to Home</a></div></body></html>";
        } else {
            echo "<html><head><title>Upload Failed</title><link rel='stylesheet' type='text/css' href='/css/style.css'></head>";
            echo "<body><div class='container'><h1>Upload failed</h1>";
            echo "<p>Error: " . htmlspecialchars($_FILES['file']['error']) . "</p>";
            echo "<a href='/cgi-bin/upload.php'>Try Again</a></div></body></html>";
        }
    } else {
        echo "<html><head><title>Upload Failed</title><link rel='stylesheet' type='text/css' href='/css/style.css'></head>";
        echo "<body><div class='container'><h1>Upload failed</h1>";
        echo "<p>No file selected.</p>";
        echo "<a href='/cgi-bin/upload.php'>Try Again</a></div></body></html>";
    }
} else {
?>
<!DOCTYPE html>
<html>
<head>
    <title>File Upload</title>
    <link rel="stylesheet" type="text/css" href="/css/style.css">
</head>
<body>
    <div class="home-button">
        <a href="/">Home</a>
    </div>
    <div class="container">
        <h1>Upload a File</h1>
        <form action="/cgi-bin/upload.php" method="POST" enctype="multipart/form-data">
            <label for="file">Choose a file to upload:</label>
            <input type="file" id="file" name="file" required>
            <button type="submit">Upload File</button>
        </form>
    </div>
</body>
</html>
<?php
}
?>