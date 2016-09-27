ctags-anubis
=========================

Add support for the Anubis language to [Ctags](http://ctags.sourceforge.net/), support functions and (a subset) of possible type definitions.

This was essentially made because i wanted to use packages which use ctags for the [Atom](https://atom.io/) editor (which have an already good support of the Anubis language by the way with the [linter-anubis](https://atom.io/packages/linter-anubis) and [language-anubis](https://atom.io/packages/language-anubis) package)

Note: If you use Atom and want to install packages that use ctags like [atom-ctags](https://atom.io/packages/atom-ctags) and [symbols-tree-view](https://atom.io/packages/symbols-tree-view) you will need to replace the ctags binaries inside each of these packages by the one you compiled with Anubis support, under Windows you need to replace this file "C:\Users\[your_username]\.atom\packages\[package_name]\vendor\ctags-win32.exe"

To compile ctags with Anubis support, you have most importantly to add "AnubisParser, \" line in "parsers.h" then compile ctags with "anubis.c", basically add "anubis.c" to "SOURCES" in source.mak (after calling ./configure) and do the same for "OBJECTS"... then you replace ctags binaries provided by various packages to have Anubis language support (generally they are inside a "vendor" directory in the package directory)

This may get "simpler" if Anubis language is added to ctags in the future... (:
