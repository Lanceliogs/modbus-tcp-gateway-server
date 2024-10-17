# Modbus TCP Gateway Server
A simple Modbus TCP gateway server tool to share regsiters between processes and systems.

## Dependencies
I use the very good and concise libmodbus library (https://github.com/stephane/libmodbus)
to handle the Modbus part. It simply works.

## Let's keep it simple
The tool spawns a Modbus TCP server that supports up to 10 clients (for now).
The clients just use the holding registers as shared data. Dpo whatever you want with it.
I will add some read-only feature soon.
 