#ifndef MSACOMPRESSOR_HPP
#define MSACOMPRESSOR_HPP

#include <vector>
#include <string>
#include "zstd.h"

/**
 * Enumeration to define different types of preprocessing methods.
 * These methods modify the sequences before compression by reducing gaps.
 */
enum PreprocessingType {
    NO_PREPROCESSING,                 // No preprocessing applied
    REDUCE_GAPS_A,                    // reducing gaps with method A
    REDUCE_GAPS_B,                    // reducing gaps with method B
    REDUCE_GAPS_C,                    // reducing gaps with method C
    REDUCE_GAPS_AND_LOWERCASE,        // Reduce gaps with method A and convert to lowercase
    REDUCE_GAPS_AND_UPPERCASE         // Reduce gaps with method A and convert to uppercase
};

/**
 * Structure to represent a sequence with an ID and its corresponding data.
 */
struct Sequence {
    std::string id;                   // Unique identifier for the sequence
    std::vector<char> data;           // Actual sequence data stored as a vector of characters
};

/**
 * Structure to represent one of the rectangular blocks into which MSA matrix is divided to.
 */
struct Rectangle {
    int startX;                             // X-coordinate of the top-left corner
    int startY;                             // Y-coordinate of the top-left corner
    int width;                              // Width of the rectangle
    int height;                             // Height of the rectangle
    std::vector<Sequence> sequences;        // List of sequences in this rectangle
    std::vector<char> compressedData;       // Compressed data for this rectangle
};

/**
 * Class responsible for compressing and decompressing MSA results using Zstandard.
 * Provides functions for both full and selective compression and decompression.
 */
class MSACompressor {
private:
    std::vector<Rectangle> rectangles;

    /**
     * Applies preprocessing to a given rectangle based on the specified preprocessing type.
     */
    void applyPreprocessing(Rectangle& rect, PreprocessingType preprocessingType);

    /**
     * Reverses the preprocessing previously applied to a given rectangle.
     */
    void reversePreprocessing(Rectangle& rect, PreprocessingType preprocessingType, const std::vector<std::string>& sequenceIds);

    /**
     * Compresses a specific rectangle using Zstandard compression with a specified level and preprocessing.
     */
    void compressRectangle(Rectangle& rect, int zstdLevel, PreprocessingType preprocessingType);

    /**
     * Compresses a specific rectangle using Zstandard compression with a specified level and preprocessing.
     * Instead of rows, it passes data to the compressor by columns.
     */
    void compressRectangleByColumn(Rectangle& rect, int zstdLevel, PreprocessingType preprocessingType);

    /**
     * Reduces gaps in a rectangle using the preprocessing method A.
     */
    void reduceGapsA(Rectangle& rect);

    /**
     * Reduces gaps in a rectangle using the preprocessing method B.
     */
    void reduceGapsB(Rectangle& rect);

    /**
     * Reduces gaps in a rectangle using the preprocessing method C.
     */
    void reduceGapsC(Rectangle& rect);

    /**
     * Reverses the effect of preprocessing method A.
     */
    void reverseGapsA(Rectangle& rect, const std::vector<std::string>& sequenceIds);

    /**
     * Reverses the effect of preprocessing method B.
     */
    void reverseGapsB(Rectangle& rect, const std::vector<std::string>& sequenceIds);

    /**
     * Reverses the effect of preprocessing method C.
     */
    void reverseGapsC(Rectangle& rect, const std::vector<std::string>& sequenceIds);

    /**
     * Reduces gaps and converts the sequence data to uppercase in the given rectangle.
     */
    void reduceGapsAndUpperCase(Rectangle& rect);

    /**
     * Reduces gaps and converts the sequence data to lowercase in the given rectangle.
     */
    void reduceGapsAndLowerCase(Rectangle& rect);

    /**
     * Decompresses a given rectangle.
     */
    void decompressRectangle(Rectangle& rect, const std::vector<std::string>& sequenceIds);

    /**
     * Decompresses a given rectangle for data compressed with function compressRectangleByColumn.
     */
    void decompressRectangleByColumn(Rectangle& rect, const std::vector<std::string>& sequenceIds);

    /**
     * Splits the sequences into rectangles based on the provided dimensions.
     */
    void splitSequencesIntoRectangles(const std::vector<Sequence>& sequences, int startX, std::vector<Rectangle>& rectangles, int A, int B);

public:

    MSACompressor();

    /**
     * Prints the instructions for the program usage.
     */
    void printUsage();

    /**
     * Compresses the input file, saving the result to the output file.
     */
    void compress(const std::string& inputFile, const std::string& outputFile, int zstdLevel, int A, int B, PreprocessingType preprocessingType);

    /**
     * Decompresses the input file and saves the results to the output file.
     */
    void decompress(const std::string& inputFile, const std::string& outputFile, PreprocessingType preprocesingType);

    /**
     * Decompresses selected sequences from the input file based on the provided sequence IDs.
     */
    void decompressSequences(const std::string& inputFile, const std::string& outputFile, std::vector<std::string>& chosenSequenceIds, PreprocessingType preprocessingType);

    /**
     * Decompresses selected columns from the input file based on the provided column numbers.
     */
    void decompressColumns(const std::string& inputFile, const std::string& outputFile, std::vector<int>& columnsIds, PreprocessingType preprocessingType);
};
#endif MSACOMPRESSOR_HPP