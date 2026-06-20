---
layout: page
title: Other Languages
---

# Other Languages

Zap's reference implementation is in C++.  Implementations in other languages are
maintained by respective authors and have not been reviewed by me
([@kentonv](https://github.com/kentonv)). Below are the implementations I'm aware
of. Some of these projects are more "ready" than others; please consult each
project's documentation for details.

##### Serialization + RPC

* [C++](cxx.html) by [@kentonv](https://github.com/kentonv)
* [C#](https://github.com/c80k/zap-dotnetcore) by [@c80k](https://github.com/c80k)
* [Erlang](http://ezap.astekk.se/) by [@kaos](https://github.com/kaos)
* [Go](https://github.com/zap/go-zap) currently maintained by [@zenhack](https://github.com/zenhack) and [@lthibault](https://github.com/lthibault)
* [Haskell](https://github.com/zenhack/haskell-zap) by [@zenhack](https://github.com/zenhack)
* [JavaScript (Node.js only)](https://github.com/zap/node-zap) by [@kentonv](https://github.com/kentonv)
* [OCaml](https://github.com/zap/zap-ocaml) by [@pelzlpj](https://github.com/pelzlpj) with [RPC](https://github.com/mirage/zap-rpc) by [@talex5](https://github.com/talex5)
* [Python](http://zap.github.io/pyzap/) by [@jparyani](https://github.com/jparyani)
* [Rust](https://github.com/dwrensha/zap-rust) by [@dwrensha](https://github.com/dwrensha)

##### Serialization only

* [C](https://github.com/opensourcerouting/c-zap) by [OpenSourceRouting](https://www.opensourcerouting.org/) / [@eqvinox](https://github.com/eqvinox) (originally by [@jmckaskill](https://github.com/jmckaskill)) (no longer maintained)
    * [Forked and maintained](https://gitlab.com/dkml/ext/c-zap) by [@jonahbeckford](https://github.com/jonahbeckford)
* [D](https://github.com/zap/zap-dlang) by [@ThomasBrixLarsen](https://github.com/ThomasBrixLarsen)
* [Java](https://github.com/zap/zap-java/) by [@dwrensha](https://github.com/dwrensha)
* [JavaScript](https://github.com/zap-js/plugin/) by [@popham](https://github.com/popham)
* [JavaScript](https://github.com/jscheid/zap-js) (older, abandoned) by [@jscheid](https://github.com/jscheid)
* [Lua](https://github.com/cloudflare/lua-zap) by [Cloudflare](http://www.cloudflare.com/) / [@calio](https://github.com/calio)
* [Nim](https://github.com/zielmicha/zap.nim) by [@zielmicha](https://github.com/zielmicha)
* [Ruby](https://github.com/cstrahan/zap-ruby) by [@cstrahan](https://github.com/cstrahan)
* [Scala](https://github.com/katis/zap-scala) by [@katis](https://github.com/katis)

##### Tools

These are other misc projects related to Zap that are not actually implementations in
new languages.

* [Common Test Framework](https://github.com/kaos/zap_test) by [@kaos](https://github.com/kaos)
* [Sublime Syntax Highlighting](https://github.com/joshuawarner32/zap-sublime) by
  [@joshuawarner32](https://github.com/joshuawarner32)
* [Vim Syntax Highlighting](https://github.com/cstrahan/vim-zap) by [@cstrahan](https://github.com/cstrahan)
* [Wireshark Dissector Plugin](https://github.com/kaos/wireshark-plugins) by [@kaos](https://github.com/kaos)
* [VS Code Syntax Highlighter](https://marketplace.visualstudio.com/items?itemName=xmonader.vscode-zap) by [@xmonader](https://github.com/xmonader)
* [IntelliJ Syntax Highlighter](https://github.com/xmonader/serzap) by [@xmonader](https://github.com/xmonader)
* [RPC Tracer](https://github.com/Toyota/zap-trace) by [@t-kondo-tmc](https://github.com/t-kondo-tmc)
* [Flow-IPC](https://github.com/Flow-IPC) (shared memory IPC transport in C++) by [Akamai](https://www.akamai.com) / [@ygoldfeld](https://github.com/ygoldfeld)
* [Zap Language Server](https://github.com/trickstar0301/zap-ls) by [@trickstar0301](https://github.com/trickstar0301)

## Contribute Your Own!

We'd like to support many more languages in the future!

If you'd like to own the implementation of Zap in some particular language,
[let us know](https://groups.google.com/group/zap)!

**You should e-mail the list _before_ you start hacking.**  We don't bite, and we'll probably have
useful tips that will save you time.  :)

**Do not implement your own schema parser.**  The schema language is more complicated than it
looks, and the algorithm to determine offsets of fields is subtle.  If you reuse the official
parser, you won't risk getting these wrong, and you won't have to spend time keeping your parser
up-to-date.  In fact, you can still write your code generator in any language you want, using
compiler plugins!

### How to Write Compiler Plugins

The Zap tool, `zap`, does not actually know how to generate code.  It only parses schemas,
then hands the parse tree off to another binary -- known as a "plugin" -- which generates the code.
Plugins are independent executables (written in any language) which read a description of the
schema from standard input and then generate the necessary code.  The description is itself a
Zap message, defined by
[schema.zap](https://github.com/zap/zap/blob/master/c%2B%2B/src/zap/schema.zap).
Specifically, the plugin receives a `CodeGeneratorRequest`, using
[standard serialization](encoding.html#serialization-over-a-stream)
(not packed).  (Note that installing the C++ runtime causes schema.zap to be placed in
`$PREFIX/include/zap` -- `/usr/local/include/zap` by default).

Of course, because the input to a plugin is itself in Zap format, if you write your
plugin directly in the language you wish to support, you may have a bootstrapping problem:  you
somehow need to generate code for `schema.zap` before you write your code generator.  Luckily,
because of the simplicity of the Zap format, it is generally not too hard to do this by
hand.  Remember that you can use `zap compile -ozap schema.zap` to get a dump of the sizes
and offsets of all structs and fields defined in the file.

`zap compile` normally looks for plugins in `$PATH` with the name `zapc-[language]`, e.g.
`zapc-c++` or `zapc-zap`.  However, if the language name given on the command line contains
a slash character, `zap` assumes that it is an exact path to the plugin executable, and does not
search `$PATH`.  Examples:

    # Searches $PATH for executable "zapc-mylang".
    zap compile -o mylang addressbook.zap

    # Uses plugin executable "myplugin" from the current directory.
    zap compile -o ./myplugin addressbook.zap

If the user specifies an output directory, the compiler will run the plugin with that directory
as the working directory, so you do not need to worry about this.

For examples of plugins, take a look at
[zapc-zap](https://github.com/zap/zap/blob/master/c%2B%2B/src/zap/compiler/zapc-zap.c%2B%2B)
or [zapc-c++](https://github.com/zap/zap/blob/master/c%2B%2B/src/zap/compiler/zapc-c%2B%2B.c%2B%2B).

### Supporting Dynamic Languages

Dynamic languages have no compile step.  This makes it difficult to work `zap compile` into the
workflow for such languages.  Additionally, dynamic languages are often scripting languages that do
not support pointer arithmetic or any reasonably-performant alternative.

Fortunately, dynamic languages usually have facilities for calling native code.  The best way to
support Zap in a dynamic language, then, is to wrap the C++ library, in particular the
[C++ dynamic API](cxx.html#dynamic-reflection).  This way you get reasonable performance while
still avoiding the need to generate any code specific to each schema.

To parse the schema files, use the `zap::SchemaParser` class (defined in `zap/schema-parser.h`).
This way, schemas are loaded at the same time as all the rest of the program's code -- at startup.
An advanced implementation might consider caching the compiled schemas in binary format, then
loading the cached version using `zap::SchemaLoader`, similar to the way e.g. Python caches
compiled source files as `.pyc` bytecode, but that's up to you.

### Testing Your Implementation

The easiest way to test that you've implemented the spec correctly is to use the `zap` tool
to [encode](zap-tool.html#encoding-messages) test inputs and
[decode](zap-tool.html#decoding-messages) outputs.
