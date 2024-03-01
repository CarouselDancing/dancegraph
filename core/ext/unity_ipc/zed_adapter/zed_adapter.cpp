
#include <memory>
#include <cstdlib>


#include <ipc/ringbuffer.h>
#include <modules/zed/zed_common.h>
#include "sig/signal_common.h"
#include <sig/signal_config.h>

#include <core/net/config_master.h>

#include <chrono>

#include "zed_adapter.h"

namespace dll {


	std::unique_ptr<ZedAdapter> zedAdapterp;
	int8_t* signalData; // A unique pointer to be freed

	// How the signal comes in from IPC

	
	zed::Zed4Header * zedHeaderp;
	zed::ZedBodies<zed::ZedSkeletonCompact_34> *zedBodyData_C34p;
	zed::ZedBodies<zed::ZedSkeletonCompact_38> *zedBodyData_C38p;
	zed::ZedBodies<zed::ZedSkeletonFull_34> *zedBodyData_F34p;
	zed::ZedBodies<zed::ZedSkeletonFull_38> *zedBodyData_F38p;

	// How it goes out to Unity
	//zed::ZedBodies* zedBodyData;

	sig::SignalMetadata* sigMetadata;
	void zed_shutdown() {
		delete[] signalData;
	}


	void zed_initialize() {


		//extern net::cfg::Root master_cfg;

		bool loader = net::cfg::Root().load();
		if (!loader) {
			spdlog::error("Can't load master config");
			return;
		}
		net::cfg::Root master_cfg = net::cfg::Root().instance();

		json zedopts;
		json zedglobalopts;

		
		zedopts = master_cfg.user_signals.at("zed").at("v2.1").opts;
		zedglobalopts = master_cfg.user_signals.at("zed").at("v2.1").globalopts;
		zedopts.merge_patch(zedglobalopts);
		
		json zedpackedopts = nlohmann::json({ {"zed/v2.1", zedopts } });

		zed::BufferProperties bufferProperties;
		bufferProperties.populate_from_json(zedpackedopts);

		// These should be derived from the config file somehow
		

		std::stringstream ss;
		ss << "Dancegraph_Zed_Out";
		std::string name = ss.str();		
		int numEntries = 25;
		
		int bufSize;
		switch (bufferProperties.bodySignalType) {
		case zed::Zed4SignalType::Body_38_Compact:
			bufSize = sizeof(sig::SignalMetadata) + zed::ZedBodies<zed::ZedSkeletonCompact_38>::size(bufferProperties.maxBodyCount); break;
		case zed::Zed4SignalType::Body_34_Full:
			bufSize = sizeof(sig::SignalMetadata) + zed::ZedBodies<zed::ZedSkeletonFull_34>::size(bufferProperties.maxBodyCount); break;
		case zed::Zed4SignalType::Body_38_Full:
			bufSize = sizeof(sig::SignalMetadata) + zed::ZedBodies<zed::ZedSkeletonFull_38>::size(bufferProperties.maxBodyCount); break;
		case zed::Zed4SignalType::Body_34_Compact: default:			
			bufSize = sizeof(sig::SignalMetadata) + zed::ZedBodies<zed::ZedSkeletonCompact_34>::size(bufferProperties.maxBodyCount); break;
		}

		signalData = new int8_t[bufSize];

		zedHeaderp = (zed::Zed4Header*)(signalData + sizeof(sig::SignalMetadata));

		zedBodyData_C34p = (zed::ZedBodies<zed::ZedSkeletonCompact_34>*) ((char*)signalData + sizeof(sig::SignalMetadata));
		zedBodyData_C38p = (zed::ZedBodies<zed::ZedSkeletonCompact_38>*) ((char*)signalData + sizeof(sig::SignalMetadata));
		zedBodyData_F34p = (zed::ZedBodies<zed::ZedSkeletonFull_34>*) ((char*)signalData + sizeof(sig::SignalMetadata));
		zedBodyData_F38p = (zed::ZedBodies<zed::ZedSkeletonFull_38>*) ((char*)signalData + sizeof(sig::SignalMetadata));

		//zedBodyData = (zed::ZedBodiesQuant*)((char*)signalData + sizeof(sig::SignalMetadata));

		spdlog::info("Max Body Count: {}, bufSize: {}, numEntries: {} ({} + {} = {})", bufferProperties.maxBodyCount, bufSize, numEntries,
			sizeof(sig::SignalMetadata),
			zed::ZedBodies<zed::ZedSkeletonCompact_34>::size(bufferProperties.maxBodyCount),
			sizeof(sig::SignalMetadata) +
			zed::ZedBodies<zed::ZedSkeletonCompact_34>::size(bufferProperties.maxBodyCount));
			
		zedAdapterp = std::unique_ptr<ZedAdapter>(new ZedAdapter(name, bufSize, numEntries));
		sigMetadata = (sig::SignalMetadata*)signalData;

	}


	ZedAdapter::ZedAdapter(std::string name, int size, int entries) : ipcReader(name, size, entries) {

	}

	ZedAdapter::~ZedAdapter () {
	}

	bool ZedAdapter::read_ipc() {
		ipc::RBError err = ipcReader.read((void*) signalData);
		
		if (err == ipc::RBError::SUCCESS) {
			
			std::stringstream ss;

			int readsize = ipcReader.get_last_read_size();
			for (int i = 0; i < readsize; i++) {				
				ss << (int)signalData[i] << " ";
			}

			spdlog::info("{} bytes from IPC: {}", readsize, ss.str());
		}
		return (err == ipc::RBError::SUCCESS);
	}


	int ZedAdapter::get_numbodies() {
		return zedHeaderp->num_skeletons;				
	}
	
	unsigned long long ZedAdapter::get_elapsed() {		
		//return std::chrono::duration_cast<std::chrono::milliseconds>(zedHeaderp->elapsed.time_since_epoch()).count();
		return 0;
	}

	//unsigned long long ZedAdapter::get_timestamp(bool meta) {
	unsigned long long ZedAdapter::get_timestamp(bool meta) {
		return std::chrono::duration_cast<std::chrono::milliseconds>(sigMetadata->acquisitionTime.time_since_epoch()).count();
		
	}

	int ZedAdapter::get_userID() {
		//return sigHeader->userID;
		return sigMetadata->userIdx;
	}
	
	int ZedAdapter::get_packetID() {
		//return sigHeader->packetID;
		return sigMetadata->packetId;
	}

	int ZedAdapter::get_dataSize() {
		return ipcReader.get_last_read_size();
	}

	template
		<typename T>
	void root_transform(float fp [], zed::ZedBodies<T>* bodyDatap, int skelcount) {
		for (int i = 0; i < skelcount; i++) {
			*(fp + i * 7 + 0) = bodyDatap->skeletons[i].root_transform.pos.x;
			*(fp + i * 7 + 0) = bodyDatap->skeletons[i].root_transform.pos.y;
			*(fp + i * 7 + 0) = bodyDatap->skeletons[i].root_transform.pos.z;

			zed::quant_quat q = bodyDatap->skeletons[i].root_transform.ori;

			float fx = ((float)q.x) / 32767;
			float fy = ((float)q.y) / 32767;
			float fz = ((float)q.z) / 32767;
			*(fp + i * 7 + 3) = fx;
			*(fp + i * 7 + 4) = fy;
			*(fp + i * 7 + 5) = fz;

			*(fp + i * 7 + 6) = sqrt(1.0f - fx * fx - fy * fy - fz * fz);
		}
	}



	void ZedAdapter::get_roottransform(float fp[]) {
		switch (zedAdapterp->sigType) {

		case zed::Zed4SignalType::Body_38_Compact:
			root_transform<zed::ZedSkeletonCompact_38>(fp, zedBodyData_C38p, zedHeaderp->num_skeletons); break;
		case zed::Zed4SignalType::Body_34_Full:
			root_transform<zed::ZedSkeletonFull_34>(fp, zedBodyData_F34p, zedHeaderp->num_skeletons); break;
		case zed::Zed4SignalType::Body_38_Full:
			root_transform<zed::ZedSkeletonFull_38>(fp, zedBodyData_F38p, zedHeaderp->num_skeletons); break;
		case zed::Zed4SignalType::Body_34_Compact:default:
			root_transform<zed::ZedSkeletonCompact_34>(fp, zedBodyData_C34p, zedHeaderp->num_skeletons); break;
		}
	}
		

	void ZedAdapter::get_bodyIDs(int ip[]) {
		for (int i = 0; i < zedHeaderp->num_skeletons; i++) {
			switch (zedAdapterp->sigType) {
			case zed::Zed4SignalType::Body_38_Compact:
				ip[i] = zedBodyData_C38p->skeletons[i].id;
			case zed::Zed4SignalType::Body_34_Full:
				ip[i] = zedBodyData_F34p->skeletons[i].id;
			case zed::Zed4SignalType::Body_38_Full:
				ip[i] = zedBodyData_F38p->skeletons[i].id;
			case zed::Zed4SignalType::Body_34_Compact:default:
				ip[i] = zedBodyData_C34p->skeletons[i].id;
			}
		}
	}

	template 
		<class T>
		void process_bones(float fp[], zed::ZedBodies<T> * zedBodiesp) {
		int bonecount = zed::signal_bonecount.at(zedAdapterp->sigType).first;

		int signal_bonecount = zed::signal_bonecount.at(zedAdapterp->sigType).first;
		int full_bonecount = zed::signal_bonecount.at(zedAdapterp->sigType).second;

		auto usefulbones = zed::useful_bone_indices.at(zedAdapterp->sigType);

		for (int i = 0; i < zedHeaderp->num_skeletons; i++) {
			int usefulidx = 0;			
			for (int j = 0; j < full_bonecount; j++) {
				float* f = (float*)fp + i * full_bonecount * 4 + j * 4;
				
				if (j == usefulbones[usefulidx]) {
					zed::quant_quat q = zedBodiesp->skeletons[i].bone_rotations[usefulidx];
					*(f + 0) = ((float)q.x) / 32767;
					*(f + 1) = ((float)q.y) / 32767;
					*(f + 2) = ((float)q.z) / 32767;
					*(f + 3) = sqrt(1.0f - f[0] * f[0] - f[1] * f[1] - f[2] * f[2]);
					usefulidx += 1;
				}
				else {
					*(f + 0) = 0;
					*(f + 1) = 0;
					*(f + 2) = 0;
					*(f + 3) = 1;
				}
				
			}
			
		}
	}


	void ZedAdapter::get_bodyData(float fp[]) {
		std::vector<int> usefulbones = zed::useful_bone_indices.at(sigType);
		switch (zedAdapterp->sigType) {
		case zed::Zed4SignalType::Body_38_Compact:
			process_bones<zed::ZedSkeletonCompact_38>(fp, zedBodyData_C38p);
		case zed::Zed4SignalType::Body_34_Full:
			process_bones<zed::ZedSkeletonFull_34>(fp, zedBodyData_F34p);
		case zed::Zed4SignalType::Body_38_Full:
			process_bones<zed::ZedSkeletonFull_38>(fp, zedBodyData_F38p);			
		case zed::Zed4SignalType::Body_34_Compact:default:
			process_bones<zed::ZedSkeletonCompact_34>(fp, zedBodyData_C34p);
			
		}

	}

}

