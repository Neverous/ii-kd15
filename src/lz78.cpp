/* 2015
 * Maciej Szeptuch
 */

#include <fstream>
#include <getopt.h>
#include <iostream>

#include <lz78/lz78.h>
#include <bitstream.h>
#include <dictionary.h>
#include <adaptive_huffman.h>
#include "log.h"
#include "common.h"

const char *VERSION = "0.1.0";
const char *HELP    = "Usage: lz78 [OPTION]... [FILE]\n\
Compress or uncompress FILE.\n\n\
-c, --stdout      write on standard output\n\
-b, --bitsize     dictionary bits (15-31, default=20)\n\
-d, --decompress  decompress\n\
-f, --force       force overwrite of output file\n\
-h, --help        give this help\n\
-q, --quiet       suppress all warnings\n\
-t, --test        test compressed file integrity\n\
-v, --verbose     verbose mode\n\
-V, --version     display version number\n\n\
With no FILE, or when FILE is -, read standard input.\n";
const char *SHORT_OPTIONS = "cb:dfhqtvV";
const struct option LONG_OPTIONS[] =
{
    {"stdout",      no_argument,        nullptr, 'c'},
    {"bitsize",     required_argument,  nullptr, 'b'},
    {"decompress",  no_argument,        nullptr, 'd'},
    {"force",       no_argument,        nullptr, 'f'},
    {"help",        no_argument,        nullptr, 'h'},
    {"quiet",       no_argument,        nullptr, 'q'},
    {"test",        no_argument,        nullptr, 't'},
    {"verbose",     no_argument,        nullptr, 'v'},
    {"version",     no_argument,        nullptr, 'V'},
    {nullptr, 0, nullptr, 0},
};

int main(int argc, char **argv)
{
    std::string file    = "";
    bool compress       = true;
    bool file_output    = true;
    bool overwrite      = false;
    bool quiet          = false;
    bool test           = false;
    bool verbose        = false;
    uint32_t bit_size   = 20;

    std::ifstream input_file;
    std::ofstream output_file;

    std::istream *input     = &std::cin;
    std::ostream *output    = &std::cout;

    Log log{std::cerr};

    int o;
    while((o = getopt_long(argc, argv, SHORT_OPTIONS, LONG_OPTIONS, 0)) != -1) switch(o)
    {
        case 'h': std::cout << HELP;
            return 0;

        case 'V': std::cout << "lz78 " << VERSION << "\n";
            return 0;

        case 'c':
            file_output = false;
            break;

        case 't':
            test = true;
            break;

        case 'd':
            compress = false;
            break;

        case 'f':
            overwrite = true;
            break;

        case 'q':
            quiet = true;
            break;

        case 'v':
            verbose = true;
            break;

        case 'b':
            bit_size = atoi(optarg);
            break;

        case '?':
        default: std::cerr << HELP;
            return 1;
    }

    if(optind < argc && std::string("-") != argv[optind])
        file = argv[optind];

    if(quiet)
        log.disable();

    else if(verbose)
        log.verbose();

    log(Log::DEBUG) << "running with options:"
                    << " stdout="       << !file_output
                    << " bitsize="      << bit_size
                    << " decompress="   << !compress
                    << " force="        << overwrite
                    << " quiet="        << quiet
                    << " test="         << test
                    << " verbose="      << verbose
                    << " file="         << (!file.empty() ? file : "STDIN");

    if(bit_size < 15 || bit_size > 31)
        throw std::runtime_error("Invalid bit_size for dictionary");

    if(!file.empty())
    {
        if(!file_exists(file))
            throw std::runtime_error("Input file doesn't exist");

        input_file.open(file, std::ifstream::in | std::ifstream::binary);
        input = &input_file;
    }

    if(file_output && !file.empty() && !test)
    {
        if(compress)
            file += ".lz78";

        else
        {
            if(!has_suffix(file, ".lz78"))
                throw std::runtime_error("Invalid file extension");

            file = file.substr(0, file.size() - 5);
        }

        if(!overwrite && file_exists(file))
            throw std::runtime_error("Output file already exists");

        output_file.open(file, std::ofstream::out | std::ofstream::binary);
        output = &output_file;
    }

    size_t dict_size = 1U << bit_size;
    if(compress)
    {
        //LZ78<Log &, Dictionary, BitOut> lz78{log, Dictionary{dict_size}, BitOut{*output}};
        LZ78<Log &, Dictionary, BitHuffOut> lz78{log, Dictionary{dict_size}, BitHuffOut{HuffOut{BitOut{*output}}}};
        if(test)
            lz78.simulate();

        BitIn bitstream{*input};
        log(log.INFO) << "Starting compression...";
        lz78.compress(bitstream);
        return !lz78.good();
    }

    else
    {
        LZ78<Log &, Dictionary, BitOut> lz78{log, Dictionary{dict_size}, BitOut{*output}};
        if(test)
            lz78.simulate();

        //BitIn bitstream{*input};
        BitHuffIn bitstream{HuffIn{BitIn{*input}}};
        log(log.INFO) << "Starting decompression...";
        lz78.decompress(bitstream);
        return !lz78.good();
    }

    return 0;
}
