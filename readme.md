# FAT32 Image Reader
This program reads a FAT32 image file and performs operations. <br/><br/>The operations supported are: 
<br/>1. **open (filename):** opens FAT32 file
<br/>2. **close:** closes FAT32 file
<br/>3. **info:** displays bytes per second, sector per cluster, reserved sector count, numFATS, and FATSz32
<br/>4. **ls:** shows all files in current directory
<br/>5. **stat (file):** shows information about a specific file
<br/>6. **cd (directory):** changes directory
<br/>7. **read  (file) (start byte) (end byte):** reads specified bytes of a file
<br/>8. **get (filename):** copies specified file to your computer
<br/>9. **exit:** exits program
