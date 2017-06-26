# C-based Agent Behavior Specification Language (CABSL)

The *C-based Agent Behavior Specification Language* (CABSL) allows
specifying the behavior of a robot or a software agent in C++11.
Semantically, it follows the ideas of the *Extensible Agent Behavior
Specification Language* [XABSL](http://www.xabsl.de) developed by Martin
Lötzsch, Max Risler, and Matthias Jüngel, but its integration into a
C++ program requires significantly less programming overhead. The actual
implementation of CABSL is only contained in two files, which can
both be found in the directory *include* of this distribution. In addition,
there is a script file in the directory *bin* that can be used to visualize
the behavior. All other source files are just part of an example on how to
use CABSL.

CABSL is also part of all B-Human Code Releases since 2013. The difference
between the version that comes with the B-Human software and the version
in this distribution is that the former is tightly integrated with
B-Human's streaming architecture, while the version in this release uses
the standard `iostream` library instead. Otherwise, they are identical.


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

* The `option_time` is the time the option was continuously active, i.e.
  the time since the first execution cycle it became active without being
  inactive afterwards. The unit of the time depends on the time values
  passed to the behavior engine (see section [Example](#example)).

* The `state_time` is the time the option was continuously in the same
  state, i.e. the time since the first execution cycle it switched to that
  state without switching to another one afterwards. The `state_time` is
  also reset when the option becomes inactive, i.e. the `state_time` can
  never be bigger than the `option_time`.

* `action_done` is true if the last sub-option executed in the previous
  cycle was in a *target state*.

* `action_aborted` is true if the last sub-option executed in the previous
  cycle was in an *aborted state*.


### Parameters

Options can have *parameters*. From the perspective of the option, they
are not different from input symbols. As in C++, parameters hide member
variables of the surrounding class with the same name. When calling an
option, its actual parameters are passed to it as they would be in C++.

However, parameters must be streamable into a standard *iostream* as
text, because they are added to a data structure that allows visualizing
the activation graph. Therefore, any parameter that is not of a primitive
data type must overload

    std::ostream& operator<<(std::ostream&, P)

where `P` is the type of the parameter.

As can be seen in the following section, the syntax of parameter
definitions is different from the syntax C++ normally uses.


### Grammar

    <cabsl>       = { <option> }
    
    <option>      = option '(' <C-ident> { ',' <param-decl> } { ',' <param-dflt> } ')'
                    '{'
                    [ common_transition <transition> ]
                    { <other-state> } initial_state <state> { <other-state> }
                    '}'
    
    <param-decl>  = '(' <type> ')' ' ' <C-ident>
    
    <param-dflt>  = '(' <type> ')' '(' <C-expr> ')' ' ' <C-ident>
    
    <other-state> = ( state | target_state | aborted_state ) <state>
    
    <state>       = '(' C-ident ')'
                    '{'
                    [ transition <transition> ]
                    [ action <action> ]
                    '}'
    
    <transition>  = '{' <C-ifelse> '}'
    
    <action>      = '{' <C-statements> '}'

`<C-ident>` is a normal C++ identifier. `<C-expr>` is a normal C++ expression
that can be used as default value for a parameter. `<C-ifelse>` is a decision
tree. It should contain `goto` statements (names of states are labels).
`<C-statements>` is a sequence of arbitrary C++ statements.


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
    Cabsl<Behavior>::execute(OptionInfos::getOption("play_soccer"));
    endFrame();

The parameter of `beginFrame` is an `unsigned int` that functions as time.
Often milliseconds are used here, but in the example, a simple frame
counter is employed instead. It is important that the time progresses
from one execution cycle to the next one, i.e. the number passed to
`beginFrame` must be different from the previous call. The second line
executes the behavior. The root of the behavior is the option
`play_soccer`, which is called here. It will call the other options if
necessary. It is possible to call more than one root option in an
execution cycle, but `execute` can only call options without parameters.
`OptionInfos::getOption` will not even find options with parameters. The 
execution of the behavior ends with a call to `endFrame`. In the example,
the behavior always sets the output symbol `next_action`, which is then
returned by `execute`.

During the execution of the behavior, data about the options and states
is collected in an instance of the class `ActivationGraph`. The address
of this instance was passed to the constructor of `Cabsl`. The method
`showActivationGraph` shows how this data can be visualized, in this
case using commands from the *ncurses* library. In the graphs, the
parameters of the enumeration type `Action` are shown literally instead
of just being displayed as numbers. This is achieved by overloading
the `operator<<` of the *iostream* library for that type.

The file *example/main.cpp* connects the behavior to the interface of
ASCII soccer. It creates four instances of the behavior and lets the
four `player` functions call the `execute` method of their respective
behavior instance.


## Visualization

If there is a file that includes all the options directly and the program
*dot* by [GraphViz](http://graphviz.org) is installed, the script
*createGraphs* can be executed to create graphs visualizing the behavior.
For the example that comes with this release, such graphs can be created
by executing

    make graphs

or

    bin/createGraphs example/options.h

This creates a file *options.pdf* that shows the graph of all options and
an additional pdf-file for each option. The script itself requires a 
header file that directly includes all options as its parameter. In 
addition the path to *dot* can be specified if it is not in the search 
path.

    usage: bin/createGraphs { options } <header file>
      options:
        -d <dot>  path to executable 'dot'
        -h        show this help


## Technical Details

### Macros

CABSL uses features of C++11, which means that support for this variant
must be enabled when compiling. It also heavily relies on macros which
work with current versions of *cl* (Microsoft's C++ compiler), *g++*, and
*clang++*. While the names of all internal macros start with `_CASBL`,
which makes conflicts unlikely, the front-end macros might cause problems
with existing code, because they use rather normal identifiers:

    aborted_state
    action
    action_aborted
    action_done
    common_transition
    initial_state
    option
    option_time
    state
    state_time
    target_state
    transition

If these identifiers are used outside their intended purpose in CABSL,
strange errors might occur, because their appearances are replaced by the
C++ preprocessor by code that does not make sense outside the behavior. To
avoid conflicts with existing header files, always include *Cabsl.h* last.


### Maximum Number of Options

When the program is started, CABSL collects information about the options
that are available. For this process, a maximum number of space is
reserved that can be increased if necessary. To increase the number,
change the following line in *Cabsl.h*:

    static const size_t maxNumOfOptions = 500;


### Code Generation

Technically, the C++ preprocessor translates each option to a member
function (two for options with parameters) and a member variable of the
surrounding class. The member variable stores the context of the option,
e.g. which is its current state. The context is passed as a hidden
parameter to each option with the help of a temporary wrapper object.
Each state is translated to an `if`-statement that checks whether the state
is the current one and that contains the transitions and actions. CABSL
uses C++ labels as well as line numbers (`__LINE__`) to identify states.
Therefore it is not allowed to write more than one state per line. Each
state defines an unreachable `goto` statement to the label
`initial_state`, which is defined by the initial state. Thereby, the C++
compiler will ensure that there is exactly one initial state if the option
has at least one state. If the compiler warning for unused labels is
enabled, the compiler will warn if there are unreachable states. There is
also an assertion which checks that never more then a single transition 
block is executed per option and execution cycle at runtime. This should
catch some errors, that result from the CABSL macros not being used as
intended.

As all options are defined inside the same class body, where C++ supports
total visibility, every option can call every other option and the
sequence in which the options are defined is not important. Each option
sets a marker in its context whether its current state is a normal,
target, or aborted state. It also preserves that marker of the last
sub-option it called for the next execution cycle so that the symbols
`action_done` and `action_aborted` can use it. The context also stores
when the option was activated (again) and when the current state was
entered to support the symbols `option_time` and `state_time`.


### Intellisense

If Microsoft Visual Studio is used and options are included from separate
files, the following preprocessor code should be added before including
*Cabsl.h*:

    #ifdef __INTELLISENSE__
    #define INTELLISENSE_PREFIX Class::
    #endif

`Class` has to be replaced by the template parameter of `Cabsl`. The
prefix helps Intellisense to understand that the options are actually part
of the class in the body of which they are included.
