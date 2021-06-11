## Coding standards

This document is a guide, not a rule.  While code may or may not endevour to
follow this guide, exceptions for the sake of laziness will of course be
expected from time to time.  Do not feel constrained by these suggestions.
Feel free to ignore it like I do. :)

In general, try to follow the coding standards as outlined at the following
links, generally with preference to the first link where a concept appears
over the subsequent links.

* [Qt Coding Style](https://wiki.qt.io/Qt_Coding_Style)
* [Qt Coding Conventions](https://wiki.qt.io/Coding_Conventions)
* [Designing Qt-Style C++ APIs](https://web.archive.org/web/20150513045011/https://doc.qt.digia.com/qq/qq13-apis.html)
* [C++ Programming Style Guidelines](http://geosoft.no/development/cppstyle.html)

Another good read for api design is the Little Manual of API Design.

* [The Little Manual of API Design](https://people.mpi-inf.mpg.de/~jblanche/api-design.pdf)

**In general this means:**

Indenting in QtCreator should left on the default Qt preset.  This is K&R
with a few exceptions.  Spaces not tabs.  Try to code within 78 columns but
don't be afraid to overflow for readability.

Data members exposed with getter/setters should internally end with _ when
they share the same name as a getter.  Data members that are not exposed via
getters and setters should not be mangled.

Don't close the parenthesis for a function call on a seperate line if you are
splitting its parameters over more than one line.

Opening braces inside a function are on the same line.  There is no need to
use a brace inside an if statement if there's only one statment there; adding
it would introduce unnecessary visual noise.

Opening braces of a function are on a new line, since they introduce code.
Opening braces of lambdas are on the same line.

Use two spaces after a period in comments.

Simple enums can be placed on the same line.

## Writing a module

Avoid macros, use C++11 (or better) features instead.

Put constants at the top of a file.

Put variables after constants.

Helper functions should be before regular functions if used by more than one
function.  Helper functions that are used by only one function should be just
before where it is used or a lambda inside the function itself.  The
rationale is that mirroring a function definition with a declaration can lead
to runtime errors.

Blocks of code (between variables, functions, classes and so on) should be
seperated by three lines.

Logic blocks inside functions should be separated by a line, although feel
free to ignore this at your discretion if the function is straightforward
or small.

Local variables do not need to be declared at the beginning of a function.

Reuse code.  If you need a logic block that appears in more than one
function, do not copy-paste it, write a function for it and call it in the
functions you need it.

If there's a piece of logic in the code that seemed nonobvious to you or
occurred to you after a period of time, write a comment about it.  Chances
are others won't work it out in five minutes if it took you half a day to
write.

Use scope to auto delete an object if you can.  For variables allocated with
new, write your delete command in the same commit.

## Classes

Order class declarations in order of enums, public, protected, private.  Put
functions before data members.  Second, the order tends to be functions
signals slots data, with the quirk that all data members should be at the
bottom of the class.

Unless doing so would obscure implementation, this loosely gives you the
following way of ordering a class:

1. enums/typedefs/etc
2. public
3. signals
4. protected
5. private
6. data members

Adding a slot from the ui designer will add an entry at at the bottom of the
cpp/header file; this should be moved to appropriate location.

Consider using your own getters and setters if data members are exposed by
them and particular logic needs to happen at the time.

## Git commits

Provide insightful comments if possible.

Commit often, but don't push a block of code until it is workable.  In
particular, the git head should compile and work, even if a feature is
missing or not exposed yet.  But see below.

Don't be afraid to push/commit even if it is "incomplete", especially so if
it has gotten long in the tooth since your last push, or you're up to a stage
where part of the feature is done but not all of it is done.  What's
important is if it is up to a state where others can work on it *and* the
user won't notice that the code base is getting improved.  But do plan to do
the rest of the work soon after a long-in-the-tooth push if possible.

Break up your commits where possible.  It's okay to have a partially working
module halfway through a block of patches, because it's only the head that
matters.  This lets other developers see how you worked on the project.  More
on this below.

When working out a new algorithm, window, or a Qt feature that you are
unfamiliar with, do not pollute the project with your own commits, work on it
in a separate directory (outside of the git repo) or branch.  Then merge when
it is in a workable state.  After this it can be brought up to scratch
because further development nearly always exposes unforseen oversights, so
while you should endevour to get it right, don't put an extreme amount of
effort into it.

Futhermore, it doesn't matter if you got something Right(TM) the first time,
as long as it seemed right enough when you did it.  Code evolves, who cares.
