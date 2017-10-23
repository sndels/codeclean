# codeclean

A simple course project that removes comments and empty lines from C-code. Note that comment tags in strings or macros are still seen as such. Supports multiple files and uses multiple processes to handle such input. Corresponding files are locked cleanup and memorymapping is utilized to get fast character by character reads and writes.

## Usage
Built by simply running `make` in the directory. Should work™ on OSX and Linux as only gcc and some sys-headers are used. Cleaning is performed by supplying files as arguments. For example:
```
./codeclean filename directory/*
```
Cleaned files are written to `[filename].clean` and a per-file log of operations will go to `[filename].log`. The log will also contain details of errors that happen in cleaning or relevant file operations while a high level report of such failures is output to stderr as well.
