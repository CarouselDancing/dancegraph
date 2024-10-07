
#include <pubsub/pubsub.h>

#include <spdlog/spdlog.h>

using namespace pubsub;

void dumpmsg(std::string sender, std::string signal, std::set<Subscriber<std::string>> & recipients) {
	std::string s = fmt::format("Signal type {} from {}:", signal, sender);
	for (auto & rec : recipients) {
		s += fmt::format(" {}", rec.id);
	}
	spdlog::info(s);
}

int main() {

	SubscriptionManager<std::string> subman;

	

	std::vector<std::string> siglist = std::vector<std::string>{ "inky", "blinky", "pinky", "clyde" };

	SubRequest<std::string> ReqNoPinky = SubRequest<std::string>{
			Op::Sub,
			Quant::All,
			std::vector<std::string>{},
			Quant::List,
			std::vector<std::string>{"inky", "blinky", "clyde"}
	};


	SubRequest<std::string> ReqMarioOnly = SubRequest<std::string>{
			Op::Sub,
			Quant::List,
			std::vector<std::string>{"mario"},
			Quant::All,
			std::vector<std::string>{}
	};

	SubRequest<std::string> ReqQbertPinky = SubRequest<std::string>{
			Op::Sub,
			Quant::List,
			std::vector<std::string>{"qbert"},
			Quant::List,
			std::vector<std::string>{"pinky"}
	};



	subman.Initialize(siglist);

	subman.Subscribe(std::string("lara"), SubType::Listener, ReqSubEverything<std::string>);

	subman.Subscribe(std::string("pacman"), SubType::Client, ReqSubEverything<std::string>);

	subman.Subscribe(std::string("mario"), SubType::Client, ReqNoPinky);

	subman.Subscribe(std::string("qbert"), SubType::Client, ReqSubEverything<std::string>);

	subman.Subscribe(std::string("luigi"), SubType::Client, ReqMarioOnly);

	subman.Subscribe(std::string("willy"), SubType::Listener, ReqQbertPinky);

	subman.Subscribe(std::string("megaman"), SubType::Listener, ReqSubEverything<std::string>);

	subman.DumpTable();
	spdlog::info(  "\n--");


	for (auto& sig : siglist) {
		std::set<Subscriber<std::string>> subset = subman.GetRecipients(std::string("pacman"), sig);
		dumpmsg("pacman", sig, subset);
	}

	for (auto& sig : siglist) {
		std::set<Subscriber<std::string>> subset = subman.GetRecipients(std::string("qbert"), sig);
		dumpmsg("qbert", sig, subset);
	}


	for (auto& sig : siglist) {
		std::set<Subscriber<std::string>> subset = subman.GetRecipients(std::string("mario"), sig);
		dumpmsg("mario", sig, subset);
	}


	for (auto& sig : siglist) {
		std::set<Subscriber<std::string>> subset = subman.GetRecipients(std::string("luigi"), sig);
		dumpmsg("luigi", sig, subset);
	}

	spdlog::info(  "Unsubscribing luigi");
	subman.Unsubscribe(std::string("luigi"), SubType::Client);

	// TODO: Removing Subscribers and altering subscriptions


	for (auto& sig : siglist) {
		std::set<Subscriber<std::string>> subset = subman.GetRecipients(std::string("pacman"), sig);
		dumpmsg("pacman", sig, subset);
	}

	for (auto& sig : siglist) {
		std::set<Subscriber<std::string>> subset = subman.GetRecipients(std::string("qbert"), sig);
		dumpmsg("qbert", sig, subset);
	}


	for (auto& sig : siglist) {
		std::set<Subscriber<std::string>> subset = subman.GetRecipients(std::string("mario"), sig);
		dumpmsg("mario", sig, subset);
	}

	exit(0);
}