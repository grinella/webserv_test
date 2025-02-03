#!/usr/bin/python3
import os
import sys
import cgi

print("Content-Type: text/html\n")

print("<html><body>")
print("<h1>Python CGI Test</h1>")

method = os.environ.get('REQUEST_METHOD', '')

if method == 'POST':
    form = cgi.FieldStorage()
    print("<h2>POST Data:</h2>")
    for key in form.keys():
        print("<p>%s: %s</p>" % (key, form.getvalue(key)))
elif method == 'GET':
    print("<h2>GET Data:</h2>")
    query_string = os.environ.get('QUERY_STRING', '')
    print("<p>Query string: %s</p>" % query_string)

print("<h2>Environment Variables:</h2>")
print("<pre>")
for key, value in os.environ.items():
    print("%s: %s" % (key, value))
print("</pre>")
print("</body></html>")