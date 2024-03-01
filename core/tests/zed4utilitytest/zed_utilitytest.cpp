
#include <spdlog/spdlog.h>

#include <argparse.hpp>

#include "../../modules/zed/zed_common.h"

using namespace zed;

int main(int argc, char** argv) {

	argparse::ArgumentParser program("zed_utilitytest");

	program.add_argument("vec1").help("vector (compact quaternion) 1")
		.nargs(3)
		.required()
		.scan<'g', float>();

	program.add_argument("vec2").help("vector (compact quaternion) 2")
		.nargs(3)
		.required()
		.scan<'g', float>();

	program.add_argument("tt").help("interpolation factor")
		.required()
		.scan<'g', float>();


	try {
		program.parse_args(argc, argv);
	}
	catch (const std::runtime_error& err) {

		std::cerr << "Argument parsing error: " << err.what() << std::endl;
		std::exit(1);
	}

	// Vec3 addition, subtraction, scalar multiples, dot product, lerp
	// Quaternion addition, subtraction, scalar multiples, dot product, slerp


	for (int i = 0; i < 38; i++) {
		std::cout << body_parts_38[i] << ": ";
		for (auto& j : body_38_tree[i]) {
			std::cout << body_parts_38[(int)j] << " ";
		}
		std::cout << std::endl;
	}



	auto v1val = program.get<std::vector<float>>("vec1");
	auto v2val = program.get<std::vector<float>>("vec2");
	float tval = program.get<float>("tt");
	
	vec3 v1 = vec3{ v1val[0], v1val[1], v1val[2] };
	vec3 v2 = vec3{ v2val[0], v2val[1], v2val[2] };
	spdlog::info("-- * Vectors");
	spdlog::info("Input as vectors: [{}], [{}], {}", v1.str(), v2.str(), tval);

	float d1 = dot(v1, v1);
	float d2 = dot(v2, v2);

	spdlog::info("Dot products: v1.v1 = {}, v2.v2 = {}, v1.v2={}", d1, d2, dot(v1, v2));

	spdlog::info("Sum: [{}], Diff: [{}], Scalar Mul: [{}], [{}]",(v1 + v2).str(), (v1 - v2).str(), (tval * v1).str(), (tval * v2).str());
	spdlog::info("Lerp: [{}]", lerp(v1, v2, tval).str());

	if ((d1 > 1.0) || (d2 > 1.0)) {
		spdlog::error("Impossible to normalize the 3-vectors as quaternions, exiting");
		exit(0);
	}
	float w1 = sqrt(1.0 - d1);
	float w2 = sqrt(1.0 - d2);

	quat q1 = quat{ v1val[0], v1val[1], v1val[2], w1 };
	quat q2 = quat{ v2val[0], v2val[1], v2val[2], w2 };
	spdlog::info("-- * Unquantized quaternions");
	spdlog::info("Unit quaternions: [{}], [{}]", q1.str(), q2.str());
	spdlog::info("Dot products: q1.q1 = {}, q2.q2 = {}, q1.q2={}", dot(q1, q1), dot(q2, q2), dot(q1, q2));

	spdlog::info("Sum: [{}], Diff: [{}], Scalar Mul: [{}], [{}]", (q1 + q2).str(), (q1 - q2).str(), (tval * q1).str(), (tval * q2).str());
	spdlog::info("Slerp: [{}]", slerp(q1, q2, tval).str());

	spdlog::info("-- * Quantized quaternions");
	quant_quat qq1 = q1.quantize();
	quant_quat qq2 = q2.quantize();

	spdlog::info("Unit quaternions: [{}], [{}]", qq1.str(), qq2.str());
	//spdlog::info("Dot products: q1.q1 = {}, q2.q2 = {}, q1.q2={}", dot(qq1, qq1), dot(qq2, qq2), dot(qq1, qq2));

	spdlog::info("Sum: [{}], Diff: [{}]", (qq1 + qq2).str(), (qq1 - qq2).str());
	spdlog::info("Slerp: [{}]", slerp(qq1, qq2, tval).str());

	// Quaternion multiplication

	spdlog::info("{} * {} = {}", q1.str(), q2.str(), (q1 * q2).str());
	spdlog::info("{} * {} = {}", q2.str(), q1.str(), (q2 * q1).str());

	// Quaternion mult by vec

	
	spdlog::info("[{}] * [{}] = [{}]", q1.str(), v2.str(), (q1 * v2).str());
	spdlog::info("[{}] * [{}] = [{}]", q2.str(), v1.str(), (q2 * v1).str());





}

	
