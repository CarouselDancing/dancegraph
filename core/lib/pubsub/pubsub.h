#pragma once

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <iostream>
#include <set>

#include <spdlog/spdlog.h>

#include <core/common/utility.h>

namespace pubsub {

	enum class SubType {		
		Client = 0 , // The only entities that actually publish messages
		Listener, // A Client that subscribes but doesn't publist
		Compound, // Compound class
		AllClients
	};

	enum class SigType {
		All = 0,
		Signal

	};

	enum class Quant {
		List = 0,
		All
	};

	enum class Op {
		Sub,
		Unsub
	};

	// Request to change
	template
		<typename T>
		struct SubRequest {

		Op change;

		Quant subquantity;
		std::vector<T> subs;

		Quant sigquantity;
		std::vector<std::string> signals;

	};


	template
		<typename T>
		SubRequest<T> ReqSubEverything = SubRequest<T>{
			Op::Sub,
			Quant::All,
			std::vector<T>{},
			Quant::All,
			std::vector<std::string>{}
	};


	// Client ids have to be unique within a type, but two Subscribers have the same id if they're a different type
	// Type T needs to have a default constructor value
	template
		<typename T>
		struct Subscriber {

		// Whatever id has, it has to implement the '<' operator as a bool
		T id;
		SubType type;

		//friend bool operator < (const Subscriber<T>&, const Subscriber<T>&);

	};

	template
		<typename T>
		bool operator == (const Subscriber<T>& r, const Subscriber<T>& s) {

		// We don't care what the id field contains if we want everybody
		if ((r.type == SubType::AllClients) && (s.type == SubType::AllClients))
			return true;

		return ((r.id) == s.id) && ((r.type == s.type));
	}

	template <class T>
	inline void hash_combine(std::size_t& s, const T& v)
	{
		std::hash<T> h;
		s ^= h(v) + 0x9e3779b9 + (s << 6) + (s >> 2);
	}


	// std::map wants this

	// Ordered first by type, then id
	// type == SubType::AllClients is treated as equality
	template
		<typename T>
		bool operator < (const Subscriber<T>& r, const Subscriber<T>& s) {

		if ((r.type == SubType::AllClients) && (s.type == SubType::AllClients))
			return false;

		if ((int)r.type < (int)s.type)
			return true;
		if (r.id < s.id)
			return true;

		return false;
	}

	using Signal = std::string;


	template
		<typename T>
		class Topic {
		public:
			void Add(Subscriber<T> s) {
				
				subscribers.insert(s);
				spdlog::info(  "Pubsub: Adding user {} to topic ({})!", s.id, subscribers.size());
			}

			void Delete(Subscriber<T> s) {
				auto it = subscribers.find(s);
				if (it != subscribers.end())
					subscribers.erase(s);
			}

			std::set<Subscriber<T>> subscribers; // The 'union' function is probably the most important one here.

	};


	// Keytype is a unique type for identifying each subscriber 
	template
		<typename T>
		class SubscriptionManager {
		public:

			void Subscribe(T subscriber, SubType type, SubRequest<T>& req) { // == SubReqDefault

				// If the new subscriber is a client (i.e. a publisher), add them to the list of publishers
				switch (type) {
				case SubType::Client:
					AddToSubscribers(subscriber, type, req);
					client_list.emplace_back(subscriber);
					for (auto sig : signal_list) {
						auto subKey = std::pair{ Subscriber<T>{subscriber, type}, sig };

						topicTable[subKey] = Topic<T>{};

						// And anyone subscribed to "All" clients gets them added in
					}
					spdlog::info(  "Pubsub:Adding Client Subscriber");
					break;
				case SubType::Listener:
					AddToSubscribers(subscriber, type, req);
					spdlog::info(  "Pubsub:Adding Listener Subscriber");
					listener_list.emplace_back(subscriber);
					break;
				case SubType::Compound:default:
					break;
				}

			}
	
			// Returns an unordered list of subscribers to a signal, given the signal's information
			std::set<Subscriber<T>> GetRecipients(T publisher, std::string sig) {
				
				// Subscribed to the Universal Publisher for this signal?

				std::pair<Subscriber<T>, Signal> uniKey = std::pair<Subscriber<T>, Signal>{ UniversalPublisher, sig };
				std::set<Subscriber<T>> * universalset = &topicTable[uniKey].subscribers;
				//spdlog::info(  "Universal Set: has size: {}", universalset->size());


				Subscriber<T> keypub = Subscriber<T>{ publisher, SubType::Client };
				Signal keysig = Signal{ sig };
				
				std::set<Subscriber<T>> clientset = topicTable[std::pair<Subscriber<T>, Signal>{keypub, keysig}].subscribers;

				std::set<Subscriber<T>> a_set = *universalset;
				
				a_set.merge(clientset);

				// We're not sending to ourselves				
				auto it = a_set.find(keypub);
				if (it != a_set.end())
					a_set.erase(it);				
				return a_set;
				
			}


			// Removes the subscriber entirely
			void Unsubscribe(T subscriber, SubType type) {

				Subscriber<T> ksub = Subscriber<T>{ subscriber, type };


				switch (type) {
				case SubType::Client:
					client_list.erase(std::remove(client_list.begin(), client_list.end(), subscriber), client_list.end());
					// Erase any publisher channels that were created
					for (auto& sig : signal_list) {
						auto ttf = topicTable.find(std::pair<Subscriber<T>, std::string>{ksub, sig});
						if (ttf != topicTable.end())
							topicTable.erase(ttf);
					}
					break;
				case SubType::Listener:
					//listener_list.erase(subscriber);
					listener_list.erase(std::remove(listener_list.begin(), listener_list.end(), subscriber), listener_list.end());

					break;
				default:
					break;
				}
				// Erase the subscriber from any subscription lists
				for (auto& [k, v] : topicTable) {
					v.Delete(ksub);
				}
			}


			// Changes the subscription with the given information, without nuking the previous subscription information
			//void UpdateSubscription(T subscriber, SubscriptionInfo<T>& updateinfo);

			// Changes the subscription with the given information, while nuking the previous subscription information
			//void ReplaceSubscription(T subscriber, SubscriptionInfo<T>& updateinfo);


/*			void SubToAllSignals(T subscriber, std::string signal) {
				Subscribe(T, SubscriptionRequest{ Op::Sub, Quant::All, std::vector<T>(), Quant::List, std::vector<T>{signal} });
			}
			*/
			// Initialize with a string-based list of all the supported signals
			void Initialize(std::vector <std::string> & siglist) {
				
				signal_list.insert(signal_list.end(), siglist.begin(), siglist.end());

				// Initialize the type with the undefined 
				UniversalPublisher.type = SubType::AllClients;
				UniversalPubList = std::vector<Subscriber<T>>{ UniversalPublisher };
				// Create a universal publisher for each signal type
				for (auto & sig : signal_list) {
					spdlog::info(  "Pubsub: Adding universal publisher to signal {}", sig);
					topicTable[std::pair<Subscriber<T>, Signal>{UniversalPublisher, sig}] = Topic<T>{};
				}
			}

			void DumpTable() {
				// This is too spammy to be called every tick even for spdlog::trace!
				if(spdlog::get_level() <= spdlog::level::trace)
					for (const auto & [k, t] : topicTable) {

						std::string s = fmt::format("({})", t.subscribers.size());
						if (k.first.type == SubType::AllClients)
							s += "*UNIVERSAL*";
						else
							s += fmt::format("{}",k.first.id);
						s += fmt::format("/{} - ", k.second);
						for (const auto& sub : t.subscribers) {
							if (sub.type == SubType::Client)
								s+= "*";
							s += fmt::format("{} ",sub.id);
						}
						spdlog::trace(s);
					}
			}
		protected:
			
			void AddToSubscribers(T subscriber, SubType type, SubRequest<T> & req) {
				// A temporary subscriber list
				std::vector<Subscriber<T>> tclist;

				std::vector<Subscriber<T>>* trans_client_list;
				std::vector<std::string>* trans_sig_list;

				// The request is either a list of users with all signals
				// Or a list of signals with all users
				// Or all signals with all users
				// Or lists of signals and a list of users

				switch (req.subquantity) {
				case Quant::All:
					trans_client_list = &UniversalPubList;
					spdlog::info(  "Pubsub: Subscriber: {}, Universal Publisher", subscriber);
					break;
				case Quant::List:
					// Clear out the 
					tclist = std::vector<Subscriber<T>>{};
					for (auto& sub : req.subs) {
						tclist.emplace_back(Subscriber<T>{sub, SubType::Client});
					}

					trans_client_list = &tclist;

					spdlog::info(  "Pubsub: Adding a list of clients {}", trans_client_list->size());
					break;
				default: // Unknown Subscription Request
					spdlog::warn(  "Pubsub: Unknown Pub Quantity");
					return;	break;
				}

				switch (req.sigquantity) {
				case Quant::All:
					trans_sig_list = &signal_list;
					break;
				case Quant::List:
					trans_sig_list = &req.signals;
					break;
				default:
					spdlog::warn(  "Pubsub: Unknown Signal Quantifier: {}", (int)req.sigquantity);
					return;	break;
				}

				spdlog::info(  "Pubsub: Client list {}", trans_client_list->size());
				spdlog::info(  "Pubsub: Signal list {}", trans_sig_list->size());

				for (auto pub : *trans_client_list) {

					for (auto sig : *trans_sig_list) {

						auto key = std::pair{ pub, Signal {sig} };
						try {
							if (key.first.type == SubType::AllClients)
								spdlog::trace(  "Pubsub: Test: Subbing {} to "  "Universal"  "/{}", subscriber, sig);
							else
								spdlog::trace(  "Pubsub: Test: Subbing {} to {}/{}", subscriber, pub.id, sig);
							topicTable[key].Add(Subscriber<T>{subscriber, type});
						}
						catch (std::exception e) {
							spdlog::warn(  "Pubsub: sub/sig pair {}/{} subscription error: {}", pub.id, sig, e.what());
						}
					}
				}
			}

			// Dual key - <subscriber, signal> 
			std::unordered_map<std::pair<Subscriber<T>, Signal>, Topic<T> > topicTable;

			std::vector<std::string> signal_list;
			std::vector<T> client_list; // Clients are both Publishers and Subscribers
			std::vector<T> listener_list; // Subscribers Only

			Subscriber<T> UniversalPublisher;
			std::vector<Subscriber<T>> UniversalPubList;
			//const Subscriber<T> allsub;


	};

}

namespace std
{
	template <class T>
	struct hash<pubsub::Subscriber<T>>
	{
		std::size_t operator()(const pubsub::Subscriber<T>& c) const
		{
			std::size_t result = 0;
			pubsub::hash_combine(result, c.id);
			pubsub::hash_combine(result, c.type);
			return result;
		}
	};

	template <class T, class U>
	struct hash<std::pair<T,U>>
	{
		std::size_t operator()(const std::pair<T, U>& c) const
		{
			std::size_t result = 0;
			pubsub::hash_combine(result, c.first);
			pubsub::hash_combine(result, c.second);
			return result;
		}
	};
}