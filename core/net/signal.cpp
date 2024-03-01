#include "signal.h"

#include "formatter.h"

// super-simple type definitions
struct vec3 { float x, y, z; };
struct vec4 { float x, y, z, w; };
using quat = vec4;
struct transform { vec3 position; quat orientation; };

namespace net
{
	// stores if music is on or off
	class MusicSignal : public TypedSignal<bool, bool>
	{
	public:
		std::string to_string() const override { return fmt::format("isMusicOn={}", State()); }
	};

	class HmdSignal : public TypedSignal<transform, transform>
	{
	public:
		std::string to_string() const override { return fmt::format("xform={}", State()); }
	};


	void InitializeEnvSignals(std::vector<std::unique_ptr<Signal>>& signals)
	{
		signals.emplace_back(new MusicSignal());
		signals.back()->SetConfig({"isMusicOn", true});
	}

	// Normally, load from file
	void InitializeClientSignals(std::vector<std::unique_ptr<Signal>>& signals)
	{
		signals.emplace_back(new HmdSignal());
		signals.back()->SetConfig({ "hmd", false });
	}
}