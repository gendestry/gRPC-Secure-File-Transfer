#!/bin/sh
# https://stackoverflow.com/questions/40623793/use-ssl-in-grpc-client-server-communication

country="US"
location="NewYork"
mypass="securepassword"
org="Organization"
org_unit = "Org-Unit"

echo Generate server key:
openssl genrsa -passout pass:$mypass -des3 -out server.key 4096

echo Generate server signing request:
openssl req -passin pass:$mypass -new -key server.key -out server.csr -subj  "/C=$country/L=$location/O=$org/OU=$org_unit/CN=localhost"

echo Self-sign server certificate:
openssl x509 -req -passin pass:$mypass -days 365 -in server.csr -signkey server.key -set_serial 01 -out server.crt

echo Remove passphrase from server key:
openssl rsa -passin pass:$mypass -in server.key -out server.key

rm server.csr
