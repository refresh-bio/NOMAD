#include "console.h"

#include <string>
#include <fstream>
#include <iostream>

using namespace std;


void Console::printUsage() {
	
	cerr << "\n"
		<< "Usage:\n"
		<< "compactors [options] <fastq_list> <anchors_tsv> <compactors_tsv>\n\n"

		<< "Positional parameters:\n"
		<< "   fastq_list		   - input file with a list of FASTQ/FASTA files to be queried for anchors (one per line)\n"
		<< "   anchors_tsv		   - input tsv file with anchors\n"
		<< "   compactors_tsv      - output tsv with compactors\n"
		<< "Options:\n"

		<< "  " << PARAM_INPUT_FORMAT << " <fasta|fastq> - input format (default: " << to_string(inputFormat) << ")\n"
		<< "  " << PARAM_NUM_KMERS << " <int> - number of kmers in a compactor segment (default: " << params.numKmers << ", does not include an anchor/seed)\n"
		<< "  " << PARAM_KMER_LEN << " <int> - length of kmers in a compactor segment (default: " << params.kmerLen << ", max: 31)\n"
		<< "  " << PARAM_EPSILON << " <real> - sequencing error (default: " << params.epsilon << ")\n"
		<< "  " << PARAM_BETA << " <real> - beta parameter for active set generation (default: " << params.beta << ")\n"
		<< "  " << PARAM_LOWER_BOUND << " <int> - minimum kmer abundance to add it to an active set (default: " << params.lowerBound << ")\n"
		<< "  " << PARAM_MAX_MISMATCH << " <int> - maximum mismatch count for compactor candidates (default: " << params.maxMismatch << ")\n"
		<< "  " << SWITCH_ALL_ANCHORS << " - find all anchors' occurences in a read, not just the first one (default: off)\n\n"

		<< "  " << SWITCH_NO_EXTENSION << " - disable recursive extension (default: enabled)\n"
		<< "  " << PARAM_MAX_LENGTH << " <int> - maximum compactor length in bases (used only with recursion; default: " << params.maxLen << ")\n"
		<< "  " << PARAM_MIN_EXTENDER_SPECIFICITY << " <real> - minimum extender specificity for current anchor to allow extensions (default: " << params.minExtenderSpecificity << ")\n"
		<< "  " << PARAM_NUM_EXTENDERS << " <int> - number of extender candidates to be verified starting from the very end of compactor (default: " << params.numExtenders << ")\n"
		<< "  " << PARAM_EXTENDERS_SHIFT << " <int> - shift in bp between extender candidates to be verified (default: " << params.extendersShift << ")\n"
		<< "  " << PARAM_MAX_ANCHOR_COMPACTORS << " <int> - maximum number of compactors that can originate from an anchor (default:" << params.maxAnchorCompactors << ")\n"
		<< "  " << PARAM_MAX_CHILD_COMPACTORS << " <int> - maximum number of child compactors produced at each extension step (default:" << params.maxChildCompactors << ")\n"
		<< "  " << SWITCH_EXTEND_ALL << " - if multiple compactors for a given anchor end with same extender, all are extended (default: off)\n\n"

		<< "  " << PARAM_OUT_FASTA << " <name> - name of the optional compactor FASTA (not generated by default)\n"
		<< "  " << SWITCH_NO_SUBCOMPACTORS << " - do not include subcompactors in the output TSV (default: off)\n"
		<< "  " << SWITCH_CUMULATED_STATS << " - include columns with cumulated stats in the output TSV (default: off)\n"
		<< "  " << SWITCH_INDEPENDENT_OUTPUTS << " - run compactors independently on input FASTQ files (default: off)\n"
		<< "\n"
		<< "  " << PARAM_NUM_THREADS << " <int> - number of threads\n"
		<< "  " << PARAM_READS_BUFFER << " <int> - size of the read buffer in GB (default: " << readsBufferGb << ")\n"
		<< "  " << SWITCH_KEEP_TEMP << " - keep temporary files after the analysis, use previously generated temporary files (if exist) during the analysis (default: off)\n"
		<< "  " << PARAM_LOG << " <name> - name of the optional log file (not generated by default)\n";
	
}

bool Console::parse(int argc, char** argv) {
	
	if (argc == 1) {
		return false;
	}
	
	vector<string> args;
	for (int i = 1; i < argc; ++i) {
		args.emplace_back(argv[i]);
	}

	std::string input_format_str;
	if (findOption(args, PARAM_INPUT_FORMAT, input_format_str)) {
		inputFormat = input_format_from_string(input_format_str);

		if (inputFormat == input_format_t::unknown)
		{
			std::cerr << "Error: unknown format " << input_format_str << "\n";
			exit(1);
		}
	}

	findOption(args, PARAM_NUM_KMERS, params.numKmers);
	params.allAnchors = findSwitch(args, SWITCH_ALL_ANCHORS);

	findOption(args, PARAM_KMER_LEN, params.kmerLen);
	findOption(args, PARAM_POLY_THRSEHOLD, params.polyThreshold);

	findOption(args, PARAM_EPSILON, params.epsilon);
	findOption(args, PARAM_BETA, params.beta);
	findOption(args, PARAM_LOWER_BOUND, params.lowerBound);
	findOption(args, PARAM_MAX_MISMATCH, params.maxMismatch);
	params.useEditDistance = findSwitch(args, SWITCH_EDIT_DISTANCE);

	// extension parameters
	bool noRecursion = findSwitch(args, SWITCH_NO_EXTENSION);
	params.useRecursion = !noRecursion;
	findOption(args, PARAM_MAX_LENGTH, params.maxLen);
	findOption(args, PARAM_MIN_EXTENDER_SPECIFICITY, params.minExtenderSpecificity);
	findOption(args, PARAM_NUM_EXTENDERS, params.numExtenders);
	findOption(args, PARAM_EXTENDERS_SHIFT, params.extendersShift);

	findOption(args, PARAM_MAX_CHILD_COMPACTORS, params.maxChildCompactors);
	findOption(args, PARAM_MAX_ANCHOR_COMPACTORS, params.maxAnchorCompactors);
	params.extendAll = findSwitch(args, SWITCH_EXTEND_ALL);
	params.newAcceptanceRule = findSwitch(args, SWITCH_NEW_ACCEPTANCE_RULE);

	// general configuration
	findOption(args, PARAM_NUM_THREADS, numThreads);
	findOption(args, PARAM_OUT_FASTA, outputFasta);
	findOption(args, PARAM_READS_BUFFER, readsBufferGb);
	findOption(args, PARAM_ANCHORS_BATCH, anchorsBatchSize);
	keepTemp = findSwitch(args, SWITCH_KEEP_TEMP);
	noSubcompactors = findSwitch(args, SWITCH_NO_SUBCOMPACTORS);
	cumulatedStats = findSwitch(args, SWITCH_CUMULATED_STATS);
	independentOutputs = findSwitch(args, SWITCH_INDEPENDENT_OUTPUTS);


	verbose = findSwitch(args, SWITCH_VERBOSE);
	findOption(args, PARAM_LOG, logFile);


	if (args.size() == 3) {
		// read mode
		ifstream ifs(args[0]);
		string line;
		while (getline(ifs, line)) {
			if (line != "" && line != "." && line != "..") {
				sampleFastqs.emplace_back(line);
			}
		}

		anchorsTsv = args[1];
		outputTsv = args[2];
		return true;
	}

	return false;
}
