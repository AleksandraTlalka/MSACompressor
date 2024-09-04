#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <unordered_map>
#include <cctype>
#include "zstd.h"
#include "MSACompressor.hpp"


void MSACompressor::applyPreprocessing(Rectangle& rect, PreprocessingType preprocessingType) {
	switch (preprocessingType) {
	case NO_PREPROCESSING:
		break;
	case REDUCE_GAPS_A:
		reduceGapsA(rect);
		break;
	case REDUCE_GAPS_B:
		reduceGapsB(rect);
		break;
	case REDUCE_GAPS_C:
		reduceGapsC(rect);
		break;
	case REDUCE_GAPS_AND_LOWERCASE:
		reduceGapsAndLowerCase(rect);
		break;
	case REDUCE_GAPS_AND_UPPERCASE:
		reduceGapsAndUpperCase(rect);
		break;
	}
}

void MSACompressor::reversePreprocessing(Rectangle& rect, PreprocessingType preprocessingType, const std::vector<std::string>& sequenceIds) {
	switch (preprocessingType) {
	case NO_PREPROCESSING:
		decompressRectangle(rect, sequenceIds);
		//decompressRectangleByColumn(rect, sequenceIds);
		break;
	case REDUCE_GAPS_A:
		reverseGapsA(rect, sequenceIds);
		break;
	case REDUCE_GAPS_B:
		reverseGapsB(rect, sequenceIds);
		break;
	case REDUCE_GAPS_C:
		reverseGapsC(rect, sequenceIds);
		break;
	case REDUCE_GAPS_AND_LOWERCASE:
		reverseGapsA(rect, sequenceIds);
		break;
	case REDUCE_GAPS_AND_UPPERCASE:
		reverseGapsA(rect, sequenceIds);
		break;
	}
}

void MSACompressor::compressRectangle(Rectangle& rect, int zstdLevel, PreprocessingType preprocessingType) {

	applyPreprocessing(rect, preprocessingType);

	std::string rectData;
	for (const auto& seq : rect.sequences) {
		for (const auto& data : seq.data) {
			rectData.push_back(data);
		}
		rectData += '#';
	}
	rect.compressedData.resize(ZSTD_compressBound(rectData.size()));
	size_t compressedSize = ZSTD_compress(rect.compressedData.data(), rect.compressedData.size(), rectData.data(), rectData.size(), zstdLevel);
	if (ZSTD_isError(compressedSize)) {
		std::cerr << "Compression error: " << ZSTD_getErrorName(compressedSize) << std::endl;
		return;
	}
	rect.compressedData.resize(compressedSize);
}

void MSACompressor::compressRectangleByColumn(Rectangle& rect, int zstdLevel, PreprocessingType preprocessingType) {

	applyPreprocessing(rect, preprocessingType);

	std::string rectData;
	for (int i = 0; i < rect.height; i++) {
		for (const auto& seq : rect.sequences) {
			rectData.push_back(seq.data[i]);
		}
		rectData += '#';
	}
	rect.compressedData.resize(ZSTD_compressBound(rectData.size()));
	size_t compressedSize = ZSTD_compress(rect.compressedData.data(), rect.compressedData.size(), rectData.data(), rectData.size(), zstdLevel);
	if (ZSTD_isError(compressedSize)) {
		std::cerr << "Compression error: " << ZSTD_getErrorName(compressedSize) << std::endl;
		return;
	}
	rect.compressedData.resize(compressedSize);
}

void MSACompressor::reduceGapsA(Rectangle& rect) {
	for (auto& seq : rect.sequences) {
		std::ostringstream result;
		char lastChar = '\0';
		int dotCount = 0;
		if (seq.data[0] == '.') {
			lastChar = '.';
			dotCount = -1;
		}

		for (char ch : seq.data) {
			if (ch != '.') {
				if (lastChar != '\0') {
					result << lastChar;
					if (dotCount > 0) {
						result << dotCount;
					}
				}
				lastChar = ch;
				dotCount = 0;
			}
			else if (ch == '.') {
				++dotCount;
			}
		}

		if (lastChar != '\0') {
			result << lastChar;
			if (dotCount > 0) {
				result << dotCount;
			}
		}

		std::string transformedData = result.str();
		seq.data.assign(transformedData.begin(), transformedData.end());
	}
}

void MSACompressor::reduceGapsB(Rectangle& rect) {
	for (auto& seq : rect.sequences) {
		std::ostringstream result;
		std::ostringstream positions;
		char lastChar = '\0';
		int dotCount = 0;
		int counter = 0;
		if (seq.data[0] == '.') {
			lastChar = '.';
			dotCount = -1;
			positions << counter << ',';
		}

		for (char ch : seq.data) {
			if (ch != '.') {
				if (lastChar != '\0') {
					result << lastChar;
					if (dotCount > 0) {
						positions << dotCount << ',';
					}
				}
				lastChar = ch;
				dotCount = 0;
			}
			else if (ch == '.') {
				if (dotCount == 0) {
					positions << counter << ',';
				}
				++dotCount;
			}
			++counter;
		}

		if (lastChar != '\0') {
			result << lastChar;
			if (dotCount > 0) {
				positions << dotCount;
			}
		}

		std::string transformedPositions = positions.str();
		std::string transformedData = transformedPositions + "@" + result.str();
		seq.data.assign(transformedData.begin(), transformedData.end());
	}
}

void MSACompressor::reduceGapsC(Rectangle& rect) {
	for (auto& seq : rect.sequences) {
		std::ostringstream result;
		std::ostringstream numbers;
		char lastChar = '\0';
		int dotCount = 0;
		int symbolCount = 0;
		if (seq.data[0] == '.') {
			lastChar = '.';
			dotCount = -1;
		}
		for (char ch : seq.data) {
			if (ch != '.') {
				if (lastChar != '\0') {
					result << lastChar;
					if (dotCount > 0) {
						numbers << dotCount << ',';
					}
				}
				lastChar = ch;
				++symbolCount;
				dotCount = 0;
			}
			else if (ch == '.') {
				if (dotCount == 0) {
					numbers << symbolCount << ',';
					symbolCount = 0;
				}
				++dotCount;
			}
		}

		if (lastChar != '\0') {
			result << lastChar;
			numbers << symbolCount << ',';
			if (dotCount > 0) {
				numbers << dotCount;
			}
		}

		std::string transformedPositions = numbers.str();
		std::string transformedData = transformedPositions + "@" + result.str();
		seq.data.assign(transformedData.begin(), transformedData.end());
	}
}

void MSACompressor::reverseGapsA(Rectangle& rect, const std::vector<std::string>& sequenceIds) {
	std::vector<char> decompressedData(rect.width * rect.height * sizeof(char) * 2);
	size_t decompressedSize = ZSTD_decompress(decompressedData.data(), decompressedData.size(), rect.compressedData.data(), rect.compressedData.size());
	if (ZSTD_isError(decompressedSize)) {
		std::cerr << "Decompression error: " << ZSTD_getErrorName(decompressedSize) << std::endl;
		return;
	}

	decompressedSize = decompressedData.size();
	size_t dataIndex = 0;

	for (int row = 0; row < rect.width; ++row) {
		Sequence seq;
		seq.id = sequenceIds[rect.startX + row];

		while (dataIndex < decompressedSize && decompressedData[dataIndex] != '#' && decompressedData[dataIndex] != '\0') {
			char symbol = decompressedData[dataIndex++];

			seq.data.push_back(symbol);
			if (dataIndex < decompressedSize && std::isdigit(decompressedData[dataIndex])) {
				int dotCount = 0;
				while (dataIndex < decompressedSize && std::isdigit(decompressedData[dataIndex])) {
					dotCount = dotCount * 10 + (decompressedData[dataIndex] - '0');
					++dataIndex;
				}
				for (int i = 0; i < dotCount; ++i) {
					seq.data.push_back('.');
				}
			}
		}

		if (dataIndex < decompressedSize && decompressedData[dataIndex] == '#') {
			++dataIndex;
		}

		rect.sequences.push_back(seq);
	}
}

void MSACompressor::reverseGapsB(Rectangle& rect, const std::vector<std::string>& sequenceIds) {
	std::vector<char> decompressedData(rect.width * rect.height * sizeof(char) * 2);
	size_t decompressedSize = ZSTD_decompress(decompressedData.data(), decompressedData.size(), rect.compressedData.data(), rect.compressedData.size());
	if (ZSTD_isError(decompressedSize)) {
		std::cerr << "Decompression error: " << ZSTD_getErrorName(decompressedSize) << std::endl;
		return;
	}

	size_t dataIndex = 0;

	for (int row = 0; row < rect.width; ++row) {
		Sequence seq;
		seq.id = sequenceIds[rect.startX + row];

		std::string posString;
		while (dataIndex < decompressedSize && decompressedData[dataIndex] != '@') {
			posString.push_back(decompressedData[dataIndex++]);
		}
		++dataIndex;

		std::istringstream posStream(posString);
		std::vector<int> dotPositions;
		std::vector<int> dotNumbers;
		std::string token;
		while (std::getline(posStream, token, ',')) {
			dotPositions.push_back(std::stoi(token));
			std::getline(posStream, token, ',');
			dotNumbers.push_back(std::stoi(token));
		}
		dotPositions.push_back(decompressedSize);

		int posIndex = 0;
		int seqIndex = 0;
		int counter = 0;
		while (dataIndex < decompressedSize && decompressedData[dataIndex] != '#' && decompressedData[dataIndex] != '\0') {
			posIndex = dotPositions[counter];
			if (seqIndex < posIndex) {
				seq.data.push_back(decompressedData[dataIndex]);
				++dataIndex;
				++seqIndex;
			}
			else {
				for (int i = 0; i < dotNumbers[counter]; i++) {
					seq.data.push_back('.');
					++seqIndex;
				}
				++counter;
			}
		}

		if (dataIndex < decompressedSize && decompressedData[dataIndex] == '#') {
			++dataIndex;
		}

		rect.sequences.push_back(seq);
	}
}

void MSACompressor::reverseGapsC(Rectangle& rect, const std::vector<std::string>& sequenceIds) {
	std::vector<char> decompressedData(rect.width * rect.height * sizeof(char) * 2);
	size_t decompressedSize = ZSTD_decompress(decompressedData.data(), decompressedData.size(), rect.compressedData.data(), rect.compressedData.size());
	if (ZSTD_isError(decompressedSize)) {
		std::cerr << "Decompression error: " << ZSTD_getErrorName(decompressedSize) << std::endl;
		return;
	}

	size_t dataIndex = 0;

	for (int row = 0; row < rect.width; ++row) {
		Sequence seq;
		seq.id = sequenceIds[rect.startX + row];

		std::string posString;
		while (dataIndex < decompressedSize && decompressedData[dataIndex] != '@') {
			posString.push_back(decompressedData[dataIndex++]);
		}
		++dataIndex;

		std::istringstream posStream(posString);
		std::vector<int> numbers;
		std::string token;
		while (std::getline(posStream, token, ',')) {
			numbers.push_back(std::stoi(token));
		}

		int posIndex = 0;
		int seqIndex = 0;
		int counter = 0;
		int flag = 0;
		while (dataIndex < decompressedSize && decompressedData[dataIndex] != '#' && decompressedData[dataIndex] != '\0') {
			if (flag == 0) {
				for (int i = 0; i < numbers[counter]; i++) {
					seq.data.push_back(decompressedData[dataIndex]);
					dataIndex++;
				}
				flag = 1;
				counter++;
			}
			else {
				for (int i = 0; i < numbers[counter]; i++) {
					seq.data.push_back('.');
				}
				flag = 0;
				counter++;
			}
		}

		if (dataIndex < decompressedSize && decompressedData[dataIndex] == '#') {
			++dataIndex;
		}

		rect.sequences.push_back(seq);
	}
}

void MSACompressor::reduceGapsAndUpperCase(Rectangle& rect) {
	for (auto& seq : rect.sequences) {
		std::ostringstream result;
		char lastChar = '\0';
		int dotCount = 0;
		if (seq.data[0] == '.') {
			lastChar = '.';
			dotCount = -1;
		}
		for (char ch : seq.data) {
			ch = std::toupper(ch);
			if (ch != '.') {
				if (lastChar != '\0') {
					result << lastChar;
					if (dotCount > 0) {
						result << dotCount;
					}
				}
				lastChar = ch;
				dotCount = 0;
			}
			else if (ch == '.') {
				++dotCount;
			}
		}
		if (lastChar != '\0') {
			result << lastChar;
			if (dotCount > 0) {
				result << dotCount;
			}
		}
		std::string transformedData = result.str();
		seq.data.assign(transformedData.begin(), transformedData.end());
	}
}

void MSACompressor::reduceGapsAndLowerCase(Rectangle& rect) {
	for (auto& seq : rect.sequences) {
		std::ostringstream result;
		char lastChar = '\0';
		int dotCount = 0;
		if (seq.data[0] == '.') {
			lastChar = '.';
			dotCount = -1;
		}
		for (char ch : seq.data) {
			ch = std::tolower(ch);
			if (ch != '.') {
				if (lastChar != '\0') {
					result << lastChar;
					if (dotCount > 0) {
						result << dotCount;
					}
				}
				lastChar = ch;
				dotCount = 0;
			}
			else if (ch == '.') {
				++dotCount;
			}
		}
		if (lastChar != '\0') {
			result << lastChar;
			if (dotCount > 0) {
				result << dotCount;
			}
		}
		std::string transformedData = result.str();
		seq.data.assign(transformedData.begin(), transformedData.end());
	}
}

void MSACompressor::decompressRectangle(Rectangle& rect, const std::vector<std::string>& sequenceIds) {
	std::vector<char> decompressedData(rect.width * rect.height * sizeof(char) * 2);
	size_t decompressedSize = ZSTD_decompress(decompressedData.data(), decompressedData.size(), rect.compressedData.data(), rect.compressedData.size());
	if (ZSTD_isError(decompressedSize)) {
		std::cerr << "Decompression error: " << ZSTD_getErrorName(decompressedSize) << std::endl;
		return;
	}

	decompressedSize = decompressedData.size();
	size_t dataIndex = 0;

	for (int row = 0; row < rect.width; ++row) {
		Sequence seq;
		seq.id = sequenceIds[rect.startX + row];

		while (dataIndex < decompressedSize && decompressedData[dataIndex] != '#' && decompressedData[dataIndex] != '\0') {
			char symbol = decompressedData[dataIndex++];

			seq.data.push_back(symbol);
		}

		if (dataIndex < decompressedSize && decompressedData[dataIndex] == '#') {
			++dataIndex;
		}

		rect.sequences.push_back(seq);
	}
}

void MSACompressor::decompressRectangleByColumn(Rectangle& rect, const std::vector<std::string>& sequenceIds) {
	std::vector<char> decompressedData(rect.width * rect.height * sizeof(char) * 2);
	size_t decompressedSize = ZSTD_decompress(decompressedData.data(), decompressedData.size(), rect.compressedData.data(), rect.compressedData.size());
	if (ZSTD_isError(decompressedSize)) {
		std::cerr << "Decompression error: " << ZSTD_getErrorName(decompressedSize) << std::endl;
		return;
	}

	decompressedSize = decompressedData.size();
	size_t dataIndex = 0;
	std::vector <Sequence> sequences;
	Sequence seq;

	for (int row = 0; row < rect.width; ++row) {
		seq.id = sequenceIds[rect.startX + row];
		sequences.push_back(seq);
	}

	for (int column = 0; column < rect.height; ++column) {
		for (int row = 0; row < rect.width; ++row) {
			if (dataIndex < decompressedSize && decompressedData[dataIndex] != '#' && decompressedData[dataIndex] != '\0') {
				char symbol = decompressedData[dataIndex++];
				sequences[row].data.push_back(symbol);
			}
		}
		if (dataIndex < decompressedSize && decompressedData[dataIndex] == '#') {
			++dataIndex;
		}

	}
	for (int row = 0; row < rect.width; ++row) {

		rect.sequences.push_back(sequences[row]);
	}
}

void MSACompressor::splitSequencesIntoRectangles(const std::vector<Sequence>& sequences, int startX, std::vector<Rectangle>& rectangles, int A, int B) {
	int numRows = sequences.size();
	if (numRows == 0) return;
	int numCols = sequences[0].data.size();
	for (int x = 0; x < numRows; x += A) {
		for (int y = 0; y < numCols; y += B) {
			Rectangle rect;
			rect.startX = startX;
			rect.startY = y;
			rect.width = std::min(A, numRows - x);
			rect.height = std::min(B, numCols - y);
			for (int i = 0; i < rect.width; ++i) {
				Sequence seq = sequences[x + i];
				Sequence subSeq;
				subSeq.id = seq.id;
				for (int j = 0; j < rect.height; ++j) {
					subSeq.data.push_back(seq.data[y + j]);
				}
				rect.sequences.push_back(subSeq);
			}
			rectangles.push_back(std::move(rect));
		}
	}
}

MSACompressor::MSACompressor() {

}

void MSACompressor::printUsage() {
	std::cout << "Usage:\n";
	std::cout << "  MSAC.exe [mode] <input_file> <output_file> [options]\n\n";
	std::cout << "Modes:\n";
	std::cout << "  Sc             Compress the file.\n";
	std::cout << "  Sd             Decompress the file.\n";
	std::cout << "  Ds             Decompress sequences.\n";
	std::cout << "  Dc             Decompress columns.\n";
	std::cout << "  Drc            Decompress a range of columns.\n\n";

	std::cout << "Options:\n";
	std::cout << "  -a<number>     Set value A (number of rows in the rectangle) (default: 200000)\n";
	std::cout << "  -b<number>     Set value B (number of columns in the rectangle) (default: 10000)\n";
	std::cout << "  -z<number>     Compression level for Zstd (from 1 to 19) (default: 13)\n";
	std::cout << "  -p<number>     Preprocessing mode:\n";
	std::cout << "                 0 - no preprocessing\n";
	std::cout << "                 1 - reduce gaps ver1\n";
	std::cout << "                 2 - reduce gaps ver2\n";
	std::cout << "                 3 - reduce gaps ver3\n";
	std::cout << "                 4 - reduce gaps and convert to lowercase\n";
	std::cout << "                 5 - reduce gaps and convert to uppercase\n";

	std::cout << "\nExamples:\n";

	std::cout << "  MSAC.exe Sc input.txt output.msac -a5 -b10 -z3 -p1\n";
	std::cout << "  MSAC.exe Sd input.msac output.txt -p3\n";
	std::cout << "  MSAC.exe Ds input.msac output.txt <SequenceId> <SequenceId> ...\n";
	std::cout << "  MSAC.exe Dc input.msac output.txt <ColumnNumber> ...\n";
	std::cout << "  MSAC.exe Drc input.msac output.txt <StartColumnNumber> <StopColumnNumber>\n";
}

void MSACompressor::compress(const std::string& inputFile, const std::string& outputFile, int zstdLevel, int A, int B, PreprocessingType preprocessingType) {

	std::ifstream ifs(inputFile);
	if (!ifs) {
		std::cerr << "Error: Unable to open input file: " << inputFile << std::endl;
		exit(1);
	}

	std::ofstream ofs(outputFile, std::ios::binary);
	if (!ofs) {
		std::cerr << "Error: Unable to open output file: " << outputFile << std::endl;
		exit(1);
	}

	std::vector<std::string> headers;
	std::string line;
	while (std::getline(ifs, line)) {
		if (line.empty() || line[0] == '/') {
			continue;
		}
		if (line[0] == '#') {
			headers.push_back(line);
			continue;
		}
		break;
	}

	for (const auto& header : headers) {
		ofs.write(header.c_str(), header.size());
		ofs.put('\n');
	}

	uint64_t dataStartPos = ofs.tellp();
	std::vector<std::string> uniqueIds;
	std::vector<Sequence> sequences;
	std::vector<std::tuple<int, int, int, int, uint64_t>> footer;
	int currentX = 0;

	do {
		std::istringstream iss(line);
		if (line[0] == '/')
			break;
		std::string id;
		std::string data;
		iss >> id >> data;
		Sequence seq = { id, {} };
		char lastSymbol = '\0';

		for (char c : data) {
			if (lastSymbol != '\0') {
				seq.data.emplace_back(lastSymbol);
			}
			lastSymbol = c;
		}
		if (lastSymbol != '\0') {
			seq.data.emplace_back(lastSymbol);
		}
		sequences.push_back(std::move(seq));

		if (sequences.size() >= A) {
			std::vector<Rectangle> rectangles;
			splitSequencesIntoRectangles(sequences, currentX, rectangles, A, B);
			for (auto& rect : rectangles) {
				compressRectangle(rect, zstdLevel, preprocessingType);
				//compressRectangleByColumn(rect, zstdLevel, preprocessingType);
				ofs.write(rect.compressedData.data(), rect.compressedData.size());
				if (rect.startY == 0) {
					for (const auto& seq : rect.sequences) {
						uniqueIds.push_back(seq.id);
					}
				}
				uint64_t compressedSize = rect.compressedData.size();
				footer.emplace_back(rect.startX, rect.startY, rect.width, rect.height, compressedSize);
			}
			currentX += A;
			sequences.clear();
		}
	} while (std::getline(ifs, line));

	if (!sequences.empty()) {
		std::vector<Rectangle> rectangles;
		splitSequencesIntoRectangles(sequences, currentX, rectangles, A, B);
		for (auto& rect : rectangles) {
			compressRectangle(rect, zstdLevel, preprocessingType);
			//compressRectangleByColumn(rect, zstdLevel, preprocessingType);
			ofs.write(rect.compressedData.data(), rect.compressedData.size());
			if (rect.startY == 0) {
				for (const auto& seq : rect.sequences) {
					uniqueIds.push_back(seq.id);
				}
			}
			uint64_t compressedSize = rect.compressedData.size();
			footer.emplace_back(rect.startX, rect.startY, rect.width, rect.height, compressedSize);
		}
	}

	uint64_t sequenceIdsStartPos = ofs.tellp();

	for (const auto& id : uniqueIds) {
		uint16_t idLength = id.size();
		ofs.write(reinterpret_cast<const char*>(&idLength), sizeof(idLength));
		ofs.write(id.data(), idLength);
	}

	uint64_t footerStartPos = ofs.tellp();

	for (const auto& entry : footer) {
		int startX, startY, width, height;
		uint64_t compressedSize;
		std::tie(startX, startY, width, height, compressedSize) = entry;

		ofs.write(reinterpret_cast<const char*>(&startX), sizeof(startX));
		ofs.write(reinterpret_cast<const char*>(&startY), sizeof(startY));
		ofs.write(reinterpret_cast<const char*>(&width), sizeof(width));
		ofs.write(reinterpret_cast<const char*>(&height), sizeof(height));
		ofs.write(reinterpret_cast<const char*>(&compressedSize), sizeof(compressedSize));
	}

	ofs.write(reinterpret_cast<const char*>(&dataStartPos), sizeof(dataStartPos));
	ofs.write(reinterpret_cast<const char*>(&sequenceIdsStartPos), sizeof(sequenceIdsStartPos));
	ofs.write(reinterpret_cast<const char*>(&footerStartPos), sizeof(footerStartPos));

	ofs.close();
}

void MSACompressor::decompress(const std::string& inputFile, const std::string& outputFile, PreprocessingType preprocesingType) {
	std::ifstream ifs(inputFile, std::ios::binary);
	if (!ifs) {
		std::cerr << "Error: Unable to open input file: " << inputFile << std::endl;
		exit(1);
	}

	ifs.seekg(-static_cast<int>(3 * sizeof(uint64_t)), std::ios::end);
	uint64_t dataStartPos, sequenceIdsStartPos, footerStartPos;
	ifs.read(reinterpret_cast<char*>(&dataStartPos), sizeof(dataStartPos));
	ifs.read(reinterpret_cast<char*>(&sequenceIdsStartPos), sizeof(sequenceIdsStartPos));
	ifs.read(reinterpret_cast<char*>(&footerStartPos), sizeof(footerStartPos));

	ifs.seekg(footerStartPos, std::ios::beg);
	std::vector<std::tuple<int, int, int, int, uint64_t>> footer;
	int resultSize = 0;
	while (true) {
		int startX, startY, width, height;
		uint64_t compressedSize;

		ifs.read(reinterpret_cast<char*>(&startX), sizeof(startX));
		if (startX == dataStartPos)break;
		ifs.read(reinterpret_cast<char*>(&startY), sizeof(startY));
		ifs.read(reinterpret_cast<char*>(&width), sizeof(width));
		ifs.read(reinterpret_cast<char*>(&height), sizeof(height));
		ifs.read(reinterpret_cast<char*>(&compressedSize), sizeof(compressedSize));

		footer.emplace_back(startX, startY, width, height, compressedSize);
		if (startX == 0) {
			resultSize += height;
		}
	}

	std::ofstream ofs(outputFile);
	if (!ofs) {
		std::cerr << "Error: Unable to open output file: " << outputFile << std::endl;
		exit(1);
	}

	ifs.clear();
	ifs.seekg(0);
	while (ifs.tellg() < dataStartPos) {
		std::string header;
		std::getline(ifs, header);
		ofs << header << std::endl;
	}

	std::vector<std::string> sequenceIds;
	ifs.seekg(sequenceIdsStartPos, std::ios::beg);
	std::streampos lineStartPos = ofs.tellp();
	std::unordered_map<std::string, std::streampos> seqIdPositions;

	while (ifs.tellg() < footerStartPos) {
		uint16_t idLength;
		ifs.read(reinterpret_cast<char*>(&idLength), sizeof(idLength));
		std::string id(idLength, ' ');
		ifs.read(&id[0], idLength);
		sequenceIds.push_back(id);
		ofs << id << " ";
		if (id.size() < 25) { ofs << std::string((25 - id.size()), ' '); }
		seqIdPositions[id] = ofs.tellp();
		ofs << std::string(resultSize, ' ');
		ofs << std::endl;
	}

	ofs.close();
	ifs.close();

	std::fstream ofsUpdate(outputFile, std::ios::in | std::ios::out);
	if (!ofsUpdate) {
		std::cerr << "Error: Unable to open output file for updating: " << outputFile << std::endl;
		exit(1);
	}

	ifs.open(inputFile, std::ios::binary);
	ifs.seekg(dataStartPos, std::ios::beg);

	for (const auto& entry : footer) {
		int startX, startY, width, height;
		uint64_t compressedSize;
		std::tie(startX, startY, width, height, compressedSize) = entry;

		Rectangle rect;
		rect.startX = startX;
		rect.startY = startY;
		rect.width = width;
		rect.height = height;
		rect.compressedData.resize(compressedSize);

		ifs.read(rect.compressedData.data(), compressedSize);
		if (!ifs) {
			std::cerr << "Error: Unable to read compressed data for rectangle at ("
				<< startX << ", " << startY << "). Expected size: " << compressedSize << std::endl;
			return;
		}

		reversePreprocessing(rect, preprocesingType, sequenceIds);

		for (const auto& seq : rect.sequences) {
			auto pos = seqIdPositions[seq.id];
			ofsUpdate.seekp(pos, std::ios::beg);
			ofsUpdate << std::string(seq.data.begin(), seq.data.end());
			std::streampos lenght = seq.data.size();
			seqIdPositions[seq.id] = ofsUpdate.tellp();
		}
	}

	ofsUpdate.close();
	ifs.close();
}

void MSACompressor::decompressSequences(const std::string& inputFile, const std::string& outputFile, std::vector<std::string>& chosenSequenceIds, PreprocessingType preprocessingType) {
	std::ifstream ifs(inputFile, std::ios::binary);
	if (!ifs) {
		std::cerr << "Error: Unable to open input file: " << inputFile << std::endl;
		exit(1);
	}

	ifs.seekg(-static_cast<int>(3 * sizeof(uint64_t)), std::ios::end);
	uint64_t dataStartPos, sequenceIdsStartPos, footerStartPos;
	ifs.read(reinterpret_cast<char*>(&dataStartPos), sizeof(dataStartPos));
	ifs.read(reinterpret_cast<char*>(&sequenceIdsStartPos), sizeof(sequenceIdsStartPos));
	ifs.read(reinterpret_cast<char*>(&footerStartPos), sizeof(footerStartPos));

	ifs.seekg(footerStartPos, std::ios::beg);
	std::vector<std::tuple<int, int, int, int, uint64_t>> footer;
	int resultSize = 0;
	while (true) {
		int startX, startY, width, height;
		uint64_t compressedSize;

		ifs.read(reinterpret_cast<char*>(&startX), sizeof(startX));
		if (startX == dataStartPos)break;
		ifs.read(reinterpret_cast<char*>(&startY), sizeof(startY));
		ifs.read(reinterpret_cast<char*>(&width), sizeof(width));
		ifs.read(reinterpret_cast<char*>(&height), sizeof(height));
		ifs.read(reinterpret_cast<char*>(&compressedSize), sizeof(compressedSize));

		footer.emplace_back(startX, startY, width, height, compressedSize);
		if (startX == 0) {
			resultSize += height;
		}
	}

	std::ofstream ofs(outputFile);
	if (!ofs) {
		std::cerr << "Error: Unable to open output file: " << outputFile << std::endl;
		exit(1);
	}

	ifs.clear();
	ifs.seekg(0);
	while (ifs.tellg() < dataStartPos) {
		std::string header;
		std::getline(ifs, header);
		ofs << header << std::endl;
	}

	std::vector<std::string> sequenceIds;
	ifs.seekg(sequenceIdsStartPos, std::ios::beg);
	std::streampos lineStartPos = ofs.tellp();
	std::unordered_map<std::string, std::streampos> seqIdPositions;
	int counter = 0;
	std::vector<int> linenumbers;

	while (ifs.tellg() < footerStartPos) {
		uint16_t idLength;
		ifs.read(reinterpret_cast<char*>(&idLength), sizeof(idLength));
		std::string id(idLength, ' ');
		ifs.read(&id[0], idLength);
		sequenceIds.push_back(id);
		for (auto i : chosenSequenceIds)
		{
			if (id == i)
			{
				ofs << id << " ";
				if (id.size() < 25) { ofs << std::string((25 - id.size()), ' '); }
				seqIdPositions[id] = ofs.tellp();
				ofs << std::string(resultSize, ' ');
				ofs << std::endl;
				linenumbers.push_back(counter);
			}
		}
		counter++;
	}

	ofs.close();
	ifs.close();

	std::fstream ofsUpdate(outputFile, std::ios::in | std::ios::out);
	if (!ofsUpdate) {
		std::cerr << "Error: Unable to open output file for updating: " << outputFile << std::endl;
		exit(1);
	}

	ifs.open(inputFile, std::ios::binary);
	ifs.seekg(dataStartPos, std::ios::beg);

	std::vector<std::tuple<int, int, int, int, uint64_t>> chosenFooter;
	for (const auto& entry : footer) {
		int startX, startY, width, height;
		uint64_t compressedSize;
		std::tie(startX, startY, width, height, compressedSize) = entry;
		for (auto i : linenumbers)
		{
			if (startX <= i && startX + width > i) {
				chosenFooter.push_back(entry);
				break;
			}
		}
	}

	for (const auto& entry : footer) {
		int startX, startY, width, height;
		uint64_t compressedSize;
		std::tie(startX, startY, width, height, compressedSize) = entry;

		Rectangle rect;
		rect.startX = startX;
		rect.startY = startY;
		rect.width = width;
		rect.height = height;
		rect.compressedData.resize(compressedSize);

		ifs.read(rect.compressedData.data(), compressedSize);
		if (!ifs) {
			std::cerr << "Error: Unable to read compressed data for rectangle at ("
				<< startX << ", " << startY << "). Expected size: " << compressedSize << std::endl;
			return;
		}


		auto it = std::find(chosenFooter.begin(), chosenFooter.end(), entry);
		if (it != chosenFooter.end()) {

			reversePreprocessing(rect, preprocessingType, sequenceIds);
			for (const auto& seq : rect.sequences) {
				for (auto i : chosenSequenceIds)
				{
					int inttt = 9;
					if (seq.id == i)
					{
						auto pos = seqIdPositions[seq.id];
						ofsUpdate.seekp(pos, std::ios::beg);
						ofsUpdate << std::string(seq.data.begin(), seq.data.end());
						std::streampos lenght = seq.data.size();
						seqIdPositions[seq.id] = ofsUpdate.tellp();
					}
				}
			}
		}
	}

	ofsUpdate.close();
	ifs.close();
}

void MSACompressor::decompressColumns(const std::string& inputFile, const std::string& outputFile, std::vector<int>& columnsIds, PreprocessingType preprocessingType) {
	std::ifstream ifs(inputFile, std::ios::binary);
	if (!ifs) {
		std::cerr << "Error: Unable to open input file: " << inputFile << std::endl;
		exit(1);
	}

	ifs.seekg(-static_cast<int>(3 * sizeof(uint64_t)), std::ios::end);
	uint64_t dataStartPos, sequenceIdsStartPos, footerStartPos;
	ifs.read(reinterpret_cast<char*>(&dataStartPos), sizeof(dataStartPos));
	ifs.read(reinterpret_cast<char*>(&sequenceIdsStartPos), sizeof(sequenceIdsStartPos));
	ifs.read(reinterpret_cast<char*>(&footerStartPos), sizeof(footerStartPos));

	ifs.seekg(footerStartPos, std::ios::beg);
	std::vector<std::tuple<int, int, int, int, uint64_t>> footer;
	while (true) {
		int startX, startY, width, height;
		uint64_t compressedSize;

		ifs.read(reinterpret_cast<char*>(&startX), sizeof(startX));
		if (startX == dataStartPos)break;
		ifs.read(reinterpret_cast<char*>(&startY), sizeof(startY));
		ifs.read(reinterpret_cast<char*>(&width), sizeof(width));
		ifs.read(reinterpret_cast<char*>(&height), sizeof(height));
		ifs.read(reinterpret_cast<char*>(&compressedSize), sizeof(compressedSize));

		footer.emplace_back(startX, startY, width, height, compressedSize);
	}

	std::ofstream ofs(outputFile);
	if (!ofs) {
		std::cerr << "Error: Unable to open output file: " << outputFile << std::endl;
		std::cerr << "Yol " << outputFile << std::endl;
		return;
	}
	ifs.clear();

	std::vector<std::string> sequenceIds;
	ifs.seekg(sequenceIdsStartPos, std::ios::beg);
	std::streampos lineStartPos = ofs.tellp();
	std::unordered_map<std::string, std::streampos> seqIdPositions;

	while (ifs.tellg() < footerStartPos) {
		uint16_t idLength;
		ifs.read(reinterpret_cast<char*>(&idLength), sizeof(idLength));
		std::string id(idLength, ' ');
		ifs.read(&id[0], idLength);
		sequenceIds.push_back(id);
		ofs << id << " ";
		if (id.size() < 25) { ofs << std::string((25 - id.size()), ' '); }
		seqIdPositions[id] = ofs.tellp();
		ofs << std::string(columnsIds.size() + 1, ' ');
		ofs << std::endl;
	}

	ofs.close();
	ifs.close();

	std::fstream ofsUpdate(outputFile, std::ios::in | std::ios::out);
	if (!ofsUpdate) {
		std::cerr << "Error: Unable to open output file for updating: " << outputFile << std::endl;
		return;
	}

	ifs.open(inputFile, std::ios::binary);
	ifs.seekg(dataStartPos, std::ios::beg);

	std::vector<std::tuple<int, int, int, int, uint64_t>> chosenFooter;
	for (const auto& entry : footer) {
		int startX, startY, width, height;
		uint64_t compressedSize;
		std::tie(startX, startY, width, height, compressedSize) = entry;
		for (auto i : columnsIds)
		{
			if (startY <= i && startY + height > i) {
				chosenFooter.push_back(entry);
				break;
			}
		}
	}

	for (const auto& entry : footer) {
		int startX, startY, width, height;
		uint64_t compressedSize;
		std::tie(startX, startY, width, height, compressedSize) = entry;

		Rectangle rect;
		rect.startX = startX;
		rect.startY = startY;
		rect.width = width;
		rect.height = height;
		rect.compressedData.resize(compressedSize);

		ifs.read(rect.compressedData.data(), compressedSize);
		if (!ifs) {
			std::cerr << "Error: Unable to read compressed data for rectangle at ("
				<< startX << ", " << startY << "). Expected size: " << compressedSize << std::endl;
			return;
		}

		auto it = std::find(chosenFooter.begin(), chosenFooter.end(), entry);
		if (it != chosenFooter.end()) {

			reversePreprocessing(rect, preprocessingType, sequenceIds);

			for (const auto& seq : rect.sequences) {

				for (auto i : columnsIds)
				{
					auto pos = seqIdPositions[seq.id];
					ofsUpdate.seekp(pos, std::ios::beg);
					if (startY <= i && startY + height > i) {
						ofsUpdate << seq.data[(i - startY)];
						seqIdPositions[seq.id] = ofsUpdate.tellp();

					}
				}

			}
		}
	}

	ofsUpdate.close();
	ifs.close();
}
