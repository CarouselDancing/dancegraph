#include <sig/signal_common.h>
#include <sig/signal_consumer.h>

using namespace sig;

int main(int argc, char** argv) {
	
	auto lib1 = SignalLibraryConsumer("C:\\Users\\esrev\\Documents\\repos\\dancegraph-native\\build\\bin\\multi-dll-lib.dll");
	auto lib2 = SignalLibraryConsumer("C:\\Users\\esrev\\Documents\\repos\\dancegraph-native\\build\\bin\\multi-dll-lib.dll");

	lib1.fnSignalConsumerInitialize({}, {});
	lib2.fnSignalConsumerInitialize({}, {});

	SignalMetadata s;

	for(int i=0;i<4; ++i)
		lib1.fnProcessSignalData(nullptr, 0, s);
	for (int i = 0; i < 4; ++i)
		lib2.fnProcessSignalData(nullptr, 0, s);
	return 0;
}
