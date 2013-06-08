pi_nfc_remote
=============

A small client and server NFC interface, utilizing libnfc.

The client is supposed to have a NFC reader attached, which he polls for
NFC tags. If a tag is found the client establishes a connection to the
server which then decides how to handle the incoming connection.

In this version the server spawns firefox on every incoming connection as
a sheer demo. This can, of course, be extended to any desired behavior.
