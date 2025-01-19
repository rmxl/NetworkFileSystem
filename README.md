# Commands
### READ
Format: READ <path>

### WRITESYNC
Format: WRITESYNC <path>

### WRITEASYNC
Format: WRITEASYNC <path>

### CREATEFILE
Format: CREATEFILE <path> <name>

### CREATEFOLDER
Format: CREATEFILE <source> <name>

### COPYFILE
Format: COPYFILE <path> <dest>

### COPYFOLDER
Format: COPYFOLDER <path> <dest>

### STREAM
Format : STREAM <path>


# Assumptions

<!-- If a path is being copied it will not be available for reading/writing, assumption written below instead of this --> 
Names of all files and folders are unique

Assumed that the name of a file will always contain a "." and folder will not contain a "." 

Assumed there will be no paths with "backup" in them.

