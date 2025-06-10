# C-based Agent Behavior Specification Language (CABSL)

The *C-based Agent Behavior Specification Language* (CABSL)
[[1]](#references) allows specifying the behavior of a robot or a software
agent in C++11. Semantically, it follows the ideas of the *Extensible Agent 
Behavior Specification Language* [XABSL](http://www.xabsl.de) developed by
Martin Lötzsch, Max Risler, and Matthias Jüngel, but its integration into a
C++ program requires significantly less programming overhead. The actual
implementation of CABSL is only contained in tree files, which can be found
in the directory *include* of this distribution. In addition, there is a
script file in the directory *bin* that can be used to visualize the
behavior. All other source files are just part of an example on how to use
CABSL.

CABSL is also part of all B-Human Code Releases since 2013. The difference
between the version that comes with the B-Human software and the version
in this distribution is that the former is tightly integrated with
B-Human's streaming architecture, while the version in this release uses
the standard *iostream* library instead.

This is the second version of CABSL, CABSL 2. In comparison to CABSL 1, it
adds the ability to split the behavior into multiple compilation units, it
uses a different syntax for passing arguments, and it adds constant
definitions as well as state variables.


## Approach

A robot control program is executed in cycles. In each cycle, the agent
acquires new data from the environment, e.g. through sensors, runs its
behavior, and then executes the commands the behavior has computed, i.e.
the agent acts. This means that a robot control program is a big loop, but
the behavior is just a mapping from the state of the world to actions that
also considers what decisions it has made before.


### Options

CABSL describes the behavior of an agent as a hierarchy of finite state
machines, the so-called *options*. Each *option* consists of a number of
*states*. Each state can define transitions to other states within the
same option as well as the *actions* that will be executed if the option
is in that state. One of the possible actions is to call another option,
which lets all options form a directed acyclic graph. In each execution
cycle, a subset of all options is executed, i.e. the options that are
reachable from the root option through the actions of their current
states. This current set of options is called the *activation graph*
(actually, it is a tree). Starting from the root option, each option
switches its current state if necessary and then it executes the actions
listed in the state that is then active. The actions can set output
values or they can call other options, which again might switch their
current state followed by the execution of actions. Per execution cycle,
each option switches its current state at most once.

It is possible to define options that do not have any state. However,
they will be treated as normal functions. For instance, they will not be
shown as part of the activation graph.


### States

Options can have a number of states. One of them is the *initial state*,
in which the option starts when it is called for the first time and to
which it falls back if it is executed again, but was not executed in the
previous cycle, because no other option called it. There are two other
kinds of special states, namely *target states* and *aborted states*.
They are intended to signify to the calling option that the current
option has either succeeded or failed. The calling option can determine,
whether the last sub-option it called has reached one of these states.
It is up to the programmer to define what succeeding or failing actually
mean, i.e. under which conditions these states are reached.


### Transitions

States can have *transitions* to other states. They are basically
decision trees, in which each leaf contains a `goto`-statement to another
state. If none of these leafs is reached, the option stays in its current
state.

An option can have a *common transition*. Its decision tree is executed
before the decision tree in the current state. The decision tree of the
current state functions as the `else`-branch of the decision tree in the
common transition. If the common transition already results in a change
of the current state, the transitions defined in the states are not
checked anymore. In general, common transitions should be used sparsely,
because they conflict with the general idea that each state has its own
conditions under which the state is left.


### Actions

States have *actions* that are executed if the option is in that state.
They set output symbols and/or call other options. Although any C++
statements can be used in an action block, it is best to avoid control
statements, i.e. branching and loops. Branching is better handled by state
transitions and loops should not be used in the behavior, but rather in
functions that are are executed before or after the behavior or that are
called from within the behavior. It is allowed to call more than one
sub-option from a single state, but only the last one called can be
checked for having reached a target state or an aborted state.


### Symbols

All options have access to the member variables of the class in the body
of which they are included. These member variables function as *input
symbols* and *output symbols*. Input symbols are usually used in the
conditions for state transitions. Output symbols are set in actions.

There are four predefined symbols that can be used in the conditions for
state transitions:

  - The `option_time` is the time the option was continuously active, i.e.
    the time since the first execution cycle it became active without being
    inactive afterwards. The unit of the time depends on the time values
    passed to the behavior engine (see section [Example](#example)).

  - The `state_time` is the time the option was continuously in the same
    state, i.e. the time since the first execution cycle it switched to
    that state without switching to another one afterwards. The
    `state_time` is also reset when the option becomes inactive, i.e. the
    `state_time` can never be bigger than the `option_time`.

  - `action_done` is true if the last sub-option executed in the previous
    cycle was in a *target state*.

  - `action_aborted` is true if the last sub-option executed in the
    previous cycle was in an *aborted state*.


### Arguments

Options can have *arguments*. From the perspective of the option, they are
not different from input symbols. As in C++, arguments hide member
variables of the surrounding class with the same name. When calling an
option, its actual arguments are passed to it as they would be in C++.

However, when calling the option, a special syntax is used. Technically,
all arguments are passed as a structure initialized using the C++ construct
*designated initializers*. For instance, if an option `go_to` has two
integer arguments `x` and `y`, it can be called with
`go_to({.x = 1, .y = 2})`. This makes it more clear which value is
passed to which argument. If default values are specified for arguments in
the declaration of the option, these arguments can be left out when calling
the option.


### Constant Definitions

Transitions and actions in options do not only depend on option arguments,
but also on constant values, e.g. how long to wait before switching the
state. The code is easier to understand and to maintain if such values are
stored as named constants (which are also documented). While constants that 
are used by more than one option should better be defined globally, there
are a lot of values that are specific to individual options. Therefore,
CABSL allows to define such constants in the head of an option.

The values of these constants can either be defined in place or they can be
loaded from an external file. In case of the latter, the option would
read a file with the same name as the option itself, but with the extension 
`.cfg` in the current directory. Thus, it would be possible to use
different values for the constants by selecting a different current
directory before starting the behavior. For instance, if an option `go_to`
defines two integer constants `length` and `width` that should be read from
a file, there must exist a file `go_to.cfg` that looks like this:

    length: 78
    width: 21


### State Variables

In general, the current state of a CABSL behavior just consists of the
information which options are currently part of the activation graph and in 
which state they currently are. However, in some situations options need
additional memory. For instance if an option should turn a robot by a
certain angle, it has to remember the robot's orientation when the command
was given to be able to determine when the desired rotation angle has been
reached. Therefore, CABSL allows to define state variables. They keep their 
values from one execution cycle to the next one. However, whenever an
option is not executed in a cycle, they are reset to their initial values
before the option is called again. Thus they always have a defined state.
As these variables are part of the behavior's state, they are also part of
the activation graph.


### Dynamic Option Selection

The command `select_option` allows to execute one option from a list of
options. It is tried to execute each option in the list in the sequence
they are given. If an option determines that it cannot currently be
executed, it stays in its `initial_state`. Otherwise, it is run normally.
`select_option` stops after the first option that was actually executed.
`select_option` returns whether an option was actually executed or whether
all options refused to activate. Note that an option that stays in its
`initial_state` when it was called by `select_option` is considered as not
having been executed at all if it has no action block for that state. If it
has, the block is still executed, but neither the `option_time` nor the
`state_time` are increased. A call to `select_option` could, e.g., look
like this, assuming the three options called behave as it was described
above:

    select_option({
      "play_striker",
      "play_midfielder",
      "play_defender"
    });


### Grammar

    <cabsl>       = { <option> }

    <option>      =  option '(' [ '(' <C-ident> ')' ] <C-ident>
                     [ ',' args '(' <decls> ')' ]
                     [ ',' ( defs | load ) '(' <decls> ')' ]
                     [ ',' vars '(' <decls> ')' ]
                     ')' ( <body> | ';' )

    <body>        = '{'
                    [ <C-statements> ]
                    [ common_transition <transition> ]
                    { <other-state> } initial_state <state> { <other-state> }
                    '}'

    <decls>       = <decl> { ',' <decl> }

    <decl>        = '(' <type> ')' [ '(' <C-expr> ')' ] ' ' <C-ident>

    <other-state> = ( state | target_state | aborted_state ) <state>

    <state>       = '(' C-ident ')'
                    '{'
                    [ transition <transition> ]
                    [ action <action> ]
                    '}'

    <transition>  = '{' <C-ifelse> '}'

    <action>      = '{' <C-statements> '}'
    
`<C-ident>` is a normal C++ identifier. `<C-expr>` is a normal C++
expression that can be used as default value for an argument.
`<C-ifelse>` is a decision tree. It should contain `goto` statements
(names of states are labels).  Conditions can use the pre-defined
symbols `state_time`, `option_time`, `action_done`, and
`action_aborted`. Within a state, the action `<C-statements>` can contain
calls to other (sub)options. `action_done` determines whether the last
sub-option called reached a target state in the previous execution cycle.
`action_aborted` does the same for an aborted state. At the beginning of an
option, it is possible to add arbitrary C++ code, which can contain
definitions that are shared by all states, e.g. lambda functions that
contain calculations used by more than one state. It is not allowed to call
other options outside of `action` blocks.

Options can declare three kinds of additional information:

  - Arguments (`args`) that can be passed to the option.
  - Constant definitions (`defs`, `load`). If `defs` is used, all constants
    need an initialization value (`<C-expr>`). If `load` is used instead,
    they do not need one. Instead, their values are loaded from a
    configuration file.
  - State variables (`vars`) that keep their values between calls.


### Defining Options Inline or Separately

Options are always part of a surrounding behavior class. However, they can
either be defined inline in the body of that class or the implementation
can be kept separately and be compiled to its own object file. For small
behaviors, the first approach is suitable. For instance, the example that
is part of this repository uses this approach (by `#include`ing options
into the body of the main behavior class). For larger behaviors, it is
better to keep options in separate compilation units to avoid re-compiling
the whole behavior after only changing the code of a single option.

If an option is defined inline, its head can contain arguments,
definitions, and variables. For instance, the following code could be
defined inline in the body a of a surrounding class:

    option(Option, args((int) arg1, (bool)(true) arg2),
                   defs((int)(0) def1, (float)(3.14f) def2),
                   vars((int)(1) var1, (char)('x') var2))
    {
      // ...
    }

If an option is split into a declaration and an actual implementation, the
declaration can only contain the arguments (with optional default values).
In that case, only the following code would be part of the body of the
surrounding class:

    option(Option, args((int) arg1, (bool)(true) arg2));

The actual implementation can then by placed in a different file. It must
repeat the arguments, but without any default values, and it can contain
constant definitions and state variables. Assuming the option was declared
in the body of a class `Behavior`, the implementation could look like this:

    #include "Behavior.h"

    option((Behavior) Option,
           args((int) arg1, (bool) arg2),
           defs((int)(0) def1, (float)(3.14f) def2),
           vars((int)(1) var1, (char)('x') var2))
    {
      // ...
    }

If options are defined separately, any declarations in the surrounding
class (other options, symbols) that are accessed by these options cannot be
declared as `private`, i.e. they have to be either `protected` or
`public`.


## Example

A port of the example soccer agent that comes with the XABSL distribution
for ASCII soccer (see the [acknowledgements](Acknowledgements.md))
demonstrates the usage of CABSL. ASCII soccer depends on the *ncurses*
library (CABSL itself does not). If the development version of this library
is installed, the example can be built by simply running *make* in the main
directory of this distribution. On Microsoft Windows, a Unix environment
such as Cygwin is required with the packages *make* and *libncurses-devel*
installed. After a successful build, the example can be executed by
calling *./soccer*. For everything to be visible, make sure that the
terminal window is at least 80 columns wide and is as high as possible. 54
rows would be optimal. The top of the window shows a soccer field, the 
bottom shows up to four activation graphs of the four players running the
CABSL example code.

The example behavior is described in the files in the directory
*example/options*. They are all included into the file *example/options.h*,
which is included into the file *example/behavior.h*. The class `Behavior`
defined in the two files *example/behavior.(h|cpp)* shows how to embed a
CABSL behavior into the code. It is derived from the template class 
`Cabsl`. It has members for all the symbols the behavior can access and it
contains a method that is called once per execution cycle called `execute`.
The method first updates all the members, i.e. the input symbols usable by
the behavior, by calling `updateWorldState`. It then runs the behavior
through these three lines:

    beginFrame(frame_counter++);
    Cabsl<Behavior>::execute("play_soccer");
    endFrame();

The argument of `beginFrame` is an `unsigned int` that functions as time.
Often milliseconds are used here, but in the example, a simple frame
counter is employed instead. It is important that the time progresses
from one execution cycle to the next one, i.e. the number passed to
`beginFrame` must be different from the previous call. The second line
executes the behavior. The root of the behavior is the option
`play_soccer`, which is called here. It will call the other options if
necessary. It is possible to call more than one root option in an
execution cycle, but `execute` can only call options without arguments.
The execution of the behavior ends with a call to `endFrame`. In the
example, the behavior always sets the output symbol `next_action`, which
is then returned by `execute`.

During the execution of the behavior, data about the options and states
is collected in an instance of the class `ActivationGraph`. The address
of this instance was passed to the constructor of `Cabsl`. The method
`showActivationGraph` shows how this data can be visualized, in this
case using commands from the *ncurses* library. In the graphs, the
arguments of the enumeration type `Action` are shown literally instead
of just being displayed as numbers. This is achieved by overloading
the `operator<<` of the *iostream* library for that type.

The file *example/main.cpp* connects the behavior to the interface of
ASCII soccer. It creates four instances of the behavior and lets the
four `player` functions call the `execute` method of their respective
behavior instance.


## Visualization

If the program *dot* by [GraphViz](http://graphviz.org) is installed, the
script *createGraphs* can be executed to create graphs visualizing the
behavior (except for options called through `select_option`). For the
example that comes with this release, such graphs can be created by
executing

    make graphs

or

    bin/createGraphs -p example/options.h

This creates a file *options.pdf* that shows the graph of all options and
an additional pdf-file for each option. The script can be called in two
ways. Either the name of a header file is passed that directly includes all
options of the behavior. Or the switch `-m` is used to specify the name of
the main pdf-file followed by a list of all cpp-files that contain options
instead. The former variant is used if all options are defined inline. The
latter one is used if options are implemented in separate cpp-files. In
addition, the path to *dot* can be specified if it is not in the search
path. By default, the script creates SVG files. This can be changed to PDF
using a switch. There is also an option to manually add data to the graphs,
which is not explained here.

    usage: bin/createGraphs { options } ( <header file> | -m <name> <cpp files> )
      options:
        -a <file> manually add data
        -d <dot>  path to executable 'dot'
        -h        show this help
        -p        output pdf instead of svg


## Technical Details

### Macros

CABSL uses features of C++20, which means that support for this variant
must be enabled when compiling. It also heavily relies on macros which
work with current versions of *cl* (Microsoft's C++ compiler), *g++*, and
*clang++*. For *cl*, the modern preprocessor must be activated, i.e. the
switch `/Zc:preprocessor` must be specified. While the names of all
internal macros start with `_CASBL`, which makes conflicts unlikely, the
front-end macros might cause problems with existing code, because they use
rather normal identifiers:

    aborted_state
    action
    action_aborted
    action_done
    common_transition
    initial_state
    option
    option_time
    select_option
    state
    state_time
    target_state
    transition

If these identifiers are used outside their intended purpose in CABSL,
strange errors might occur, because their appearances are replaced by the
C++ preprocessor by code that does not make sense outside the behavior. To
avoid conflicts with existing header files, always include *Cabsl.h* last.


### Code Generation

Technically, the C++ preprocessor translates each option to a member
function (and some helpers) and a member variable of the surrounding class.
The member variable stores the context of the option, e.g. which is its
current state. The context is passed as a hidden argument to each option
with the help of a temporary wrapper object. Each state is translated to an 
`if`-statement that checks whether the state is the current one and that
contains the transitions and actions. CABSL uses C++ labels as well as line
numbers (`__LINE__`) to identify states. Therefore it is not allowed to
write more than one state per line. Each state defines an unreachable
`goto` statement to the label `initial_state`, which is defined by the
initial state. Thereby, the C++ compiler will ensure that there is exactly
one initial state if the option has at least one state. If the compiler
warning for unused labels is enabled, the compiler will warn if there are
unreachable states. There is also an assertion which checks that never more
than a single transition block is executed per option and execution cycle
at runtime. This should catch some errors, that result from the CABSL
macros not being used as intended.

As all options are declared inside the same class body, where C++ supports
total visibility, every option can call every other option and the
sequence in which the options are defined is not important. Each option
sets a marker in its context whether its current state is a normal,
target, or aborted state. It also preserves that marker of the last
sub-option it called for the next execution cycle so that the symbols
`action_done` and `action_aborted` can use it. The context also stores
when the option was activated (again) and when the current state was
entered to support the symbols `option_time` and `state_time`.

Arguments, constant definitions, and state variables are all passed as
parameters to the member function that contains the code of the option. The
latter two are always passed as references (`const` for constant
definitions). They are stored in memory as structures owned by the option's
context.


### Streaming Values

Arguments and state variables are added to the activation graph. To do
this, they are written to a stream using the operator `<<`. The method
`str()` of that stream is then used to get a string representation of the
value afterwards. By default, CABSL uses the class `std::stringstream`
as the implementation for that stream. Arguments and state variables must
be streamable into a standard *ostream* as text, because they are added to
a data structure that allows visualizing the activation graph. Therefore,
if arguments or state variables are not of primitive data types, the
streaming operator must be overloaded for those types or these values will
not appear in the activation graph:

    std::ostream& operator<<(std::ostream&, const V&)

where `V` is the type of the value.

It is also possible to use a different class for converting values to text.
The class must use the operator `<<` for adding values and return the
resulting string through a member function `str()`. It can then be passed
to CABSL as the third template parameter of the class `cabsl::Cabsl<>`.


### Reading Configuration Files

By default, the class *InFileStream.h* is used to read values from
configuration files. Its constructor simply adds the extension *.cfg* to the
file name passed and opens the file with that name. It implements a method
`read` that reads a single name/value pair per line. For reading the actual
values, the operator `>>` of the class `std::istream` is used. Therefore,
if `load` is used and constant definitions are not of primitive data types,
the streaming operator must be overloaded for those types:

    std::istream& operator>>(std::istream&, V&)

where `V` is the type of the value.

It is also possible to replace the class `InFileStream` by a custom one, as 
long as the interface stays the same. The class used can then be passed
to CABSL as the second template parameter of the class `cabsl::Cabsl<>`.


### Intellisense

If Microsoft Visual Studio is used and inline options are included from
separate files, the following preprocessor code should be added before
including *Cabsl.h*:

    #ifdef __INTELLISENSE__
    #define INTELLISENSE_PREFIX Class::
    #endif

`Class` has to be replaced by the first template parameter of `Cabsl`. The
prefix helps Intellisense to understand that the options are actually part
of the class into the body of which they are included.


### Code Breaking Changes in Version 2

CABSL 2 is not entirely downward compatible with CABSL 1. The following
details must be changed when upgrading:

  - CABSL 2 requires C++20, because it uses designated initializers.
  - All CABSL definitions are now part of the namespace `cabsl`. Therefore,
    the classes `Cabsl` and `ActivationGraph` must be prefixed by
    `cabsl::`.
  - The list of formal option arguments must be surrounded by `args(...)`.
    For instance, `option(Option, (int) arg1, (float)(1.f) arg2)` must be
    replaced by `option(Option, args((int) arg1, (float)(1.f) arg2))`.
  - When calling an option with arguments, the arguments must now be named 
    explicitly: Instead of, e.g., `Option(1, 2.f)` now
    `Option({.arg1 = 1, .arg2 = 2.f})` must be written (or
    `Option({.arg1 = 1})` to use the default value for `arg2`).
  - When calling `execute` or `select_option`, options are now referred to
    by their name, i.e. as a string, instead of an abstract value.


## References

 1. Röfer, T. (2018). CABSL – C-Based Agent Behavior Specification
    Language. In: Akiyama, H., Obst, O., Sammut, C., Tonidandel, F. (eds).
    RoboCup 2017: Robot World Cup XXI. RoboCup 2017. Lecture Notes in
    Computer Science, vol 11175.
    [Springer](https://link.springer.com/chapter/10.1007/978-3-030-00308-1_11),
    Cham.