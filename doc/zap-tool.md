---
layout: page
title: The zap Tool
---

# The `zap` Tool

Zap comes with a command-line tool called `zap` intended to aid development and
debugging.  This tool can be used to:

* Compile Zap schemas to produce source code in multiple languages.
* Generate unique type IDs.
* Decode Zap messages to human-readable text.
* Encode text representations of Zap messages to binary.
* Evaluate and extract constants defined in Zap schemas.

This page summarizes the functionality.  A complete reference on the command's usage can be
found by typing:

    zap help

## Compiling Schemas

    zap compile -oc++ myschema.zap

This generates files `myschema.zap.h` and `myschema.zap.c++` which contain C++ source code
corresponding to the types defined in `myschema.zap`.  Options exist to control output location
and import paths.

The above example generates C++ code, but the tool is able to generate output in any language
for which a plugin is available.  Compiler plugins are just regular programs named
`zapc-language`.  For example, the above command runs `zapc-c++`.  [More on how to write
compiler plugins](otherlang.html#how-to-write-compiler-plugins).

Note that some Zap implementations (especially for interpreted languages) do not require
generating source code.

## Decoding Messages

    zap decode myschema.zap MyType < message.bin > message.txt

`zap decode` reads a binary Zap message from standard input and decodes it to a
human-readable text format (specifically, the format used for specifying constants and default
values in [the schema language](language.html)).  By default it
expects an unpacked message, but you can decode a
[packed](encoding.html#packing) message with the `--packed` flag.

## Encoding Messages

    zap encode myschema.zap MyType < message.txt > message.bin

`zap encode` is the opposite of `zap decode`: it takes a text-format message on stdin and
encodes it to binary (possibly [packed](encoding.html#packing),
with the `--packed` flag).

This is mainly useful for debugging purposes, to build test data or to apply tweaks to data
decoded with `zap decode`.  You should not rely on `zap encode` for encoding data written
and maintained in text format long-term -- instead, use `zap eval`, which is much more powerful.

## Evaluating Constants

    zap eval myschema.zap myConstant

This prints the value of `myConstant`, a [const](language.html#constants) declaration, after
applying variable substitution.  It can also output the value in binary format (`--binary` or
`--packed`).

At first glance, this may seem no more interesting than `zap encode`:  the syntax used to define
constants in schema files is the same as the format accepted by `zap encode`, right?  There is,
however, a big difference:  constants in schema files may be defined in terms of other constants,
which may even be imported from other files.

As a result, `zap eval` is a great basis for implementing config files.  For example, a large
company might maintain a production server that serves dozens of clients and needs configuration
information about each one.  Rather than maintaining the config as one enormous file, it can be
written as several separate files with a master file that imports the rest.

Such a configuration should be compiled to binary format using `zap eval` before deployment,
in order to verify that there are no errors and to make deployment easier and faster.  While you
could technically ship the text configs to production and have the servers parse them directly
(e.g. with `zap::SchemaParser`), encoding before deployment is more efficient and robust.
