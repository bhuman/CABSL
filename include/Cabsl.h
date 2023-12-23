/**
 * @file Cabsl.h
 *
 * The file defines a set of macros that define the C-based agent
 * behavior specification language (CABSL). Semantically, it follows
 * the ideas of XABSL.
 *
 * Grammar:
 *
 * The file defines a set of macros that define the C-based agent
 * behavior specification language (CABSL). Semantically, it follows
 * the ideas of XABSL.
 *
 * Grammar:
 *
 *     <cabsl>       = { <option> }
 *
 *     <option>      =  option '(' [ '(' <C-ident> ')' ] <C-ident>
 *                      [ ',' args '(' <decls> ')' ]
 *                      [ ',' ( defs | load ) '(' <decls> ')' ]
 *                      [ ',' vars '(' <decls> ')' ]
 *                      ')' ( <body> | ';' )
 *
 *     <body>        = '{'
 *                     [ <C-statements> ]
 *                     [ common_transition <transition> ]
 *                     { <other-state> } initial_state <state> { <other-state> }
 *                     '}'
 *
 *     <decls>       = <decl> { ',' <decl> }
 *
 *     <decl>        = '(' <type> ')' [ '(' <C-expr> ')' ] ' ' <C-ident>
 *
 *     <other-state> = ( state | target_state | aborted_state ) <state>
 *
 *     <state>       = '(' C-ident ')'
 *                     '{'
 *                     [ transition <transition> ]
 *                     [ action <action> ]
 *                     '}'
 *
 *     <transition>  = '{' <C-ifelse> '}'
 *
 *     <action>      = '{' <C-statements> '}'
 *
 * `<C-ident>` is a normal C++ identifier. `<C-expr>` is a normal C++
 * expression that can be used as default value for an argument.
 * `<C-ifelse>` is a decision tree. It should contain `goto` statements
 * (names of states are labels).  Conditions can use the pre-defined
 * symbols `state_time`, `option_time`, `action_done`, and
 * `action_aborted`. Within a state, the action `<C-statements>` can contain
 * calls to other (sub)options. `action_done` determines whether the last
 * sub-option called reached a target state in the previous execution cycle.
 * `action_aborted` does the same for an aborted state. At the beginning of an
 * option, it is possible to add arbitrary C++ code, which can contain
 * definitions that are shared by all states, e.g. lambda functions that
 * contain calculations used by more than one state. It is not allowed to call
 * other options outside of `action` blocks.
 *
 * Options can declare three kinds of additional parameters:
 *
 * - Arguments (args) that can be passed to the option.
 * - Definitions (defs, load) define constant parameters of the option, i.e.
 *   usually values the implementation depends upon. If "load" is used
 *   instead of "defs", their values are loaded from a configuration file with
 *   the name "option<option name>.cfg".
 * - Variables (vars) define option-local variables. They keep their
 *   values from each call of the option to the next call. However, they
 *   are (re-)initialized with their default values if the option was not
 *   executed in the previous cycle.
 *
 * Options can either be defined inline or just be declared. In case of the
 * latter, there also must be an implementation in a separate file that
 * includes the file containing the declaration. In that file, the name of
 * the option must be preceded by the name of its class in parentheses.
 *
 * If an option is defined inline, its head can contain arguments,
 * definitions, and variables. If it is split into a declaration and an
 * actual implementation, the declaration can only contain the arguments
 * (with optional default values) while the implementation must repeat the
 * arguments (without default values) and it can contain definitions and
 * variables.
 *
 * If options with arguments are called, a single struct is passed
 * containing all arguments. This means that the list of actual arguments
 * has to be enclosed in braces. This allows to specify their names in the
 * call, e.g. 'go_to_point({.x = 10, .y = 20})'. Arguments not specified
 * are set to their default values, either the ones that were given in the
 * argument list of the option or the default value for the datatype.
 *
 * The command 'select_option' allows to execute one option from a list of
 * options. It is tried to execute each option in the list in the sequence they
 * are given. If an option determines that it cannot currently be executed,
 * it stays in its 'initial_state'. Otherwise, it is run normally. 'select_option'
 * stops after the first option that was actually executed. Note that an
 * option that stays in its 'initial_state' when it was called by 'select_option'
 * is considered as not having been executed at all if it has no action block
 * for that state. If it has, the block is still executed, but neither the
 * 'option_time' nor the 'state_time' are increased.
 *
 * If Microsoft Visual Studio is used and options are included from separate
 * files, the following preprocessor code might be added before including
 * this file. "Class" has to be replaced by the template parameter of Cabsl:
 *
 * #ifdef __INTELLISENSE__
 * #define INTELLISENSE_PREFIX Class::
 * #endif
 *
 * @author Thomas Röfer
 */

#pragma once

#include <cassert>
#include <sstream>
#include <type_traits>
#include <unordered_map>
#include "ActivationGraph.h"

/**
 * Base class for helper structures that makes sure that the
 * destructor is virtual.
 */
struct CabslStructBase
{
  virtual ~CabslStructBase() = default;
};

/**
 * The base class for CABSL behaviors.
 * Note: Private variables that cannot be declared as private start with an
 * underscore.
 * @tparam T The class that implements the behavior. It must also be derived from
 * this class.
 */
template<typename T> class Cabsl
{
protected:
  using CabslBehavior = T; /**< This type allows to access the derived class by name. */

  /**
   * The context stores the current state of an option.
   */
  class OptionContext
  {
  public:
    /** The different types of states (for implementing initial_state, target_state, and aborted_state). */
    enum StateType
    {
      normalState,
      initialState,
      targetState,
      abortedState
    };

    int state; /**< The state currently selected. This is actually the line number in which the state was declared. */
    const char* stateName; /**< The name of the state (for activation graph). */
    unsigned lastFrame = static_cast<unsigned>(-1); /**< The timestamp of the last frame in which this option was executed (except for the initial state when called from \c select_option). */
    unsigned lastSelectFrame = static_cast<unsigned>(-1); /**< The timestamp of the last frame in which this option was executed (in any case). */
    unsigned optionStart; /**< The time when the option started to run (for option_time). */
    unsigned stateStart; /**< The time when the current state started to run (for state_time). */
    StateType stateType; /**< The type of the current state in this option. */
    StateType subOptionStateType; /**< The type of the state of the last suboption executed (for action_done and action_aborted). */
    bool addedToGraph; /**< Was this option already added to the activation graph in this frame? */
    bool transitionExecuted; /**< Has a transition already been executed? True after a state change. */
    bool hasCommonTransition; /**< Does this option have a common transition? Is reset when entering the first state. */
    CabslStructBase* defs = nullptr; /**< Option configuration definitions. */
    CabslStructBase* vars = nullptr; /**< Option variables. */

    /** Destructor. */
    ~OptionContext()
    {
      delete defs;
      delete vars;
    }
  };

  /**
   * Instances of this class are passed as a default argument to each option.
   * They maintain the current state of the option.
   */
  class OptionExecution
  {
  private:
    /** Helper to determine, whether A is streamable. */
    template<typename U> struct isStreamableBase
    {
      template<typename V> static auto test(V*) -> decltype(std::declval<std::ostream&>() << std::declval<V>());
      template<typename> static auto test(...) -> std::false_type;
      using type = typename std::is_same<std::ostream&, decltype(test<U>(nullptr))>::type;
    };
    template<typename U> struct isStreamable : isStreamableBase<U>::type {};

    const char* optionName; /**< The name of the option (for activation graph). */
    Cabsl* instance; /**< The object that encapsulates the behavior. */
    bool fromSelect; /**< Option is called from 'select_option'. */
    mutable std::vector<std::string> arguments; /**< Argument names and their values. */

  public:
    OptionContext& context; /**< The context of the state. */

    /**
     * The constructor checks, whether the option was active in the previous frame and
     * if not, it switches back to the initial state.
     * @param optionName The name of the option (for activation graph).
     * @param context The context of the state.
     * @param instance The object that encapsulates the behavior.
     */
    OptionExecution(const char* optionName, OptionContext& context, Cabsl* instance, bool fromSelect = false) :
      optionName(optionName), instance(instance), fromSelect(fromSelect), context(context)
    {
      if(context.lastFrame != instance->lastFrameTime && context.lastFrame != instance->_currentFrameTime)
      {
        context.optionStart = instance->_currentFrameTime; // option started now
        context.stateStart = instance->_currentFrameTime; // initial state started now
        context.state = 0; // initial state is always marked with a 0
        context.stateType = OptionContext::initialState;
      }
      if(context.lastSelectFrame != instance->lastFrameTime && context.lastSelectFrame != instance->_currentFrameTime)
        context.subOptionStateType = OptionContext::normalState; // reset action_done and action_aborted
      context.addedToGraph = false; // not added to graph yet
      context.transitionExecuted = false; // no transition executed yet
      context.hasCommonTransition = false; // until one is found, it is assumed that there is no common transition
      ++instance->depth; // increase depth counter for activation graph
    }

    /**
     * The destructor cleans up the option context.
     */
    ~OptionExecution()
    {
      if(!fromSelect || context.stateType != OptionContext::initialState)
      {
        addToActivationGraph(); // add to activation graph if it has not been already
        context.lastFrame = instance->_currentFrameTime; // Remember that this option was called in this frame
      }
      context.lastSelectFrame = instance->_currentFrameTime; // Remember that this option was called in this frame (even in select_option/initial)
      --instance->depth; // decrease depth counter for activation graph
      context.subOptionStateType = instance->stateType; // remember the state type of the last sub option called
      instance->stateType = context.stateType; // publish the state type of this option, so the caller can grab it
    }

    /**
     * The method is executed whenever the state is changed.
     * @param newState The new state to which it was changed.
     * @param stateType The type of the new state.
     */
    void updateState(int newState, typename OptionContext::StateType stateType) const
    {
      assert(context.hasCommonTransition != context.transitionExecuted); // [common_]transition is missing
      context.transitionExecuted = true; // a transition was executed, do not execute another one
      if(context.state != newState) // ignore transitions that stay in the same state
      {
        context.state = newState;
        context.stateStart = instance->_currentFrameTime; // state started now
        context.stateType = stateType; // remember type of this state
      }
    }

    /**
     * Adds a string description containing the current value of an argument to the list of arguments.
     * The description is only added if the argument is streamable.
     * @tparam U The type of the argument.
     * @param value The current value of the argument.
     */
    template<typename U> std::enable_if<isStreamable<U>::value>::type addArgument(const char* name, const U& value) const
    {
      const char* p = name + std::strlen(name) - 1;
      while(p >= name && *p != ')' && *p != ' ')
        --p;
      name = p + 1;

      std::stringstream stream;
      stream << value;
      const std::string text = stream.str();
      if(!text.empty())
        arguments.emplace_back(name + (" = " + text));
    }

    /** Does not write the argument to a stream, because it is not streamable. */
    template<typename U> typename std::enable_if<!isStreamable<U>::value>::type addArgument(const char*, const U&) const {}

    /**
     * The method adds information about the current option and state to the activation graph.
     * It suppresses adding it twice in the same frame.
     */
    void addToActivationGraph() const
    {
      if(!context.addedToGraph && instance->activationGraph)
      {
        instance->activationGraph->graph.emplace_back(optionName, instance->depth,
                                                      context.stateName,
                                                      instance->_currentFrameTime - context.optionStart,
                                                      instance->_currentFrameTime - context.stateStart,
                                                      arguments);
        context.addedToGraph = true;
      }
    }
  };

  /** A class to store information about an option. */
  struct OptionDescriptor
  {
    const char* name; /**< The name of the option. */
    void (CabslBehavior::*option)(const OptionExecution&); /**< The option method. */
    size_t offsetOfContext; /**< The memory offset of the context within the behavior class. */
    int index; /**< The index of the option (for the enum of all options). */

    /** Default constructor, because STL types need one. */
    OptionDescriptor() = default;

    /**
     * Constructor.
     * @param name The name of the option.
     * @param option The option method.
     * @param offsetOfContext The memory offset of the context within the behavior class.
     */
    OptionDescriptor(const char* name, void (CabslBehavior::*option)(const OptionExecution&), size_t offsetOfContext) :
      name(name), option(option), offsetOfContext(offsetOfContext), index(0)
    {}
  };

public:
  /** A class that collects information about all options in the behavior. */
  class OptionInfos
  {
  private:
    static std::unordered_map<std::string, const OptionDescriptor*>* optionsByName; /**< All argumentless options, indexed by their names. */
    static std::vector<void (*)()>* modifyHandlers; /**< All modify handlers for options with definitions. */

  public:
    /** The constructor prepares the collection of information if this has not been done yet. */
    OptionInfos()
    {
      if(!optionsByName)
        init();
    }

    /** The destructor frees the global object. */
    ~OptionInfos()
    {
      delete optionsByName;
      delete modifyHandlers;
      optionsByName = nullptr;
      modifyHandlers = nullptr;
    }

    /**
     * The method prepares the collection of information about all options in optionByIndex
     * and optionsByName. It also adds a dummy option descriptor at index 0 with the name "none".
     */
    static void init()
    {
      assert(!optionsByName);
      optionsByName = new std::unordered_map<std::string, const OptionDescriptor*>;
      static OptionDescriptor descriptor("none", 0, 0);
      (*optionsByName)[descriptor.name] = &descriptor;
    }

    /**
     * The method adds information about an option to the collections.
     * It will be called from the constructors of static objects created for each
     * option.
     * This method is only called for options without arguments, because only they
     * can be called externally.
     * @param fnDescriptor A function that can return the description of an option.
     */
    static void add(OptionDescriptor*(*fnDescriptor)())
    {
      if(!optionsByName)
        init();

      assert(optionsByName);
      OptionDescriptor& descriptor = *fnDescriptor();
      if(optionsByName->find(descriptor.name) == optionsByName->end()) // only register once
        (*optionsByName)[descriptor.name] = &descriptor;
    }

    /**
     * The method registers a handler to modify definitions.
     * @param modifyHandler The address of the handler.
     */
    static void add(void (*modifyHandler)())
    {
      if(!modifyHandlers)
        modifyHandlers = new std::vector<void (*)()>;
      modifyHandlers->push_back(modifyHandler);
    }

    /**
     * The method executes a certain option. Note that only argumentless options can be
     * executed.
     * @param behavior The behavior instance.
     * @param option The name of the option.
     * @param fromSelect Was this method called fron "select_option"?
     * @return Was the option actually executed?
     */
    static bool execute(CabslBehavior* behavior, const std::string& option, bool fromSelect = false)
    {
      auto pair = optionsByName->find(option);
      if(pair != optionsByName->end())
      {
        const OptionDescriptor& descriptor = *pair->second;
        OptionContext& context = *reinterpret_cast<OptionContext*>(reinterpret_cast<char*>(behavior) + descriptor.offsetOfContext);
        (behavior->*(descriptor.option))(OptionExecution(descriptor.name, context, behavior, fromSelect));
        return context.stateType != OptionContext::initialState;
      }
      else
        return false;
    }

    /**
     * The method executes a list of options. It stops after the first option that reports that
     * it was actually executed.
     * @param behavior The behavior instance.
     * @param options The list of option names. The options are executed in that order.
     * @return Was an option actually executed?
     */
    static bool execute(CabslBehavior* behavior, const std::vector<std::string>& options)
    {
      for(const std::string& option : options)
        if(execute(behavior, option, true))
          return true;
      return false;
    }

    /** Executes all handlers that allow to modify the definitions. */
    static void executeModifyHandlers()
    {
      if(modifyHandlers)
        for(void (*modifyHandler)() : *modifyHandlers)
          modifyHandler();
    }
  };

protected:
  /**
   * A template class for collecting information about an option.
   * @tparam descriptor A function that can return the description of the option.
   */
  template<const void*(*descriptor)()> class OptionInfo : public OptionContext
  {
  private:
    /** A helper structure to register information about the option. */
    struct Registrar
    {
      Registrar() {OptionInfos::add(reinterpret_cast<OptionDescriptor*(*)()>(descriptor));}
    };
    static Registrar registrar; /**< The instance that registers the information about the option through construction. */

  public:
    /** A dummy constructor that enforces linkage of the static member. */
    OptionInfo() {static_cast<void>(&registrar);}
  };

private:
  static OptionInfos collectOptions; /**< This global instantiation collects data about all options. */
  typename OptionContext::StateType stateType; /**< The state type of the last option called. */
  unsigned lastFrameTime; /**< The timestamp of the last time the behavior was executed. */
  unsigned char depth; /**< The depth level of the current option. Used for activation graph. */
  ActivationGraph* activationGraph; /**< The activation graph for debug output. Can be zero if not set. */

protected:
  static thread_local Cabsl* _theInstance; /**< The instance of this behavior used. */
  unsigned _currentFrameTime; /**< The timestamp of the last time the behavior was executed. */

  /**
   * Constructor.
   * @param activationGraph When set, the activation graph will be filled with the
   *                        options and states executed in each frame.
   */
  Cabsl(ActivationGraph* activationGraph = nullptr) :
    stateType(OptionContext::normalState),
    lastFrameTime(0),
    depth(0),
    activationGraph(activationGraph),
    _currentFrameTime(0)
  {
    static_cast<void>(&collectOptions); // Enforce linking of this global object
  }

public:
  /**
   * Must be call at the beginning of each behavior execution cycle even if no option is called.
   * @param frameTime The current time in ms.
   */
  void beginFrame(unsigned frameTime)
  {
    _currentFrameTime = frameTime;
    if(activationGraph)
      activationGraph->graph.clear();
    _theInstance = this;
    OptionInfos::executeModifyHandlers();
  }

  /**
   * Execute an option as a root.
   * Several root options can be executed in a single behavior execution cycle.
   * @param root The root option that is executed.
   */
  void execute(const std::string& root)
  {
    OptionInfos::execute(static_cast<CabslBehavior*>(this), root);
  }

  /** Must be called at the end of each behavior execution cycle even if no option is called. */
  void endFrame()
  {
    _theInstance = nullptr;
    lastFrameTime = _currentFrameTime;
  }
};

template<typename CabslBehavior> thread_local Cabsl<CabslBehavior>* Cabsl<CabslBehavior>::_theInstance;
template<typename CabslBehavior> std::unordered_map<std::string, const typename Cabsl<CabslBehavior>::OptionDescriptor*>* Cabsl<CabslBehavior>::OptionInfos::optionsByName;
template<typename CabslBehavior> std::vector<void (*)()>* Cabsl<CabslBehavior>::OptionInfos::modifyHandlers;
template<typename CabslBehavior> typename Cabsl<CabslBehavior>::OptionInfos Cabsl<CabslBehavior>::collectOptions;
template<typename CabslBehavior> template<const void*(descriptor)()> typename Cabsl<CabslBehavior>::template OptionInfo<descriptor>::Registrar Cabsl<CabslBehavior>::OptionInfo<descriptor>::registrar;

/**
 * The macro defines a state. It must be followed by a block of code that defines the state's body.
 * @param name The name of the state.
 */
#define state(name) _state(name, __LINE__, OptionContext::normalState)

/**
 * The macro defines a target state. It must be followed by a block of code that defines the state's body.
 * A parent option can check whether a target state has been reached through action_done.
 * @param name The name of the target state.
 */
#define target_state(name) _state(name, __LINE__, OptionContext::targetState)

/**
 * The macro defines an aborted state. It must be followed by a block of code that defines the state's body.
 * A parent option can check whether an aborted state has been reached through action_aborted.
 * @param name The name of the aborted state.
 */
#define aborted_state(name) _state(name, __LINE__, OptionContext::abortedState)

/**
 * The macro defines an option. It must be followed by a block of code that defines the option's body
 * The option gets an additional parameter that manages its context. If the option has parameters,
 * two methods are generated. The first one adds the parameters to the execution environment and calls
 * the second one.
 * @param ... The name of the option and an arbitrary number of parameters. They can include default
 *            parameters at the end. Their syntax is described at the beginning of this file.
 */
#define option(...) \
  _CABSL_OPTION_I(_CABSL_HAS_PARAMS(__VA_ARGS__), __VA_ARGS__)
#define _CABSL_OPTION_I(hasParams, ...) _CABSL_OPTION_II(hasParams, (__VA_ARGS__))
#define _CABSL_OPTION_II(hasParams, params) _CABSL_OPTION_##hasParams params

/** Macro specialization for option with parameters. */
#define _CABSL_OPTION_1(name, ...) _CABSL_OPTION_III(_CABSL_TUPLE_SIZE(__VA_ARGS__, ignore), name, __VA_ARGS__, ignore)

/** Generate an attribute declaration without an initialization. */
#define _CABSL_DECL_WITHOUT_INIT(seq) _CABSL_DECL_IV seq)) _CABSL_VAR(seq);

/** Generate a parameter declaration with an initialization if avaliable. */
#define _CABSL_PARAM_WITH_INIT(seq) _CABSL_DECL_IV seq)) _CABSL_VAR(seq) _CABSL_INIT(seq),

/** Generate a parameter declaration without an initialization. */
#define _CABSL_PARAM_WITHOUT_INIT(seq) _CABSL_DECL_IV seq)) _CABSL_VAR(seq),

/** Generate a parameter declaration without an initialization (without comma). */
#define _CABSL_PARAM_WITHOUT_INIT2(seq) _CABSL_DECL_IV seq)) _CABSL_VAR(seq)

/** Generate a variable name for the list of actual parameters of a method call. */
#define _CABSL_VAR_COMMA(seq) _CABSL_VAR(seq),

/** Generate code for streaming a variable and adding it to the parameters stored in the execution environment. */
#define _CABSL_STREAM(seq) \
  _CABSL_JOIN(_CABSL_STREAM_, _CABSL_SEQ_SIZE(seq))(seq) \
    _o.addArgument(#seq, _CABSL_VAR(seq));

#ifndef __INTELLISENSE__

/** Macro specialization for parameterless option. */
#define _CABSL_OPTION_0(name) \
  static const void* _get##name##Descriptor() \
  { \
    static OptionDescriptor descriptor(#name, reinterpret_cast<void (CabslBehavior::*)(const OptionExecution&)>(&CabslBehavior::name), \
                              reinterpret_cast<size_t>(&reinterpret_cast<CabslBehavior*>(16)->_##name##Context) - 16); \
    return &descriptor; \
  } \
  OptionInfo<&CabslBehavior::_get##name##Descriptor> _##name##Context; \
  void name(const OptionExecution& _o = OptionExecution(#name, static_cast<CabslBehavior*>(_theInstance)->_##name##Context, _theInstance))
#define _CABSL_OPTION_III(n, name, ...) _CABSL_OPTION_IV(n, name, (_CABSL_PARAM_WITH_INIT, __VA_ARGS__), (_CABSL_VAR_COMMA, __VA_ARGS__), (_CABSL_STREAM, __VA_ARGS__), (_CABSL_PARAM_WITHOUT_INIT, __VA_ARGS__))
#define _CABSL_OPTION_IV(n, name, params1, params2, params3, params4) \
  OptionContext _##name##Context; \
  void name(_CABSL_ATTR_##n params1 const OptionExecution& _o = OptionExecution(#name, static_cast<CabslBehavior*>(_theInstance)->_##name##Context, _theInstance)) \
  { \
    _CABSL_ATTR_##n params3 \
    _##name(_CABSL_ATTR_##n params2 _o); \
  } \
  void _##name(_CABSL_ATTR_##n params4 const OptionExecution& _o)

/** If a default value exists, only stream parameters that are different from it. */
#define _CABSL_STREAM_1(seq)
#define _CABSL_STREAM_2(seq) if(!(_CABSL_VAR(seq) == _CABSL_INIT_II seq) _CABSL_INIT_I_2_I(seq)))

/**
 * The macro defines an initial state. It must be followed by a block of code that defines the state's body.
 * Since there does not need to be a transition to an initial state, an unreachable goto statement is defined
 * to avoid warnings about unused labels. The initial state also has an unused extra label that simply ensures
 * that whenever there are states, there must be exactly one initial state.
 * @param name The name of the initial state.
 */
#define initial_state(name) \
  initial_state: \
  if(_o.context.state == -1) \
    goto name; \
  _state(name, 0, OptionContext::initialState)

/**
 * The actual code generated for the state macros above.
 * An unreachable goto statement ensures that there is an initial state.
 * @param name The name of the state.
 * @param line The line number in which the state is defined of 0 for the initial state.
 * @param stateType The type of the state.
 */
#define _state(name, line, stateType) \
  if(false) \
  { \
    goto initial_state; \
  name: _o.updateState(line, stateType); \
  } \
  _o.context.hasCommonTransition = false; \
  if(_o.context.state == line && (_o.context.stateName = #name))

/**
 * The macro marks a common_transition. It sets a flag so that a transition is accepted,
 * even if not executed through the keyword "transition".
 */
#define common_transition \
  _o.context.hasCommonTransition = true;

/**
 * The macro marks a transition. It should be followed by a block of code that contains the decision
 * tree that implements the transition.
 * The macro prevents the option from executing more than one transition per frame.
 * Setting the flag here also allows the transition target to check whether actually
 * a transition block was used to jump to the new state.
 */
#define transition \
  if((_o.context.transitionExecuted ^= true))

/**
 * The macro marks an action. It should be followed by a block of code that contains the
 * implementation of the action.
 * The macro adds information about the current option and state to the activation graph,
 * so that it is added before any suboptions called in the action block can add theirs.
 */
#define action _o.addToActivationGraph();

/** The time since the execution of this option started. */
#define option_time int(_currentFrameTime - _o.context.optionStart)

/** The time since the execution of the current state started. */
#define state_time int(_currentFrameTime - _o.context.stateStart)

/** Did a suboption called reached a target state? */
#define action_done (_o.context.subOptionStateType == OptionContext::targetState)

/** Did a suboption called reached an aborted state? */
#define action_aborted (_o.context.subOptionStateType == OptionContext::abortedState)

/**
 * Executes the first applicable option from a list.
 * @param ... The list of options as a std::vector<std::string>.
 * @return Was an option executed?
 */
#define select_option(...) OptionInfos::execute(this, __VA_ARGS__)

#else // __INTELLISENSE__

#ifndef INTELLISENSE_PREFIX
#define INTELLISENSE_PREFIX
#endif
#define _CABSL_OPTION_0(name) void INTELLISENSE_PREFIX name()
#define _CABSL_OPTION_III(n, name, ...) _CABSL_OPTION_IV(n, name, (_CABSL_PARAM_WITH_INIT, __VA_ARGS__))
#define _CABSL_OPTION_IV(n, name, params) \
  void INTELLISENSE_PREFIX name(_CABSL_ATTR_##n params bool _ignore = false)

#define initial_state(name) \
  initial_state: \
  if(false) \
    goto name; \
  _state(name, 0, )

#define _state(name, line, stateType) \
  if(false) \
  { \
    goto initial_state; \
  name:; \
  } \
  if(true)

#define common_transition ;
#define transition if(true)
#define action ;
#define option_time 0
#define state_time 0
#define action_done false
#define action_aborted false
#define select_option(...) false

#endif

/** Define symbol if Microsoft's traditional preprocessor is used. */
#if defined _MSC_VER && (!defined _MSVC_TRADITIONAL || _MSVC_TRADITIONAL)
#define _CABSL_MSC
#endif

/** Check whether an option has parameters. */
#ifdef _CABSL_MSC
#define _CABSL_HAS_PARAMS(...) _CABSL_JOIN(_CABSL_TUPLE_SIZE_II, (__VA_ARGS__, _CABSL_HAS_PARAMS_II))
#else
#define _CABSL_HAS_PARAMS(...) _CABSL_HAS_PARAMS_I((__VA_ARGS__, _CABSL_HAS_PARAMS_II))
#define _CABSL_HAS_PARAMS_I(params) _CABSL_TUPLE_SIZE_II params
#endif
#define _CABSL_HAS_PARAMS_II \
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0

/**
 * Determine the number of entries in a tuple.
 */
#if defined _CABSL_MSC && !defined Q_MOC_RUN
#define _CABSL_TUPLE_SIZE(...) _CABSL_JOIN(_CABSL_TUPLE_SIZE_II, (__VA_ARGS__, _CABSL_TUPLE_SIZE_III))
#else
#define _CABSL_TUPLE_SIZE(...) _CABSL_TUPLE_SIZE_I((__VA_ARGS__, _CABSL_TUPLE_SIZE_III))
#define _CABSL_TUPLE_SIZE_I(params) _CABSL_TUPLE_SIZE_II params
#endif
#define _CABSL_TUPLE_SIZE_II( \
  a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, \
  a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, \
  a41, a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54, a55, a56, a57, a58, a59, a60, \
  a61, a62, a63, a64, a65, a66, a67, a68, a69, a70, a71, a72, a73, a74, a75, a76, a77, a78, a79, a80, \
  a81, a82, a83, a84, a85, a86, a87, a88, a89, a90, a91, a92, a93, a94, a95, a96, a97, a98, a99, a100, ...) a100
#define _CABSL_TUPLE_SIZE_III \
  100, 99, 98, 97, 96, 95, 94, 93, 92, 91, 90, 89, 88, 87, 86, 85, 84, 83, 82, 81, 80, \
  79, 78, 77, 76, 75, 74, 73, 72, 71, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, \
  59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, \
  39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, \
  19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1

/**
 * Determine whether a sequence is of the form "(a) b" or "(a)(b) c".
 * In the first case, 1 is returned, otherwise 2.
 */
#if !defined _CABSL_MSC  || defined __INTELLISENSE__ || defined Q_MOC_RUN
#define _CABSL_SEQ_SIZE(...) _CABSL_CAT(_CABSL_SEQ_SIZE, _CABSL_SEQ_SIZE_0 __VA_ARGS__))
#define _CABSL_SEQ_SIZE_0(...) _CABSL_SEQ_SIZE_1
#define _CABSL_SEQ_SIZE_1(...) _CABSL_SEQ_SIZE_2
#define _CABSL_SEQ_SIZE_CABSL_SEQ_SIZE_0 0 _CABSL_DROP(
#define _CABSL_SEQ_SIZE_CABSL_SEQ_SIZE_1 1 _CABSL_DROP(
#define _CABSL_SEQ_SIZE_CABSL_SEQ_SIZE_2 2 _CABSL_DROP(
#else
#define _CABSL_SEQ_SIZE(...) _CABSL_JOIN(_CABSL_SEQ_SIZE, _CABSL_SEQ_SIZE_I(_CABSL_SEQ_SIZE_I(_CABSL_SEQ_SIZE_II(__VA_ARGS__)))))
#define _CABSL_SEQ_SIZE_I(...) _CABSL_CAT(_CABSL_DROP, __VA_ARGS__)
#define _CABSL_SEQ_SIZE_II(...) _CABSL_DROP __VA_ARGS__
#define _CABSL_SEQ_SIZE_CABSL_DROP 2 _CABSL_DROP(
#define _CABSL_SEQ_SIZE_CABSL_DROP_CABSL_DROP 1 _CABSL_DROP (
#define _CABSL_SEQ_SIZE_CABSL_DROP_CABSL_DROP_CABSL_DROP 0 _CABSL_DROP (
#endif

/**
 * Apply a macro to all elements of a tuple.
 */
#define _CABSL_ATTR_1(f, a1)
#define _CABSL_ATTR_2(f, a1, a2) f(a1)
#define _CABSL_ATTR_3(f, a1, a2, a3) f(a1) f(a2)
#define _CABSL_ATTR_4(f, a1, a2, a3, a4) f(a1) f(a2) f(a3)
#define _CABSL_ATTR_5(f, a1, a2, a3, a4, a5) f(a1) f(a2) f(a3) f(a4)
#define _CABSL_ATTR_6(f, a1, a2, a3, a4, a5, a6) f(a1) f(a2) f(a3) f(a4) f(a5)
#define _CABSL_ATTR_7(f, a1, a2, a3, a4, a5, a6, a7) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6)
#define _CABSL_ATTR_8(f, a1, a2, a3, a4, a5, a6, a7, a8) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7)
#define _CABSL_ATTR_9(f, a1, a2, a3, a4, a5, a6, a7, a8, a9) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8)
#define _CABSL_ATTR_10(f, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8) f(a9)
#define _CABSL_ATTR_11(f, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8) f(a9) f(a10)
#define _CABSL_ATTR_12(f, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8) f(a9) f(a10) f(a11)
#define _CABSL_ATTR_13(f, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8) f(a9) f(a10) f(a11) f(a12)
#define _CABSL_ATTR_14(f, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8) f(a9) f(a10) f(a11) f(a12) f(a13)
#define _CABSL_ATTR_15(f, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8) f(a9) f(a10) f(a11) f(a12) f(a13) f(a14)
#define _CABSL_ATTR_16(f, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8) f(a9) f(a10) f(a11) f(a12) f(a13) f(a14) f(a15)
#define _CABSL_ATTR_17(f, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8) f(a9) f(a10) f(a11) f(a12) f(a13) f(a14) f(a15) f(a16)
#define _CABSL_ATTR_18(f, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8) f(a9) f(a10) f(a11) f(a12) f(a13) f(a14) f(a15) f(a16) f(a17)
#define _CABSL_ATTR_19(f, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8) f(a9) f(a10) f(a11) f(a12) f(a13) f(a14) f(a15) f(a16) f(a17) f(a18)
#define _CABSL_ATTR_20(f, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8) f(a9) f(a10) f(a11) f(a12) f(a13) f(a14) f(a15) f(a16) f(a17) f(a18) f(a19)
#define _CABSL_ATTR_21(f, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8) f(a9) f(a10) f(a11) f(a12) f(a13) f(a14) f(a15) f(a16) f(a17) f(a18) f(a19) f(a20)
#define _CABSL_ATTR_22(f, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8) f(a9) f(a10) f(a11) f(a12) f(a13) f(a14) f(a15) f(a16) f(a17) f(a18) f(a19) f(a20) f(a21)
#define _CABSL_ATTR_23(f, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8) f(a9) f(a10) f(a11) f(a12) f(a13) f(a14) f(a15) f(a16) f(a17) f(a18) f(a19) f(a20) f(a21) f(a22)
#define _CABSL_ATTR_24(f, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8) f(a9) f(a10) f(a11) f(a12) f(a13) f(a14) f(a15) f(a16) f(a17) f(a18) f(a19) f(a20) f(a21) f(a22) f(a23)
#define _CABSL_ATTR_25(f, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8) f(a9) f(a10) f(a11) f(a12) f(a13) f(a14) f(a15) f(a16) f(a17) f(a18) f(a19) f(a20) f(a21) f(a22) f(a23) f(a24)
#define _CABSL_ATTR_26(f, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8) f(a9) f(a10) f(a11) f(a12) f(a13) f(a14) f(a15) f(a16) f(a17) f(a18) f(a19) f(a20) f(a21) f(a22) f(a23) f(a24) f(a25)
#define _CABSL_ATTR_27(f, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8) f(a9) f(a10) f(a11) f(a12) f(a13) f(a14) f(a15) f(a16) f(a17) f(a18) f(a19) f(a20) f(a21) f(a22) f(a23) f(a24) f(a25) f(a26)
#define _CABSL_ATTR_28(f, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8) f(a9) f(a10) f(a11) f(a12) f(a13) f(a14) f(a15) f(a16) f(a17) f(a18) f(a19) f(a20) f(a21) f(a22) f(a23) f(a24) f(a25) f(a26) f(a27)
#define _CABSL_ATTR_29(f, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8) f(a9) f(a10) f(a11) f(a12) f(a13) f(a14) f(a15) f(a16) f(a17) f(a18) f(a19) f(a20) f(a21) f(a22) f(a23) f(a24) f(a25) f(a26) f(a27) f(a28)
#define _CABSL_ATTR_30(f, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8) f(a9) f(a10) f(a11) f(a12) f(a13) f(a14) f(a15) f(a16) f(a17) f(a18) f(a19) f(a20) f(a21) f(a22) f(a23) f(a24) f(a25) f(a26) f(a27) f(a28) f(a29)
#define _CABSL_ATTR_31(f, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8) f(a9) f(a10) f(a11) f(a12) f(a13) f(a14) f(a15) f(a16) f(a17) f(a18) f(a19) f(a20) f(a21) f(a22) f(a23) f(a24) f(a25) f(a26) f(a27) f(a28) f(a29) f(a30)
#define _CABSL_ATTR_32(f, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8) f(a9) f(a10) f(a11) f(a12) f(a13) f(a14) f(a15) f(a16) f(a17) f(a18) f(a19) f(a20) f(a21) f(a22) f(a23) f(a24) f(a25) f(a26) f(a27) f(a28) f(a29) f(a30) f(a31)
#define _CABSL_ATTR_33(f, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8) f(a9) f(a10) f(a11) f(a12) f(a13) f(a14) f(a15) f(a16) f(a17) f(a18) f(a19) f(a20) f(a21) f(a22) f(a23) f(a24) f(a25) f(a26) f(a27) f(a28) f(a29) f(a30) f(a31) f(a32)
#define _CABSL_ATTR_34(f, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8) f(a9) f(a10) f(a11) f(a12) f(a13) f(a14) f(a15) f(a16) f(a17) f(a18) f(a19) f(a20) f(a21) f(a22) f(a23) f(a24) f(a25) f(a26) f(a27) f(a28) f(a29) f(a30) f(a31) f(a32) f(a33)
#define _CABSL_ATTR_35(f, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8) f(a9) f(a10) f(a11) f(a12) f(a13) f(a14) f(a15) f(a16) f(a17) f(a18) f(a19) f(a20) f(a21) f(a22) f(a23) f(a24) f(a25) f(a26) f(a27) f(a28) f(a29) f(a30) f(a31) f(a32) f(a33) f(a34)
#define _CABSL_ATTR_36(f, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8) f(a9) f(a10) f(a11) f(a12) f(a13) f(a14) f(a15) f(a16) f(a17) f(a18) f(a19) f(a20) f(a21) f(a22) f(a23) f(a24) f(a25) f(a26) f(a27) f(a28) f(a29) f(a30) f(a31) f(a32) f(a33) f(a34) f(a35)
#define _CABSL_ATTR_37(f, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8) f(a9) f(a10) f(a11) f(a12) f(a13) f(a14) f(a15) f(a16) f(a17) f(a18) f(a19) f(a20) f(a21) f(a22) f(a23) f(a24) f(a25) f(a26) f(a27) f(a28) f(a29) f(a30) f(a31) f(a32) f(a33) f(a34) f(a35) f(a36)
#define _CABSL_ATTR_38(f, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8) f(a9) f(a10) f(a11) f(a12) f(a13) f(a14) f(a15) f(a16) f(a17) f(a18) f(a19) f(a20) f(a21) f(a22) f(a23) f(a24) f(a25) f(a26) f(a27) f(a28) f(a29) f(a30) f(a31) f(a32) f(a33) f(a34) f(a35) f(a36) f(a37)
#define _CABSL_ATTR_39(f, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8) f(a9) f(a10) f(a11) f(a12) f(a13) f(a14) f(a15) f(a16) f(a17) f(a18) f(a19) f(a20) f(a21) f(a22) f(a23) f(a24) f(a25) f(a26) f(a27) f(a28) f(a29) f(a30) f(a31) f(a32) f(a33) f(a34) f(a35) f(a36) f(a37) f(a38)
#define _CABSL_ATTR_40(f, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8) f(a9) f(a10) f(a11) f(a12) f(a13) f(a14) f(a15) f(a16) f(a17) f(a18) f(a19) f(a20) f(a21) f(a22) f(a23) f(a24) f(a25) f(a26) f(a27) f(a28) f(a29) f(a30) f(a31) f(a32) f(a33) f(a34) f(a35) f(a36) f(a37) f(a38) f(a39)
#define _CABSL_ATTR_41(f, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8) f(a9) f(a10) f(a11) f(a12) f(a13) f(a14) f(a15) f(a16) f(a17) f(a18) f(a19) f(a20) f(a21) f(a22) f(a23) f(a24) f(a25) f(a26) f(a27) f(a28) f(a29) f(a30) f(a31) f(a32) f(a33) f(a34) f(a35) f(a36) f(a37) f(a38) f(a39) f(a40)
#define _CABSL_ATTR_42(f, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8) f(a9) f(a10) f(a11) f(a12) f(a13) f(a14) f(a15) f(a16) f(a17) f(a18) f(a19) f(a20) f(a21) f(a22) f(a23) f(a24) f(a25) f(a26) f(a27) f(a28) f(a29) f(a30) f(a31) f(a32) f(a33) f(a34) f(a35) f(a36) f(a37) f(a38) f(a39) f(a40) f(a41)
#define _CABSL_ATTR_43(f, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8) f(a9) f(a10) f(a11) f(a12) f(a13) f(a14) f(a15) f(a16) f(a17) f(a18) f(a19) f(a20) f(a21) f(a22) f(a23) f(a24) f(a25) f(a26) f(a27) f(a28) f(a29) f(a30) f(a31) f(a32) f(a33) f(a34) f(a35) f(a36) f(a37) f(a38) f(a39) f(a40) f(a41) f(a42)
#define _CABSL_ATTR_44(f, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8) f(a9) f(a10) f(a11) f(a12) f(a13) f(a14) f(a15) f(a16) f(a17) f(a18) f(a19) f(a20) f(a21) f(a22) f(a23) f(a24) f(a25) f(a26) f(a27) f(a28) f(a29) f(a30) f(a31) f(a32) f(a33) f(a34) f(a35) f(a36) f(a37) f(a38) f(a39) f(a40) f(a41) f(a42) f(a43)
#define _CABSL_ATTR_45(f, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8) f(a9) f(a10) f(a11) f(a12) f(a13) f(a14) f(a15) f(a16) f(a17) f(a18) f(a19) f(a20) f(a21) f(a22) f(a23) f(a24) f(a25) f(a26) f(a27) f(a28) f(a29) f(a30) f(a31) f(a32) f(a33) f(a34) f(a35) f(a36) f(a37) f(a38) f(a39) f(a40) f(a41) f(a42) f(a43) f(a44)
#define _CABSL_ATTR_46(f, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8) f(a9) f(a10) f(a11) f(a12) f(a13) f(a14) f(a15) f(a16) f(a17) f(a18) f(a19) f(a20) f(a21) f(a22) f(a23) f(a24) f(a25) f(a26) f(a27) f(a28) f(a29) f(a30) f(a31) f(a32) f(a33) f(a34) f(a35) f(a36) f(a37) f(a38) f(a39) f(a40) f(a41) f(a42) f(a43) f(a44) f(a45)
#define _CABSL_ATTR_47(f, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8) f(a9) f(a10) f(a11) f(a12) f(a13) f(a14) f(a15) f(a16) f(a17) f(a18) f(a19) f(a20) f(a21) f(a22) f(a23) f(a24) f(a25) f(a26) f(a27) f(a28) f(a29) f(a30) f(a31) f(a32) f(a33) f(a34) f(a35) f(a36) f(a37) f(a38) f(a39) f(a40) f(a41) f(a42) f(a43) f(a44) f(a45) f(a46)
#define _CABSL_ATTR_48(f, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8) f(a9) f(a10) f(a11) f(a12) f(a13) f(a14) f(a15) f(a16) f(a17) f(a18) f(a19) f(a20) f(a21) f(a22) f(a23) f(a24) f(a25) f(a26) f(a27) f(a28) f(a29) f(a30) f(a31) f(a32) f(a33) f(a34) f(a35) f(a36) f(a37) f(a38) f(a39) f(a40) f(a41) f(a42) f(a43) f(a44) f(a45) f(a46) f(a47)
#define _CABSL_ATTR_49(f, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8) f(a9) f(a10) f(a11) f(a12) f(a13) f(a14) f(a15) f(a16) f(a17) f(a18) f(a19) f(a20) f(a21) f(a22) f(a23) f(a24) f(a25) f(a26) f(a27) f(a28) f(a29) f(a30) f(a31) f(a32) f(a33) f(a34) f(a35) f(a36) f(a37) f(a38) f(a39) f(a40) f(a41) f(a42) f(a43) f(a44) f(a45) f(a46) f(a47) f(a48)
#define _CABSL_ATTR_50(f, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8) f(a9) f(a10) f(a11) f(a12) f(a13) f(a14) f(a15) f(a16) f(a17) f(a18) f(a19) f(a20) f(a21) f(a22) f(a23) f(a24) f(a25) f(a26) f(a27) f(a28) f(a29) f(a30) f(a31) f(a32) f(a33) f(a34) f(a35) f(a36) f(a37) f(a38) f(a39) f(a40) f(a41) f(a42) f(a43) f(a44) f(a45) f(a46) f(a47) f(a48) f(a49)
#define _CABSL_ATTR_51(f, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50, a51) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8) f(a9) f(a10) f(a11) f(a12) f(a13) f(a14) f(a15) f(a16) f(a17) f(a18) f(a19) f(a20) f(a21) f(a22) f(a23) f(a24) f(a25) f(a26) f(a27) f(a28) f(a29) f(a30) f(a31) f(a32) f(a33) f(a34) f(a35) f(a36) f(a37) f(a38) f(a39) f(a40) f(a41) f(a42) f(a43) f(a44) f(a45) f(a46) f(a47) f(a48) f(a49) f(a50)
#define _CABSL_ATTR_52(f, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8) f(a9) f(a10) f(a11) f(a12) f(a13) f(a14) f(a15) f(a16) f(a17) f(a18) f(a19) f(a20) f(a21) f(a22) f(a23) f(a24) f(a25) f(a26) f(a27) f(a28) f(a29) f(a30) f(a31) f(a32) f(a33) f(a34) f(a35) f(a36) f(a37) f(a38) f(a39) f(a40) f(a41) f(a42) f(a43) f(a44) f(a45) f(a46) f(a47) f(a48) f(a49) f(a50) f(a51)
#define _CABSL_ATTR_53(f, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8) f(a9) f(a10) f(a11) f(a12) f(a13) f(a14) f(a15) f(a16) f(a17) f(a18) f(a19) f(a20) f(a21) f(a22) f(a23) f(a24) f(a25) f(a26) f(a27) f(a28) f(a29) f(a30) f(a31) f(a32) f(a33) f(a34) f(a35) f(a36) f(a37) f(a38) f(a39) f(a40) f(a41) f(a42) f(a43) f(a44) f(a45) f(a46) f(a47) f(a48) f(a49) f(a50) f(a51) f(a52)
#define _CABSL_ATTR_54(f, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8) f(a9) f(a10) f(a11) f(a12) f(a13) f(a14) f(a15) f(a16) f(a17) f(a18) f(a19) f(a20) f(a21) f(a22) f(a23) f(a24) f(a25) f(a26) f(a27) f(a28) f(a29) f(a30) f(a31) f(a32) f(a33) f(a34) f(a35) f(a36) f(a37) f(a38) f(a39) f(a40) f(a41) f(a42) f(a43) f(a44) f(a45) f(a46) f(a47) f(a48) f(a49) f(a50) f(a51) f(a52) f(a53)
#define _CABSL_ATTR_55(f, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54, a55) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8) f(a9) f(a10) f(a11) f(a12) f(a13) f(a14) f(a15) f(a16) f(a17) f(a18) f(a19) f(a20) f(a21) f(a22) f(a23) f(a24) f(a25) f(a26) f(a27) f(a28) f(a29) f(a30) f(a31) f(a32) f(a33) f(a34) f(a35) f(a36) f(a37) f(a38) f(a39) f(a40) f(a41) f(a42) f(a43) f(a44) f(a45) f(a46) f(a47) f(a48) f(a49) f(a50) f(a51) f(a52) f(a53) f(a54)
#define _CABSL_ATTR_56(f, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54, a55, a56) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8) f(a9) f(a10) f(a11) f(a12) f(a13) f(a14) f(a15) f(a16) f(a17) f(a18) f(a19) f(a20) f(a21) f(a22) f(a23) f(a24) f(a25) f(a26) f(a27) f(a28) f(a29) f(a30) f(a31) f(a32) f(a33) f(a34) f(a35) f(a36) f(a37) f(a38) f(a39) f(a40) f(a41) f(a42) f(a43) f(a44) f(a45) f(a46) f(a47) f(a48) f(a49) f(a50) f(a51) f(a52) f(a53) f(a54) f(a55)
#define _CABSL_ATTR_57(f, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54, a55, a56, a57) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8) f(a9) f(a10) f(a11) f(a12) f(a13) f(a14) f(a15) f(a16) f(a17) f(a18) f(a19) f(a20) f(a21) f(a22) f(a23) f(a24) f(a25) f(a26) f(a27) f(a28) f(a29) f(a30) f(a31) f(a32) f(a33) f(a34) f(a35) f(a36) f(a37) f(a38) f(a39) f(a40) f(a41) f(a42) f(a43) f(a44) f(a45) f(a46) f(a47) f(a48) f(a49) f(a50) f(a51) f(a52) f(a53) f(a54) f(a55) f(a56)
#define _CABSL_ATTR_58(f, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54, a55, a56, a57, a58) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8) f(a9) f(a10) f(a11) f(a12) f(a13) f(a14) f(a15) f(a16) f(a17) f(a18) f(a19) f(a20) f(a21) f(a22) f(a23) f(a24) f(a25) f(a26) f(a27) f(a28) f(a29) f(a30) f(a31) f(a32) f(a33) f(a34) f(a35) f(a36) f(a37) f(a38) f(a39) f(a40) f(a41) f(a42) f(a43) f(a44) f(a45) f(a46) f(a47) f(a48) f(a49) f(a50) f(a51) f(a52) f(a53) f(a54) f(a55) f(a56) f(a57)
#define _CABSL_ATTR_59(f, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54, a55, a56, a57, a58, a59) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8) f(a9) f(a10) f(a11) f(a12) f(a13) f(a14) f(a15) f(a16) f(a17) f(a18) f(a19) f(a20) f(a21) f(a22) f(a23) f(a24) f(a25) f(a26) f(a27) f(a28) f(a29) f(a30) f(a31) f(a32) f(a33) f(a34) f(a35) f(a36) f(a37) f(a38) f(a39) f(a40) f(a41) f(a42) f(a43) f(a44) f(a45) f(a46) f(a47) f(a48) f(a49) f(a50) f(a51) f(a52) f(a53) f(a54) f(a55) f(a56) f(a57) f(a58)
#define _CABSL_ATTR_60(f, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54, a55, a56, a57, a58, a59, a60) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8) f(a9) f(a10) f(a11) f(a12) f(a13) f(a14) f(a15) f(a16) f(a17) f(a18) f(a19) f(a20) f(a21) f(a22) f(a23) f(a24) f(a25) f(a26) f(a27) f(a28) f(a29) f(a30) f(a31) f(a32) f(a33) f(a34) f(a35) f(a36) f(a37) f(a38) f(a39) f(a40) f(a41) f(a42) f(a43) f(a44) f(a45) f(a46) f(a47) f(a48) f(a49) f(a50) f(a51) f(a52) f(a53) f(a54) f(a55) f(a56) f(a57) f(a58) f(a59)
#define _CABSL_ATTR_61(f, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54, a55, a56, a57, a58, a59, a60, a61) f(a1) f(a2) f(a3) f(a4) f(a5) f(a6) f(a7) f(a8) f(a9) f(a10) f(a11) f(a12) f(a13) f(a14) f(a15) f(a16) f(a17) f(a18) f(a19) f(a20) f(a21) f(a22) f(a23) f(a24) f(a25) f(a26) f(a27) f(a28) f(a29) f(a30) f(a31) f(a32) f(a33) f(a34) f(a35) f(a36) f(a37) f(a38) f(a39) f(a40) f(a41) f(a42) f(a43) f(a44) f(a45) f(a46) f(a47) f(a48) f(a49) f(a50) f(a51) f(a52) f(a53) f(a54) f(a55) f(a56) f(a57) f(a58) f(a59) f(a60)

/** Simply drop all parameters passed. */
#define _CABSL_DROP(...)

/** Concatenate the two parameters. Works in some situations. */
#define _CABSL_CAT(a, ...) _CABSL_CAT_I(a, __VA_ARGS__)
#define _CABSL_CAT_I(a, ...) a ## __VA_ARGS__

/** Concatenate the two parameters. Works in other situations. */
#define _CABSL_JOIN(a, b) _CABSL_JOIN_I(a, b)
#define _CABSL_JOIN_I(a, b) _CABSL_JOIN_II(a ## b)
#define _CABSL_JOIN_II(res) res

/** Generate the actual declaration. */
#define _CABSL_DECL_IV(...) _CABSL_VAR(__VA_ARGS__) _CABSL_DROP(_CABSL_DROP(

/** Extract the variable from the declaration. */
#define _CABSL_VAR(...) _CABSL_JOIN(_CABSL_VAR_, _CABSL_SEQ_SIZE(__VA_ARGS__))(__VA_ARGS__)
#define _CABSL_VAR_0(...) __VA_ARGS__
#define _CABSL_VAR_1(...) _CABSL_VAR_1_I(__VA_ARGS__)
#define _CABSL_VAR_1_I(...) _CABSL_VAR_1_II __VA_ARGS__
#define _CABSL_VAR_1_II(...)
#define _CABSL_VAR_2(...) _CABSL_VAR_2_I(__VA_ARGS__)
#define _CABSL_VAR_2_I(...) _CABSL_VAR_2_II __VA_ARGS__
#define _CABSL_VAR_2_II(...) _CABSL_VAR_2_III
#define _CABSL_VAR_2_III(...)

/** Generate the initialisation code from the declaration if required. */
#define _CABSL_INIT(seq) _CABSL_JOIN(_CABSL_INIT_I_, _CABSL_SEQ_SIZE(seq))(seq)
#define _CABSL_INIT_I_1(...)
#define _CABSL_INIT_I_2(...) = _CABSL_DECL_IV __VA_ARGS__))(_CABSL_INIT_II __VA_ARGS__) _CABSL_INIT_I_2_I(__VA_ARGS__))
#define _CABSL_INIT_I_2_I(...) _CABSL_INIT_I_2_II __VA_ARGS__)
#define _CABSL_INIT_I_2_II(...) _CABSL_INIT_I_2_III
#define _CABSL_INIT_I_2_III(...) __VA_ARGS__ _CABSL_DROP(
#define _CABSL_INIT_II(...) _CABSL_DROP(
