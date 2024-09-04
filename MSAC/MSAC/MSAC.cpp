#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include "zstd.h"
#include <set>
#include <unordered_map>
#include <cctype>
#include <chrono>
#include "MSACompressor.hpp"

int main(int argc, char* argv[]) {
	MSACompressor compressor;
	if (argc < 4) {
		compressor.printUsage();
		return 1;
	}
	std::string mode = argv[1];
	std::string inFile = argv[2];
	std::string outFile = argv[3];
	int zstdLevel = 13;
	int A = 200000;
	int B = 9000;
	PreprocessingType preprocessingType = PreprocessingType::REDUCE_GAPS_A;
	int pos = 0;

	if (mode == "Sc") {
		for (int i = 4; i < argc; ++i) {
			std::string arg = argv[i];

			if (arg.substr(0, 2) == "-a") {
				A = std::stoi(arg.substr(2));
				if (A < 1) A = 1;
			}
			else if (arg.substr(0, 2) == "-b") {
				B = std::stoi(arg.substr(2));
				if (B < 1) B = 1;
			}
			else if (arg.substr(0, 2) == "-z") {
				zstdLevel = std::stoi(arg.substr(2));
				if (zstdLevel < 1) zstdLevel = 1;
				if (zstdLevel > 19) zstdLevel = 19;
			}
			else if (arg.substr(0, 2) == "-p") {
				int pType = std::stoi(arg.substr(2));
				switch (pType) {
				case 0:
					preprocessingType = PreprocessingType::NO_PREPROCESSING;
					break;
				case 1:
					preprocessingType = PreprocessingType::REDUCE_GAPS_A;
					break;
				case 2:
					preprocessingType = PreprocessingType::REDUCE_GAPS_B;
					break;
				case 3:
					preprocessingType = PreprocessingType::REDUCE_GAPS_C;
					break;
				case 4:
					preprocessingType = PreprocessingType::REDUCE_GAPS_AND_LOWERCASE;
					break;
				case 5:
					preprocessingType = PreprocessingType::REDUCE_GAPS_AND_UPPERCASE;
					break;
				default:
					compressor.printUsage();
					exit(1);
				}
			}
			else {
				compressor.printUsage();
				exit(1);
			}
		}
	}
	else {
		for (int i = 4; i < argc; ++i) {
			std::string arg = argv[i];
			if (arg.substr(0, 2) == "-p") {
				int pType = std::stoi(arg.substr(2));
				switch (pType) {
				case 0:
					preprocessingType = PreprocessingType::NO_PREPROCESSING;
					break;
				case 1:
					preprocessingType = PreprocessingType::REDUCE_GAPS_A;
					break;
				case 2:
					preprocessingType = PreprocessingType::REDUCE_GAPS_B;
					break;
				case 3:
					preprocessingType = PreprocessingType::REDUCE_GAPS_C;
					break;
				case 4:
					preprocessingType = PreprocessingType::REDUCE_GAPS_AND_LOWERCASE;
					break;
				case 5:
					preprocessingType = PreprocessingType::REDUCE_GAPS_AND_UPPERCASE;
					break;
				default:
					compressor.printUsage();
					exit(1);
				}
				pos=i;
			}
		}
	}

	if (mode == "Sc") {
		compressor.compress(inFile, outFile, zstdLevel, A, B, preprocessingType);
		std::cout << "File compressed successfully." << std::endl;
	}
	else if (mode == "Sd") {
		compressor.decompress(inFile, outFile, preprocessingType);
		std::cout << "File decompressed successfully." << std::endl;
	}
	else if (mode == "Ds") {
		std::vector<std::string> sequenceIds;
		std::string id;
		for (int i = 4; i < argc; i++)
		{
			if (i != pos) {
				id = argv[i];
				sequenceIds.push_back(id);
			}
		}
		compressor.decompressSequences(inFile, outFile, sequenceIds, preprocessingType);
		std::cout << "File decompressed successfully." << std::endl;
	}
	else if (mode == "Dc") {
		std::vector<int> columnsIds;
		int id;
		for (int i = 4; i < argc; i++)
		{
			if (i != pos) {
				id = atoi(argv[i]);
				columnsIds.push_back(id);
			}
		}
		compressor.decompressColumns(inFile, outFile, columnsIds, preprocessingType);
		std::cout << "File decompressed successfully." << std::endl;
	}
	else if (mode == "Drc") {
		std::vector<int> columnsIds;
		int startId, stopId;
		if (pos == 0) {
			startId = atoi(argv[4]);
			stopId = atoi(argv[5]);
		}
		else {
			startId = atoi(argv[5]);
			stopId = atoi(argv[6]);
		}
		for (int i = startId; i < stopId + 1; i++)
		{
			columnsIds.push_back(i);
		}
		compressor.decompressColumns(inFile, outFile, columnsIds, preprocessingType);
		std::cout << "File decompressed successfully." << std::endl;
	}
	else {
		std::cerr << "Invalid mode." << std::endl;
		compressor.printUsage();
		return 1;
	}

	return 0;
}