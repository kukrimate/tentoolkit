# tentoolkit
Create Windows 10 installation images under Linux from Microsoft ESD files.

## usage
This tool is build up from 3 parts.
- The sh script `esddl` can be used to download ESD files from the MS catalog
- The C program `mkmedia` is used for creating a Windows installer tree from that
- Finall if you want to install from a DVD you can use `buildiso` to build a DVD image
