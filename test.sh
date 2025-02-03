#!/bin/bash

curl -v http://localhost:8080/
curl -v http://localhost:8080/css/style.css
curl -v http://localhost:8080/cgi-bin/test.php
curl -v http://localhost:8080/cgi-bin/test.py
curl -v http://localhost:8080/cgi-bin/test.sh
# Create a test file
echo "test content" > test.txt
# Try uploading it
curl -v -X POST -F "file=@test.txt" http://localhost:8080/uploads/
curl -v http://localhost:8080/uploads/
curl -v http://localhost:8080/nonexistent
curl -v http://localhost:7060/
curl -v http://localhost:8080/old-page