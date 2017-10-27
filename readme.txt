codeclean
-------------------------------------------------------------------------------

Removes comments and empty lines from C-code. Note that comment tags in strings
or macros are still seen as such. Supports multiple files and uses multiple pro-
cesses to handle such input. Files are locked during cleanup and memory mapping
is utilized to get fast character by character reads and writes. 

Usage
-------------------------------------------------------------------------------
Built by simply running make in the directory. Should work^tm on OSX and Linux as
only gcc and some sys-headers are used. Cleaning is performed by supplying files
as arguments. For example:

./codeclean filename directory/*

Cleaned files are written to [filename].clean and a per-file log of operations
will be written to [filename].log. The log will also contain details of errors
that happen in cleaning or relevant file operations while a high level report of
such failures is output to stderr as well so the user is aware of them.

The test program I used is also built and the files I tailored for testing are
included.

Cleanup style
-------------------------------------------------------------------------------
Line comments are cleaned so that the characters between the opening tag and newline
are removed.

Original:
this is some code before // a comment ends the line
the next line also has some code on it

Cleaned:
this is some code before 
the next line also has some code on it

Comment blocks are also removed in a similar way but they naturally cause surrounding
characters to be joined on the same line:
Original:
this is some code before /* comment block interrupts
it and continues on the next line and has */ some other code after it
the next line also has some code on it

Cleaned:
this is some code before  some other code after it
the next line also has some code on it

Lines that would only have whitespace in the cleaned output are considered empty.
As a result, the following examples would also result in empty cleaned output:

// Line begins with a comment
                    // Line only has an indented comment
/* Comment block at the beginning of the line */
                    /* Indented comment block with
                     * spaninning multiple lines */

However, an indented comment block with text following it before a newline will
leave the text indented as the block will be "squashed":

Original:
                    /* This is an indented
                     * comment block with
                     * some code right after
                     * the closing tag */this will be on the level of the opening tag

Cleaned:
                    this will be on the level of the opening tag
