/*
 * Finite State Machine base class definition.
 *
 * @author Michel Megens
 * @email  michel@michelmegens.net
 */

/**
 * @file fsm_base.h FSM base header.
 */

#pragma once

#include <lwiot/types.h>

#include "fsm_policy.h"

#include <lwiot/stl/tuple.h>
#include <lwiot/stl/pair.h>
#include <lwiot/stl/referencewrapper.h>
#include <lwiot/stl/sharedpointer.h>
#include <lwiot/stl/array.h>

#include <lwiot/kernel/uniquelock.h>

#include <lwiot/traits/integralconstant.h>
#include <lwiot/traits/isintegral.h>
#include <lwiot/traits/ismoveconstructible.h>
#include <lwiot/traits/iscopyconstructible.h>
#include <lwiot/traits/isconvirtible.h>
#include <lwiot/traits/issame.h>

namespace lwiot
{
	namespace detail_fsm
	{
		template<size_t... Indices>
		struct IndexSequence {
			using Type = IndexSequence<Indices...>;

			constexpr size_t size() const noexcept
			{
				return sizeof...(Indices);
			}
		};

		template<size_t idx, size_t... Indices>
		struct MakeIndexSequence : MakeIndexSequence<idx - 1, idx - 1, Indices...> {
		};

		template<size_t... Indices>
		struct MakeIndexSequence<0, Indices...> {
			using Type = IndexSequence<Indices...>;

			static constexpr size_t size() noexcept
			{
				return sizeof...(Indices);
			}
		};

		/**
		 * @brief Check if the FSM policy defines a threading model.
		 * @tparam T Policy type.
		 * @ingroup fsm
		 */
		template<typename T>
		struct HasThreading {
		public:
			template<typename U>
			static constexpr traits::TrueType test(typename U::Threading *)
			{
				return {};
			}

			template<typename>
			static constexpr traits::FalseType test(...)
			{
				return {};
			}

		private:
			enum data {
				value = !!decltype(test<T>(0))()
			};

		public:
			static constexpr bool Value = data::value; //!< Indicator if \p T defines a threading model.
		};

		/**
		 * @brief Select a threading policy.
		 * @tparam T Policy type.
		 * @tparam threading Indicator if \p T has a threading model.
		 * @see HasThreading
		 * @ingroup fsm
		 */
		template<typename T, bool threading>
		struct SelectThreadingPolicy;

		template<typename T>
		struct SelectThreadingPolicy<T, true> {
			typedef typename T::Threading Type;
		};

		template<typename T>
		struct SelectThreadingPolicy<T, false> {
			typedef SingleThreading Type;
		};

		template<typename... T>
		struct TemplateForAll;

		template<>
		struct TemplateForAll<> : public traits::TrueType {
		};

		template<typename... T>
		struct TemplateForAll<traits::FalseType, T...> : public traits::FalseType {
		};

		template<typename... T>
		struct TemplateForAll<traits::TrueType, T...> : public TemplateForAll<T...>::type {
		};

		typedef uint32_t FsmStateId; //!< FSM state ID type.

		/**
		 * FSM base helper. The helper checks the validity of the argument types. All arguments should be
		 * both move and copy constructible.
		 *
		 * @tparam W Watchdog type.
		 * @tparam Args Handler arguments.
		 * @ingroup fsm
		 */
		template<typename W, typename... Args>
		struct FsmBase_helper {
			/**
			 * Create a new FsmBase_helper object and construct a default watchdog object.
			 */
			explicit FsmBase_helper() : m_watchdog()
			{
			}

			FsmBase_helper(FsmBase_helper&& other) noexcept = default;
			FsmBase_helper(const FsmBase_helper& other) = default;

			FsmBase_helper& operator=(FsmBase_helper&& other) noexcept = default;
			FsmBase_helper& operator=(const FsmBase_helper& other) = default;

			/**
			 * Create a new FsmBase_helper object and construct a default watchdog object, setting the WDT
			 * timeout to \p timeout.
			 *
			 * @param timeout WDT timeout.
			 */
			explicit FsmBase_helper(time_t timeout) : m_watchdog()
			{
				this->m_watchdog.enable(timeout);
			}

		private:
			typedef W WatchdogType; //!< FSM watchdog type definition.

			/**
			 * Check if all arguments are move constructible.
			 *
			 * @return True or false.
			 */
			static constexpr bool IsMoveConstructible()
			{
				return TemplateForAll<typename traits::IsMoveConstructible<Args>::type...>::value;
			}

			/**
			 * Check if all arguments are copy constructible.
			 *
			 * @return True or false.
			 */
			static constexpr bool IsCopyConstructible()
			{
				return TemplateForAll<typename traits::IsCopyConstructible<Args>::type...>::value;
			}

			/**
			 * Check if the Watchdog type is constructible.
			 *
			 * @return True or false.
			 */
			static constexpr bool CanCreateWatchdog()
			{
				return traits::IsConstructible<WatchdogType>::value;
			}


		protected:
			WatchdogType m_watchdog; //!< FSM watchdog.

			static_assert(FsmBase_helper::IsCopyConstructible(), "Arguments must be copy constructible!");
			static_assert(FsmBase_helper::IsMoveConstructible(), "Arguments must be move constructible!");
			static_assert(FsmBase_helper::CanCreateWatchdog(), "Unable to construct watchdog type!");
		};

		/**
		 * @brief FSM status.
		 * @ingroup fsm
		 */
		enum class FsmStatus {
			StateUnchanged, //!< No transition has taken place.
			StateChanged, //!< The FSM transitioned from one state to another.
			Fault, //!< Fatal error indicator.
			Error, //!< Error indicator. The error state has been executed.
			Stopped, //!< FSM is not running.
			Running //!< FSM is running.
		};

		/**
		 * @brief Transition type definition.
		 * @tparam P Policy type.
		 * @tparam Args Handler argument types.
		 * @ingroup fsm
		 */
		template<typename P, typename... Args>
		class Transition {
		public:
			typedef P PolicyType; //!< Policy type definition.
			typedef typename P::FsmEvent FsmEventType; //!< FSM event type definition.
			typedef Function<bool(Args...)> GuardHandlerType; //!< Guard handler type.
			typedef FsmStateId FsmStateIdType; //!< State ID type.

			/**
			 * @brief Create a new transition object.
			 */
			explicit Transition() : m_handler(), m_event(), m_next()
			{
			}

			/**
			 * @brief Create a new transition.
			 * @param event Alphabet symbol that triggers this transition.
			 * @param next State this transition transitions into.
			 * @param handler Guard handler.
			 */
			explicit Transition(const FsmEventType &event, const FsmStateIdType &next, const GuardHandlerType &handler) :
					m_handler(handler), m_event(event), m_next(next)
			{
			}

			/**
			 * @brief Create a new transition.
			 * @param event Alphabet symbol that triggers this transition.
			 * @param next State this transition transitions into.
			 */
			explicit Transition(const FsmEventType &event, const FsmStateIdType &next) :
					m_handler(), m_event(event), m_next(next)
			{
			}

			/**
			 * @brief Execute the guard handler.
			 * @param args Guard handler arguments.
			 * @return True or false based on whether or not the guard handler succeeded or not.
			 */
			bool guard(Args &&... args)
			{
				return this->m_handler(stl::forward<Args>(args)...);
			}

			/**
			 * @brief Check if this transition has a guard handler.
			 * @return True or false based on whether or not this state has a guard handler.
			 */
			bool hasGuard() const
			{
				return static_cast<bool>(this->m_handler.valid());
			}

			/**
			 * @brief Getter for the event (alphabet symbol).
			 * @return The alphabet symbol tied to this transition.
			 */
			const FsmEventType &event() const
			{
				return this->m_event;
			}

			/**
			 * @brief Equality operator.
			 * @param eventId Alphabet symbol to test against.
			 * @return True or false based on whether or not the transition and \p eventId match.
			 */
			bool operator==(const FsmEventType &eventId) const
			{
				return this->m_event == eventId;
			}

			/**
			 * @brief Inequality operator.
			 * @param eventId Alphabet symbol to test against.
			 * @return The inverse of the equality operator.
			 */
			bool operator!=(const FsmEventType &eventId) const
			{
				return this->m_event != eventId;
			}

			/**
			 * @brief Set the guard handler.
			 * @tparam Func Guard handler type.
			 * @param func Guard handler to assign to this transition.
			 */
			template<typename Func>
			void setGuard(Func &&func)
			{
				this->m_handler = stl::forward<Func>(func);
			}

			/**
			 * @brief Set the guard handler.
			 * @param guard Guard handler to set.
			 */
			void setGuard(const GuardHandlerType &guard)
			{
				this->m_handler = guard;
			}

			/**
			 * @brief Set the alphabet symbol of this transition.
			 * @param event Alphabet symbol.
			 */
			void setEvent(const FsmEventType &event)
			{
				this->m_event = event;
			}

			/**
			 * @brief Set the state ID of the state this transition transitions into.
			 * @param next The state this transition leads to.
			 */
			void setNext(const FsmStateIdType &next)
			{
				this->m_next = next;
			}

			/**
			 * @brief Getter for the continuation state.
			 * @return The continuation state of this transition.
			 */
			const FsmStateIdType &next() const
			{
				return this->m_next;
			}

			constexpr operator bool() const
			{
				return this->m_next && this->m_event;
			}

		private:
			GuardHandlerType m_handler; //!< Guard handler.
			FsmEventType m_event; //!< Alphabet symbol tied to the transition.
			FsmStateIdType m_next; //!< Continuation state.
		};

		/**
		 * @brief FSM state definition.
		 * @tparam P Policy type.
		 * @tparam H Handler type.
		 * @tparam R Return type.
		 * @tparam Args Handler arguments.
		 * @ingroup fsm
		 */
		template<typename P, typename H, typename R, typename... Args>
		class State {
			typedef P PolicyType; //!< Policy type.
			typedef H HandlerType; //!< State handler type definition.
			typedef Transition<P, Args...> TransitionType; //!< Transition type.
			typedef typename PolicyType::FsmEvent FsmEventType; //!< FSM event type.
			typedef FsmStateId FsmStateIdType; //!< State ID type.

			/**
			 * @brief Dynamic array type definition.
			 * @tparam T value type.
			 */
			template <typename T>
			using ArrayList = typename PolicyType::template ArrayList<T>;

		public:
			/**
			 * @brief Create a new state object.
			 */
			explicit State() : m_id(State::generateFsmStateId()), m_parent()
			{
			}

			/**
			 * @brief Create a new state with \p parent as a parent state.
			 * @param parent Parent state.
			 */
			explicit State(const FsmStateIdType parent) : m_id(generateFsmStateId()), m_parent(parent)
			{
			}

			/**
			 * @brief Invoke the state action.
			 * @tparam ReturnType StateHandler return type.
			 * @param args StateHandler arguments.
			 * @return State return type. Defined by the implementor of FsmBase.
			 */
			template<typename ReturnType = R>
			inline traits::EnableIf_t<traits::Not<traits::IsSame<void, ReturnType>>::value, bool>
			action(Args &&... args)
			{
				return this->m_action(stl::forward<Args>(args)...);
			}

			/**
			 * @brief Invoke the state action.
			 * @tparam ReturnType State return type.
			 * @param args StateHandler arguments.
			 * @return Always true.
			 */
			template<typename ReturnType = R>
			inline traits::EnableIf_t<traits::IsSame<void, ReturnType>::value, bool> action(Args &&... args)
			{
				this->m_action(stl::forward<Args>(args)...);
				return true;
			}

			/**
			 * @brief Set the state ID.
			 * @param id State ID to set.
			 */
			void setId(const FsmStateIdType &id)
			{
				this->m_id = id;
			}

			/**
			 * @brief Set a parent state by ID.
			 * @param id ID of the parent state.
			 */
			void setParent(const FsmStateIdType &id)
			{
				this->m_parent = id;
			}

			/**
			 * @brief Set a parent state.
			 * @param state State to set as parent.
			 */
			void setParent(const State &state)
			{
				this->m_parent = state.m_id;
			}

			/**
			 * @brief State ID getter.
			 * @return The state ID.
			 */
			const FsmStateIdType &id() const
			{
				return this->m_id;
			}

			/**
			 * @brief Parent state ID getter.
			 * @return The ID of the parent state.
			 * @note The return value of this function is undefined if State::hasParent returns false.
			 */
			const FsmStateIdType &parent() const
			{
				return this->m_parent;
			}

			/**
			 * @brief StateHandler setter.
			 * @param handler State handler to bind to this state.
			 */
			void setAction(const HandlerType &handler)
			{
				this->m_action = handler;
			}

			/**
			 * @brief StateHandler setter.
			 * @tparam Action StateHandler type.
			 * @param act Handler to set.
			 */
			template<typename Action>
			void setAction(Action &&act)
			{
				this->m_action = stl::forward<Action>(act);
			}

			/**
			 * @brief Check if this state has a valid handler.
			 * @return True or false based on whether or not this state has a valid handler.
			 */
			bool hasAction() const
			{
				return static_cast<bool>(this->m_action);
			}

			/**
			 * @brief Check if this state has a valid parent object.
			 * @return True or false based on whether or not this state has a valid parent object.
			 */
			bool hasParent() const
			{
				return static_cast<bool>(this->m_parent);
			}

		private:
			FsmStateIdType m_id; //!< State ID.
			FsmStateIdType m_parent; //!< Parent state ID.

			HandlerType m_action; //!< State handler object.

			/* METHODS */

			/**
			 * @brief Generate a unique state ID.
			 * @return A unique state ID type.
			 */
			static FsmStateIdType generateFsmStateId()
			{
				FsmStateIdType result = 0UL;

				for(auto num = 0UL; num < sizeof(FsmStateIdType); num++) {
					auto r = lwiot::random();
					r %= 1U << BITS_PER_BYTE;

					result |= r << (num * BITS_PER_BYTE);
				}

				return result;
			}
		};

		/**
		 * @brief Union type to index the FSM state transition table.
		 * @tparam S State ID type.
		 * @tparam E Event ID type.
		 * @ingroup fsm
		 */
		template <typename S, typename E>
		union SttIndex {
			typedef uint64_t IntegerType; //!< Integer type.

			/**
			 * @brief Conversion operator to IntegerType.
			 * @return The IntegerType representation of SttIndex.
			 */
			operator IntegerType() const
			{
				return this->integer;
			}

			IntegerType integer; //!< Integer value.

			struct {
				typedef S StateIdType; //!< State ID type.
				typedef E EventIdType; //!< Event ID type.

				StateIdType stateId; //!< State ID.
				EventIdType eventId; //!< Event ID.
			} value;
		};

#ifdef DOXYGEN
		/**
		 * @brief FSM base class for various FSM implementations.
		 *
		 * A finite state machine (FSM) is a mathematical model of computation. It is an abstract
		 * machine that can only be in one of a finite number of states. A transition from one state
		 * to the next can either be initiated by the state execution itself, or by external input. A
		 * FSM is defined by defined by a quintuple \f$(\sum,S,s_{0},\delta, F)\f$, where:
		 *
		 *  * \f$\sum\f$ is the input alphabet, a finite non-empty set of symbols
		 *  * \f$S\f$ is a finite non-empty set of states
		 *  * \f$s_{0}\f$ is the initial state (member of S)
		 *  * \f$\delta\f$ is the transition function: \f$\delta : S \times \sum \rightarrow S\f$
		 *  * \f$F\f$ is a finite, possibly empty, set of final states (subset of S)
		 *
		 * @see FsmBase_helper
		 * @tparam Handler State handler type.
		 * @tparam Policy Policy type.
		 * @tparam Watchdog Watchdog type.
		 * @ingroup fsm
		 */
		template <typename Handler, typename Policy, typename Watchdog>
		class FsmBase : public FsmBase_helper<W, Args...>
#else

		template<typename Handler, typename Policy, typename Watchdog>
		class FsmBase;

		template<typename P, typename W, typename R, typename... Args>
		class FsmBase<R(Args...), P, W> : public FsmBase_helper<W, Args...>
#endif
		{
			typedef FsmBase_helper<W, Args...> Base; //!< Base class type.
			typedef FsmBase<R(Args...), P, W> ThisClass; //!< This class definition.
			typedef SttIndex<FsmStateId, typename P::FsmEvent> SttIndexType;

		protected:
			/**
			 * @brief Set to true if a threading model is present on the PolicyType.
			 */
			static constexpr bool Threaded = HasThreading<P>::Value;

		public:
			typedef R ReturnType; //!< Return type definition.
			typedef P PolicyType; //!< Policy type definition.

			typedef typename SelectThreadingPolicy<PolicyType, Threaded>::Type Threading; //!< Threading model definition.
			typedef Function<ReturnType(Args...)> HandlerType; //!< State handler.
			typedef State<P, HandlerType, ReturnType, Args...> StateType; //!< State type.
			typedef typename Threading::Lock LockType; //!< Mutex type.
			typedef typename Threading::Event EventType; //!< Waiting queue type.
			typedef typename PolicyType::FsmEvent FsmEventType; //!< FSM event type.
			typedef Transition<P, Args...> TransitionType; //!< Transition type.
			typedef FsmStateId FsmStateIdType; //!< State ID type.
			typedef stl::SharedPointer<StateType> StatePointer; //!< Smart state pointer type.

			/**
			 * @brief Queue type.
			 * @tparam T Encapsulated type.
			 */
			template<typename T>
			using QueueType = typename PolicyType::template Queue<T>;
			template<typename T>

			/**
			 * @brief Dynamic array type.
			 * @tparam T Encapsulated type.
			 */
			using ArrayListType = typename PolicyType::template ArrayList<T>;
			/**
			 * @brief Map type.
			 * @tparam K Key type.
			 * @tparam V Value type.
			 */
			template<typename K, typename V>
			using MapType = typename PolicyType::template Map<K, V>;
			/**
			 * @brief Unordered set type.
			 * @tparam T Encapsulated type.
			 */
			template<typename T>
			using SetType = typename PolicyType::template Set<T>;

		private:
			/// @name Type check methods
			/// @{

			/**
			 * @brief Check if \p _Tp is an integral type.
			 * @tparam _Tp Type to check.
			 * @return True or false based on whether \p _Tp is an integral type.
			 */
			template <typename _Tp>
			static constexpr bool IsIntegralType()
			{
				return traits::IsIntegral<_Tp>::Value;
			}

			/**
			 * @brief Check if \_Tp is void.
			 * @tparam _Tp Type to check.
			 * @return True or false based on whether \p _Tp is void.
			 *
			 * This method uses traits::IsSame to check \p _Tp.
			 */
			template<typename _Tp>
			constexpr static bool IsVoid()
			{
				return traits::IsSame<_Tp, void>::value;
			}

			/**
			 * @brief Check if \p _Tp is void or bool.
			 * @tparam _Tp Type to check.
			 * @return True or false based on whether \p _Tp is void or bool or not.
			 */
			template<typename _Tp>
			constexpr static bool IsVoidOrBool()
			{
				return IsVoid<_Tp>() || IsBoolable<_Tp>();
			}

			/**
			 * @brief Check if \p _Tp is convirtable to bool.
			 * @tparam _Tp Type to check.
			 * @return  True or false based on whether \p is convirtable to bool or not.
			 */
			template<typename _Tp>
			constexpr static bool IsBoolable()
			{
				constexpr auto is_bool = traits::IsSame<_Tp, bool>::value;
				constexpr auto is_conv = traits::IsConvirtable<_Tp, bool>::value;

				return is_bool || is_conv;
			}

			/// @}

			static_assert(IsIntegralType<FsmStateIdType>(), "State ID should be an integral constant!");
			static_assert(IsIntegralType<FsmEventType>(), "FSM Event type should be an integral constant!");
			static_assert(IsVoidOrBool<ReturnType>(), "Return value should be bool or void!");
			static_assert(IsBoolable<FsmEventType>(), "EventType should be convirtable to bool!");
			static_assert(IsBoolable<FsmStateIdType>(), "EventIdType should be convirtable to bool!");

		protected:
			/**
			 * @brief Argument wrapper type.
			 * @see stl::Tuple
			 *
			 * The state handler argument values are wrapper in the \p ArgumentWrapper while they are stored on
			 * the event queue.
			 */
			using ArgumentWrapper = stl::Tuple<typename traits::RemoveCv<typename traits::RemoveCv<Args>::type>::type...>;

		public:

			/**
			 * @brief Create a new FsmBase object.
			 * @param tmo Timeout for the watchdog timer.
			 */
			explicit FsmBase(time_t tmo = 2000);

			/**
			 * @brief Create a new FsmBase object.
			 * @param silent Indicator if log messages should be suppressed or not.
			 * @param tmo Timeout for the watchdog timer.
			 */
			explicit FsmBase(bool silent, time_t tmo = 2000);

			/**
			 * @brief Copy an FsmBase object.
			 * @param other Object to copy.
			 */
			FsmBase(const FsmBase &other);

			/**
			 * @brief Move construct an FSM base object.
			 * @param other Object to move.
			 */
			FsmBase(FsmBase &&other) noexcept;

			/**
			 * @brief Destroy an FSM base object.
			 */
			virtual ~FsmBase();

			/**
			 * @brief Move assignment operator implementation.
			 *
			 * @param other FSM object to move.
			 * @see move
			 * @return Current object.
			 */
			FsmBase &operator=(FsmBase &&other) noexcept;

			/**
			 * @brief Copy assignment operator implementation.
			 *
			 * @param other FSM to copy.
			 * @return Current object.
			 */
			FsmBase &operator=(const FsmBase &other);

			/**
			 * @brief Swap two FSM's.
			 *
			 * @param a First FSM.
			 * @param b Second FSM.
			 * @see lwiot::stl::swap
			 */
			friend void swap(FsmBase &a, FsmBase &b)
			{
				using lwiot::stl::swap;

				if(&a < &b) {
					a.m_lock.lock();
					b.m_lock.lock();
				} else {
					b.m_lock.lock();
					a.m_lock.lock();
				}

				swap(a.m_stt, b.m_stt);
				swap(a.m_events, b.m_events);
				swap(a.m_states, b.m_states);
				swap(a.m_stop_states, b.m_stop_states);
				swap(a.m_start_state, b.m_start_state);
				swap(a.m_current, b.m_current);
				swap(a.m_error_state, b.m_error_state);
				swap(a.m_status, b.m_status);
				swap(a.m_in_transition, b.m_in_transition);
				swap(a.m_logger, b.m_logger);
				swap(a.m_silent, b.m_silent);

				if(&a < &b) {
					b.m_lock.unlock();
					a.m_lock.unlock();
				} else {
					a.m_lock.unlock();
					b.m_lock.unlock();
				}
			}

			/**
			 * @name State methods
			 * @{
			 */

			/**
			 * @brief Request the current status.
			 * @return Current FSM status.
			 * @see FsmStatus m_lock
			 * @note This methods acquires m_lock.
			 */
			inline FsmStatus status() const;

			/**
			 * @brief Check if the FSM is currently running.
			 * @return True or false based on whether the FSM is running or not.
			 * @note This methods acquires m_lock.
			 */
			inline bool running() const;

			/**
			 * @brief Get the current State.
			 * @return The current FSM state.
			 * @note The return value of this method is undefined if the FSM is not running.
			 */
			stl::SharedPointer<StateType> current() const;

			/**
			 * @brief Check if the FSM accepts a given \p event.
			 *
			 * @param event Event to check.
			 * @return True or false based on whether the FSM accepts \p event.
			 */
			bool accept(const FsmEventType &event) const;

			/**
			 * @brief Check if the FSM is valid.
			 * @return True or false based on whether the FSM is valid or not.
			 * @see FsmStatus
			 */
			bool valid() const;

			/**
			 * @brief Check if the FSM is deterministic.
			 * @return True or false based on whether the FSM is deterministic or not.
			 */
			bool deterministic() const;
			///@}

			/// @name FSM builder methods
			/// @{

			/**
			 * @brief Add a new transition.
			 * @tparam T Transition arguments.
			 * @param state State to add a transition to.
			 * @param args Arguments to be forwarded to the transition constructor.
			 * @return A success indicator.
			 */
			template <typename... T>
			bool addTransition(FsmStateIdType state, T&&... args);

			/**
			 * @brief Add a new transition to a state.
			 * @param state State to add a transition to.
			 * @param transition Transition to add to \p state.
			 * @return A success indicator.
			 */
			bool addTransition(FsmStateIdType state, TransitionType&& transition);

			/**
			 * @brief Add a new state.
			 * @param state State to add.
			 * @return A stl::Pair containing the assigned ID and a success indicator.
			 * @note The first stl::Pair value is undefined if the second value of the return stl::Pair is false.
			 */
			stl::Pair<FsmStateIdType, bool> addState(StateType &&state);

			/**
			 * @brief Add a new state.
			 * @param state State to add.
			 * @return A stl::Pair containing the assigned ID and a success indicator.
			 * @note The first stl::Pair value is undefined if the second value of the return stl::Pair is false.
			 */
			stl::Pair<FsmStateIdType, bool> addState(const StateType &state);

			/**
			 * @brief Add a range of states.
			 * @param states Dynamic array of states to add to the FSM.
			 * @return True or false based on whether \p states has been added to the FSM.
			 */
			bool addStates(ArrayListType<StateType> &states);

			/**
			 * @brief Define the start state.
			 * @param id Start state ID.
			 */
			void setStartState(const FsmStateIdType &id);

			/**
			 * @brief Add a stop state.
			 * @param state State ID to add to the set of stop states.
			 * @return True or false based on whether or not \p state has been added to the stop state set.
			 */
			bool addStopState(const FsmStateIdType &state);

			/**
			 * @brief Add a range of stop states.
			 * @param states Array of state IDs to add to the stop state set.
			 * @return True or false based on whether or not \p states have been added to the stop state set.
			 */
			bool addStopStates(const ArrayListType<FsmStateIdType> &states);

			/**
			 * @brief Define the error state.
			 * @param id Error state ID.
			 * @return True or false based on whether or not \p id has been defined as the error state.
			 */
			bool setErrorState(const FsmStateIdType &id);

			/**
			 * @brief Add a new symbol to the alphabet.
			 * @param event Event symbol to add to the alphabet.
			 * @see transition raise
			 * @see m_alphabet
			 * @return True or false based on whether or not \p event has been added to the alphabet.
			 */
			bool addAlphabetSymbol(const FsmEventType& event);

			/**
			 * @brief Add a new symbol to the alphabet using move semantics.
			 * @param event Event symbol to add to the alphabet.
			 * @see transition raise
			 * @see m_alphabet
			 * @return True or false based on whether or not \p event has been added to the alphabet.
			 */
			bool addAlphabetSymbol(FsmEventType&& event);
			/// @}

			/// @name FSM operation methods
			/// @{
			/**
			 * @brief Force stop the FSM.
			 * @note This method will not wait for the FSM to reach a stop state.
			 */
			virtual void halt();

			/**
			 * @brief Execute the FSM.
			 */
			virtual void run() = 0;

			/**
			 * @brief Transition from one state to another.
			 * @param event Input event, which must be in the alphabet.
			 * @param args Arguments to pass to the state and transition handlers.
			 * @return True or false based on whether or not the FSM accepts \p event.
			 * @note This method should be used by state handlers to invoke a transition to the next state. External
			 *       entity's should use FsmBase::raise to influence the FSM.
			 *
			 * This method invokes a transition. Only a single transition can be queued at any given time, attempting
			 * to queue a second transition will result in this method returning \p false.
			 */
			virtual bool transition(FsmEventType &&event, Args &&... args);

			/**
			 * @brief Raise an FSM event in order to transition to another state.
			 * @param event Input event, which must be in the alphabet.
			 * @param args Arguments to pass to the state and transition handlers.
			 * @return True or false based on whether or not the FSM accepts \p event.
			 * @note This method raises an external event. State handlers should use FsmBase::transition in order to invoke
			 *       a transition.
			 */
			virtual bool raise(FsmEventType &&event, Args &&... args);

			/// @}

		protected:
			QueueType<stl::Pair<FsmEventType, ArgumentWrapper>> m_events; //!< Queue of requested transitions.
			mutable LockType m_lock; //!< FSM lock.
			mutable Logger m_logger; //!< Logging stream.

			static constexpr int Timeout = 200;

			/* METHODS */

			/**
			 * @brief Move \p other into \p this.
			 * @param other Object to move.
			 */
			virtual void move(FsmBase &other);

			/**
			 * @brief Copy an FsmBase object.
			 * @param other FsmBase object to copy.
			 */
			virtual void copy(const FsmBase &other);

			/**
			 * @brief Create an ArgumentWrapper object using move semantics..
			 * @param args Arguments to wrap.
			 * @return ArgumentWrapper based on the value of \p args.
			 * @see stl::Tuple
			 */
			static constexpr auto MakeArgumentWrapper(Args &&... args)
			{
				return stl::MakeTuple(stl::forward<Args>(args)...);
			}

			/// @name Transition function handlers
			/// @{

			/**
			 * @brief Perform a transition.
			 *
			 * @tparam Indices Argument indices.
			 * @param event Alphabet symbol.
			 * @param args Transition arguments.
			 * @return The current FSM state.
			 * @see State
			 * @see Transition
			 * @see FsmEventType
			 * @see m_states
			 * @note This method will acquire and release @ref m_lock.
			 *
			 * Transition from the current state to the next state based on \p event. The arguments wrapped
			 * by \p args will be passed to the state handlers.
			 */
			template<size_t... Indices>
			FsmStatus transition(FsmEventType &&event, ArgumentWrapper &&args, IndexSequence<Indices...>) noexcept;

			/**
			 * @brief Attempt to transition to the next state.
			 * @return Current FSM state.
			 */
			FsmStatus transition() noexcept;

			///@}

			/// @name Operational methods
			/// @{

			/**
			 * @brief Start the FSM.
			 * @param check Value to indicate whether the FSM should be checked for validity before starting.
			 */
			void start(bool check);

			/**
			 * @brief Stop the FSM.
			 * @param recurse Value to indicate whether the FSM should wait for the FSM to reach a stop state.
			 * @return True or false based on whether the FSM has stopped succesfully or not.
			 */
			bool stop(bool recurse);

			/// @}

		private:
			FsmBase::MapType<typename SttIndexType::IntegerType, TransitionType> m_stt; //!< State transition table.
			FsmBase::MapType<FsmStateIdType, stl::SharedPointer<StateType>> m_states; //!< FSM state map.
			FsmBase::ArrayListType<stl::ReferenceWrapper<StateType>> m_stop_states; //!< Stop state set.
			stl::ReferenceWrapper<StateType> m_start_state; //!< Start state reference.
			FsmStateIdType m_current; //!< Current state ID.
			stl::ReferenceWrapper<StateType> m_error_state; //!< Error state reference.
			FsmStatus m_status; //!< FSM status.
			EventType m_stop_event; //!< Stop event. Signalled every time a stop state is executed.
			bool m_in_transition; //!< Transition indicator. Set when a state invokes a transition. Cleared after execution.
			SetType<FsmEventType> m_alphabet; //!< DFA alphabet definition.
			bool m_silent; //!< Flag which indicates if message output is enabled or not.

			/* METHODS */

			/**
			 * @brief Add an event to the event queue.
			 * @param event Event to add.
			 * @param args State handler arguments.
			 * @param front Indicator whether or not the event is added to the front of the queue.
			 *
			 * Add a new event to the event queue. FsmBase::transition will add to the front of the queue, while
			 * FsmBase::raise will add to the back of the queue.
			 */
			void addEvent(FsmEventType &&event, ArgumentWrapper &&args, bool front = true);

			/**
			 * @brief Check whether a state is a stop state or not.
			 * @param state State to check.
			 * @return True or false based on whether \p state is a stop state or not.
			 */
			bool isStopState(const FsmStateIdType &state) const;

			/**
			 * @brief Transition to the error state.
			 * @param args Arguments to pass to the state handler.
			 */
			void toErrorState(Args &&... args);

			/**
			 * @brief Update the DFA alphabet based on a new transition.
			 * @param transition Transition to add to the alphabet.
			 */
			void updateAlphabet(const TransitionType& transition);

			/**
			 * @brief Check if a state accepts a given event (alphabet symbol).
			 * @param state State to check.
			 * @param event Event to check.
			 * @note This member function assumes that \p event \f$ \in \f$ FsmBase::m_alphabet.
			 * @return A pair, containing the transition and a boolean indicating whether or
			 *         not \p state has a transition for \p event.
			 */
			stl::Pair<stl::ReferenceWrapper<const TransitionType>, bool> lookup(const StatePointer& state, FsmEventType event) const;
		};

		template<typename P, typename W, typename R, typename... Args>
		FsmBase<R(Args...), P, W>::FsmBase(time_t tmo) : Base(tmo), m_lock(true), m_logger("fsm", stdout),
		                                                 m_current(), m_status(FsmStatus::Stopped), m_in_transition(false),
		                                                 m_silent(false)
		{
            this->m_logger.setVisibility(Logger::Visibility::Info);
            this->m_logger.setStreamVisibility(Logger::Visibility::Info);
		}

		template<typename P, typename W, typename R, typename... Args>
		FsmBase<R(Args...), P, W>::FsmBase(bool silent, time_t tmo) : Base(tmo), m_lock(true), m_logger("fsm", stdout),
		                                                              m_current(), m_status(FsmStatus::Stopped), m_in_transition(false),
		                                                              m_silent(silent)
		{
			this->m_logger.setVisibility(Logger::Visibility::Info);
			this->m_logger.setStreamVisibility(Logger::Visibility::Info);
		}

		template<typename P, typename W, typename R, typename... Args>
		FsmBase<R(Args...), P, W>::FsmBase(const FsmBase &other)  : Base(), m_lock(true),
		                                                            m_current(), m_status(FsmStatus::Stopped),
		                                                            m_in_transition(false), m_silent(false)
		{
			FsmBase::copy(other);
		}

		template<typename P, typename W, typename R, typename... Args>
		FsmBase<R(Args...), P, W>::FsmBase(FsmBase &&other) noexcept : Base(), m_lock(true), m_current(),
		                                                               m_status(FsmStatus::Stopped), m_in_transition(false),
		                                                               m_silent(false)
		{
			FsmBase::move(other);
		}

		template<typename P, typename W, typename R, typename... Args>
		FsmBase<R(Args...), P, W>::~FsmBase()
		{
			UniqueLock<LockType>  lock(this->m_lock);

			this->m_status = FsmStatus::Stopped;
			this->m_events.clear();
			this->m_stop_states.clear();
			this->m_states.clear();
			this->m_stop_event.signal();
			this->m_alphabet.clear();
		}

		template<typename P, typename W, typename R, typename... Args>
		FsmBase<R(Args...), P, W> &FsmBase<R(Args...), P, W>::operator=(const FsmBase &other)
		{
			this->copy(other);
			return *this;
		}

		template<typename P, typename W, typename R, typename... Args>
		FsmBase<R(Args...), P, W> &FsmBase<R(Args...), P, W>::operator=(FsmBase &&other) noexcept
		{
			this->move(other);
			return *this;
		}

		template<typename P, typename W, typename R, typename... Args>
		void FsmBase<R(Args...), P, W>::halt()
		{
			UniqueLock<LockType> lock(this->m_lock);

			if(!this->running())
				return;

			this->m_status = FsmStatus::Stopped;
		}

		template<typename P, typename W, typename R, typename... Args>
		bool FsmBase<R(Args...), P, W>::stop(bool recurse)
		{
			UniqueLock<LockType> lock(this->m_lock);

			if(!this->running())
				return true;

			auto &current = this->m_states.at(this->m_current);

			if(current->id() == this->m_error_state->id()) {
				this->m_status = FsmStatus::Stopped;
				return true;
			}

			for(auto &state : this->m_stop_states) {
				if(state->id() == current->id()) {
					this->m_status = FsmStatus::Stopped;
					return true;
				}
			}

			if(!recurse)
				return false;

			this->m_stop_event.wait(lock);
			return stop(false);
		}

		template<typename P, typename W, typename R, typename... Args>
		void FsmBase<R(Args...), P, W>::start(bool check)
		{
			UniqueLock<LockType> lock(this->m_lock);

			if(check && !this->valid())
				return;

			this->m_current = this->m_start_state->id();
			this->m_status = FsmStatus::Running;
		}

		template<typename P, typename W, typename R, typename... Args>
		inline bool FsmBase<R(Args...), P, W>::running() const
		{
			UniqueLock<LockType> lock(this->m_lock);
			return this->m_status == FsmStatus::Running;
		}

		template<typename P, typename W, typename R, typename... Args>
		bool FsmBase<R(Args...), P, W>::accept(const FsmEventType &event) const
		{
			UniqueLock<LockType> lock(this->m_lock);

			if(!this->running())
				return false;

			auto state = this->current();

			do {
				SttIndexType index {0ULL};

				index.value.eventId = event;
				index.value.stateId = state->id();

				auto transition = this->m_stt.find(index);

				if(transition != this->m_stt.end())
					return true;

				if(!state->parent())
					break;

				state = this->m_states.at(state->parent());
			} while(state);


			return false;
		}

		template<typename P, typename W, typename R, typename... Args>
		bool FsmBase<R(Args...), P, W>::valid() const
		{
			UniqueLock<LockType> lock(this->m_lock);

			if(!(this->m_status == FsmStatus::Running || this->m_status == FsmStatus::Stopped))
				return false;

			if(this->m_states.size() == 0)
				return false;

			if(!this->m_start_state || this->m_stop_states.size() == 0)
				return false;

			if(!this->m_error_state)
				return false;

			return this->deterministic();
		}

		template<typename P, typename W, typename R, typename... Args>
		bool FsmBase<R(Args...), P, W>::deterministic() const
		{
			for(const StatePointer& state : this->m_states) {
				/* We need to store accepted events in order to check for epsilon transitions */
				SetType<FsmEventType> accepts;

				for(const auto& symbol: this->m_alphabet) {
					auto result = this->lookup(state, symbol);

					if(!result.second && state->hasAction()) {
						this->m_logger << "FSM is missing a transition for [State ID: " << state->id() <<
						               " | Event ID: " << symbol << "]" << Logger::newline;
						return false;
					}

					auto value = accepts.insert(symbol);

					if(!value.second) {
						this->m_logger << "FSM contains an epsilon transition: [state ID: " << state->id() <<
						               " | Event ID: " << symbol << "]" << Logger::newline;
						return false;
					}
				}
			}

			return true;
		}

		template<typename P, typename W, typename R, typename... Args>
		stl::SharedPointer<State<P, Function<R(Args...)>, R, Args...>> FsmBase<R(Args...), P, W>::current() const
		{
			UniqueLock<LockType> lock(this->m_lock);

			if(!this->running())
				return stl::MakeShared<StateType>();

			return this->m_states.at(this->m_current);
		}

		template<typename P, typename W, typename R, typename... Args>
		FsmStatus FsmBase<R(Args...), P, W>::status() const
		{
			UniqueLock<LockType> lock(this->m_lock);
			return this->m_status;
		}

		template<typename P, typename W, typename R, typename... Args>
		bool FsmBase<R(Args...), P, W>::addAlphabetSymbol(const FsmEventType &event)
		{
			auto rv = this->m_alphabet.insert(event);
			return rv.second;
		}

		template<typename P, typename W, typename R, typename... Args>
		bool FsmBase<R(Args...), P, W>::addAlphabetSymbol(FsmEventType &&event)
		{
			auto rv = this->m_alphabet.emplace(stl::forward<FsmEventType>(event));
			return rv.second;
		}


		template<typename P, typename W, typename R, typename... Args>
		template <typename... T>
		bool FsmBase<R(Args...), P, W>::addTransition(FsmStateIdType state, T &&... args)
		{
			TransitionType transition(stl::forward<T>(args)...);
			SttIndexType index { 0ULL };

			index.value.stateId = state;
			index.value.eventId = transition.event();

			this->updateAlphabet(transition);

			auto rv = this->m_stt.emplace(stl::move(index.integer), stl::move(transition));
			return rv.second;
		}

		template<typename P, typename W, typename R, typename... Args>
		bool FsmBase<R(Args...), P, W>::addTransition(FsmStateIdType state, TransitionType&& transition)
		{
			SttIndexType index {0ULL};

			index.value.eventId = transition.event();
			index.value.stateId = state;

			this->updateAlphabet(transition);

			auto rv = this->m_stt.emplace(stl::move(index.integer), stl::forward<TransitionType>(transition));
			return rv.second;
		}

		template<typename P, typename W, typename R, typename... Args>
		stl::Pair<FsmStateId, bool> FsmBase<R(Args...), P, W>::addState(StateType &&state)
		{
			UniqueLock<LockType> lock(this->m_lock);
			auto id = state.id();

			if(this->m_states.contains(id))
				return stl::Pair<FsmStateIdType, bool>(id, false);

			this->m_logger << "Adding state " << state.id() << Logger::newline;

			auto ptr = stl::MakeShared<StateType>(stl::forward<StateType>(state));
			auto pair = stl::Pair<FsmStateIdType, bool>(id, false);
			auto rv = this->m_states.emplace(stl::move(id), stl::move(ptr));
			pair.second = rv.second;

			return pair;
		}

		template<typename P, typename W, typename R, typename... Args>
		stl::Pair<FsmStateId, bool> FsmBase<R(Args...), P, W>::addState(const StateType &state)
		{
			UniqueLock<LockType> lock(this->m_lock);
			auto id = state.id();

			if(this->m_states.contains(id))
				return stl::Pair<FsmStateIdType, bool>(id, false);

			this->m_logger << "Adding state " << state.id() << Logger::newline;

			auto ptr = stl::MakeShared<StateType>(state);
			auto pair = stl::Pair<FsmStateIdType, bool>(id, false);
			auto rv = this->m_states.emplace(stl::move(id), stl::move(ptr));
			pair.second = rv.second;

			return stl::move(pair);
		}

		template<typename P, typename W, typename R, typename... Args>
		bool FsmBase<R(Args...), P, W>::addStates(ArrayListType<StateType> &states)
		{
			UniqueLock<LockType> lock(this->m_lock);

			for(auto &state : states) {
				auto id = state.id();

				if(this->m_states.contains(id))
					return false;

				auto ptr = stl::MakeShared<StateType>(stl::forward<StateType>(state));
				auto rv = this->m_states.emplace(stl::move(id), stl::move(ptr));
				this->m_logger << "Adding state " << state.id() << Logger::newline;

				if(!rv.second)
					return false;
			}

			return true;
		}

		template<typename P, typename W, typename R, typename... Args>
		void FsmBase<R(Args...), P, W>::setStartState(const FsmStateIdType &id)
		{
			UniqueLock<LockType> lock(this->m_lock);

			if(!this->m_states.contains(id))
				return;

			auto ptr = this->m_states.at(id);
			this->m_start_state = *ptr;
		}

		template<typename P, typename W, typename R, typename... Args>
		bool FsmBase<R(Args...), P, W>::setErrorState(const FsmStateIdType &id)
		{
			UniqueLock<LockType> lock(this->m_lock);

			if(!this->m_states.contains(id))
				return false;

			this->m_error_state = *this->m_states.at(id);
			return true;
		}

		template<typename P, typename W, typename R, typename... Args>
		bool FsmBase<R(Args...), P, W>::addStopState(const FsmStateIdType &state)
		{
			UniqueLock<LockType> lock(this->m_lock);

			if(!this->m_states.contains(state))
				return false;

			auto ptr = this->m_states.at(state);
			stl::ReferenceWrapper<StateType> ref(*ptr);
			this->m_stop_states.push_back(stl::move(ref));

			return true;
		}

		template<typename P, typename W, typename R, typename... Args>
		bool FsmBase<R(Args...), P, W>::addStopStates(const ArrayListType<FsmStateIdType> &states)
		{
			UniqueLock<LockType> lock(this->m_lock);

			/*
			 * Do not merge the loops below. It is critical that either all states are
			 * added succesfully or none at all.
			 */
			for(auto &state: states) {
				if(!this->m_states.contains(state))
					return false;
			}

			for(auto &state: states) {
				auto ptr = this->m_states.at(state);
				stl::ReferenceWrapper<StateType> ref(*ptr);
				this->m_stop_states.push_back(stl::move(ref));
			}

			return true;
		}

		template<typename P, typename W, typename R, typename... Args>
		bool FsmBase<R(Args...), P, W>::raise(FsmEventType &&event, Args &&... args)
		{
			UniqueLock<LockType> lock(this->m_lock);

			if(!this->accept(event))
				return false;

			auto wrapper = MakeArgumentWrapper(stl::forward<Args>(args)...);
			this->addEvent(stl::forward<FsmEventType>(event), stl::move(wrapper), false);

			return true;
		}

		template<typename P, typename W, typename R, typename... Args>
		bool FsmBase<R(Args...), P, W>::transition(FsmEventType &&event, Args &&... args)
		{
			UniqueLock<LockType> lock(this->m_lock);

			if(!this->accept(event) || this->m_in_transition)
				return false;

			auto wrapper = MakeArgumentWrapper(stl::forward<Args>(args)...);
			this->addEvent(stl::forward<FsmEventType>(event), stl::move(wrapper));
			this->m_in_transition = true;

			return true;
		}

		template<typename P, typename W, typename R, typename... Args>
		template<size_t... Indices>
		FsmStatus FsmBase<R(Args...), P, W>::transition(FsmEventType &&event, ArgumentWrapper &&args,
		                                                IndexSequence<Indices...>) noexcept
		{
			UniqueLock<LockType> lock(this->m_lock);
			stl::ReferenceWrapper<const TransitionType> transition;
			stl::SharedPointer<StateType> state;
			bool success;

			this->m_watchdog.reset();

			if(!this->running())
				return this->m_status;

			/*
			 * Find the transition for the given input symbol. Past this point we
			 * only need to verify that an actual transition was found, verifying
			 * the actual transition itself or the state it points is not necessary
			 * because:
			 *
			 * a) A verified running FSM is deterministic;
			 * b) We verified that the current state accepts the given input
			 *    symbol atleast twice.
			 */
			state = this->m_states.at(this->m_current);
			auto result = this->lookup(state, event);
			transition = stl::move(result.first);

			auto next = transition->next();
			this->m_current = next;
			state = this->m_states[next];

			ArgumentWrapper copy(args);

			if(state->hasAction())
				success = state->action(stl::get<Indices>(stl::move(args))...);
			else
				success = false;

			if(unlikely(!success)) {
				this->toErrorState(stl::get<Indices>(stl::move(copy))...);
				this->m_stop_event.signal();

				this->m_logger << "Unable to succesfully execute FSM state!" << Logger::newline;

				return FsmStatus::Fault;
			}

			if(this->isStopState(next))
				this->m_stop_event.signal();

			return FsmStatus::StateChanged;
		}

		template<typename P, typename W, typename R, typename... Args>
		FsmStatus FsmBase<R(Args...), P, W>::transition() noexcept
		{
			UniqueLock<LockType> lock(this->m_lock);

			if(this->m_events.size() == 0UL)
				return FsmStatus::StateUnchanged;

			auto value = stl::move(*this->m_events.begin());
			this->m_events.erase(this->m_events.begin());
			auto rv = this->transition(stl::move(value.first), stl::move(value.second),
			                           typename MakeIndexSequence<sizeof...(Args)>::Type());
			this->m_in_transition = false;

			return rv;
		}

		template<typename P, typename W, typename R, typename... Args>
		stl::Pair<stl::ReferenceWrapper<const Transition<P, Args...>>, bool>
		FsmBase<R(Args...), P, W>::lookup(const StatePointer &state, FsmEventType event) const
		{
			stl::Pair<stl::ReferenceWrapper<const TransitionType>, bool> rv;
			SttIndexType index = { 0ULL };

			index.value.eventId = event;
			index.value.stateId = state->id();

			auto result = this->m_stt.find(index);
			rv.second = result != this->m_stt.end();

			if(unlikely(!rv.second)) {
				auto parent = state->parent();

				if(!state->hasParent())
					return rv;

				return this->lookup(this->m_states.at(parent), event);
			}

			rv.first = stl::MakeCRef(*result);
			return rv;
		}

		template<typename P, typename W, typename R, typename... Args>
		bool FsmBase<R(Args...), P, W>::isStopState(const FsmStateIdType &state) const
		{
			for(const stl::ReferenceWrapper<StateType> &ref : this->m_stop_states) {
				if(ref->id() == state)
					return true;
			}

			return false;
		}

		template<typename P, typename W, typename R, typename... Args>
		void FsmBase<R(Args...), P, W>::addEvent(FsmEventType &&event, ArgumentWrapper &&args, bool front)
		{
			stl::Pair<FsmEventType, ArgumentWrapper> value(stl::forward<FsmEventType>(event), stl::move(args));

			if(front)
				this->m_events.push_front(stl::move(value));
			else
				this->m_events.push_back(stl::move(value));
		}

		template<typename P, typename W, typename R, typename... Args>
		void FsmBase<R(Args...), P, W>::toErrorState(Args &&... args)
		{
			this->m_current = this->m_error_state->id();
			this->m_status = FsmStatus::Error;
			this->m_error_state->action(stl::forward<Args>(args)...);
		}

		template<typename P, typename W, typename R, typename... Args>
		void FsmBase<R(Args...), P, W>::copy(const FsmBase &other)
		{
			UniqueLock<LockType> l1(this->m_lock);
			UniqueLock<LockType> l2(other.m_lock);

			this->m_stt = other.m_stt;
			this->m_events = other.m_events;
			this->m_states = other.m_states;
			this->m_stop_states = other.m_stop_states;
			this->m_start_state = other.m_start_state;
			this->m_current = other.m_current;
			this->m_error_state = other.m_error_state;
			this->m_status = other.m_status;
			this->m_in_transition = other.m_in_transition;
			this->m_logger = other.m_logger;
			this->m_silent = other.m_silent;
		}

		template<typename P, typename W, typename R, typename... Args>
		void FsmBase<R(Args...), P, W>::move(FsmBase &other)
		{
			using stl::swap;
			swap(*this, other);
		}

		template<typename P, typename W, typename R, typename... Args>
		void FsmBase<R(Args...), P, W>::updateAlphabet(const TransitionType& transition)
		{
			if(this->m_alphabet.contains(transition.event()))
				return;

			this->m_alphabet.insert(transition.event());
		}
	}

/**
 * @brief Event signal type definition.
 * @ingroup fsm
 *
 * Default FSM event handler argument type.
 */
class Signal {
	public:
		/**
		 * @brief Construct a new Signal type.
		 * @param now Current timestamp.
		 */
		explicit Signal(time_t now = lwiot_tick_ms()) : m_moment(now)
		{ }

		/**
		 * @brief Timestamp getter.
		 * @return The timestamp at which the signal was created.
		 */
		time_t time() const
		{
			return this->m_moment;
		}

	private:
		time_t m_moment;
	};

	/**
	 * @brief Type cast a base signal to a derived class.
	 * @tparam T Requested signal type.
	 * @param value Base signal pointer.
	 * @return Shared pointer of type \p T.
	 * @ingroup fsm
	 */
	template <typename T>
	inline stl::SharedPointer<T> SignalAs(stl::SharedPointer<Signal> value)
	{
		return stl::static_pointer_cast<T>(value);
	}
}
