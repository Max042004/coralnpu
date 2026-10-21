#!/bin/bash
# Code generation step for @mpact-sim//mpact/sim/decoder:bin_format_parser_source
set -euo pipefail
rm -rf build/mpact/tmp/mpact_sim_mpact_sim_decoder_bin_format_parser_source && mkdir -p build/mpact/tmp/mpact_sim_mpact_sim_decoder_bin_format_parser_source build/mpact/gen/mpact-sim/mpact/sim/decoder
java -jar "$ANTLR_JAR" vendor/mpact-sim/mpact/sim/decoder/BinFormat.g4 -no-visitor -listener -Dlanguage=Cpp -package mpact -o build/mpact/tmp/mpact_sim_mpact_sim_decoder_bin_format_parser_source -Xexact-output-dir;
sed -i'.bak'  '/namespace mpact {/ a\
namespace sim {\
namespace decoder {\
namespace bin_format {\
namespace generated {' build/mpact/tmp/mpact_sim_mpact_sim_decoder_bin_format_parser_source/BinFormatParser.h;
sed -i'.bak'  '/}  \/\/ namespace mpact/i\
}  \/\/ namespace sim
' build/mpact/tmp/mpact_sim_mpact_sim_decoder_bin_format_parser_source/BinFormatParser.h;
sed -i'.bak'  '/}  \/\/ namespace mpact/i\
}  \/\/ namespace decoder
' build/mpact/tmp/mpact_sim_mpact_sim_decoder_bin_format_parser_source/BinFormatParser.h;
sed -i'.bak'  '/}  \/\/ namespace mpact/i\
}  \/\/ namespace bin_format
' build/mpact/tmp/mpact_sim_mpact_sim_decoder_bin_format_parser_source/BinFormatParser.h;
sed -i'.bak'  '/}  \/\/ namespace mpact/i\
}  \/\/ namespace generated
' build/mpact/tmp/mpact_sim_mpact_sim_decoder_bin_format_parser_source/BinFormatParser.h;
sed -i'.bak'  's/using namespace mpact;/using namespace mpact::sim::decoder::bin_format::generated;/' build/mpact/tmp/mpact_sim_mpact_sim_decoder_bin_format_parser_source/BinFormatParser.cpp;
sed -i'.bak'  's/using namespace mpact;/using namespace mpact::sim::decoder::bin_format::generated;/' build/mpact/tmp/mpact_sim_mpact_sim_decoder_bin_format_parser_source/BinFormatBaseListener.cpp;
sed -i'.bak'  's/using namespace mpact;/using namespace mpact::sim::decoder::bin_format::generated;/' build/mpact/tmp/mpact_sim_mpact_sim_decoder_bin_format_parser_source/BinFormatListener.cpp;
sed -i'.bak'  '/namespace mpact {/ a\
namespace sim {\
namespace decoder {\
namespace bin_format {\
namespace generated {' build/mpact/tmp/mpact_sim_mpact_sim_decoder_bin_format_parser_source/BinFormatBaseListener.h;
sed -i'.bak'  '/}  \/\/ namespace mpact/i\
}  \/\/ namespace sim
' build/mpact/tmp/mpact_sim_mpact_sim_decoder_bin_format_parser_source/BinFormatBaseListener.h;
sed -i'.bak'  '/}  \/\/ namespace mpact/i\
}  \/\/ namespace decoder
' build/mpact/tmp/mpact_sim_mpact_sim_decoder_bin_format_parser_source/BinFormatBaseListener.h;
sed -i'.bak'  '/}  \/\/ namespace mpact/i\
}  \/\/ namespace bin_format
' build/mpact/tmp/mpact_sim_mpact_sim_decoder_bin_format_parser_source/BinFormatBaseListener.h;
sed -i'.bak'  '/}  \/\/ namespace mpact/i\
}  \/\/ namespace generated
' build/mpact/tmp/mpact_sim_mpact_sim_decoder_bin_format_parser_source/BinFormatBaseListener.h;
sed -i'.bak'  '/namespace mpact {/ a\
namespace sim {\
namespace decoder {\
namespace bin_format {\
namespace generated {' build/mpact/tmp/mpact_sim_mpact_sim_decoder_bin_format_parser_source/BinFormatListener.h;
sed -i'.bak'  '/}  \/\/ namespace mpact/i\
}  \/\/ namespace sim
' build/mpact/tmp/mpact_sim_mpact_sim_decoder_bin_format_parser_source/BinFormatListener.h;
sed -i'.bak'  '/}  \/\/ namespace mpact/i\
}  \/\/ namespace decoder
' build/mpact/tmp/mpact_sim_mpact_sim_decoder_bin_format_parser_source/BinFormatListener.h;
sed -i'.bak'  '/}  \/\/ namespace mpact/i\
}  \/\/ namespace bin_format
' build/mpact/tmp/mpact_sim_mpact_sim_decoder_bin_format_parser_source/BinFormatListener.h;
sed -i'.bak'  '/}  \/\/ namespace mpact/i\
}  \/\/ namespace generated
' build/mpact/tmp/mpact_sim_mpact_sim_decoder_bin_format_parser_source/BinFormatListener.h;
sed -i'.bak' '/antlr4-runtime.h/s//antlr4-runtime\/antlr4-runtime.h/' build/mpact/tmp/mpact_sim_mpact_sim_decoder_bin_format_parser_source/BinFormatParser.h;
grep -q '^#pragma once' build/mpact/tmp/mpact_sim_mpact_sim_decoder_bin_format_parser_source/BinFormatParser.h && echo '
#endif  // ANTLR4_GEN_BINFORMATPARSER_H
' >> build/mpact/tmp/mpact_sim_mpact_sim_decoder_bin_format_parser_source/BinFormatParser.h;
sed -i'.bak' '/#pragma once/s//#ifndef ANTLR4_GEN_BINFORMATPARSER_H\n#define ANTLR4_GEN_BINFORMATPARSER_H/' build/mpact/tmp/mpact_sim_mpact_sim_decoder_bin_format_parser_source/BinFormatParser.h;
sed -i'.bak' '/antlr4-runtime.h/s//antlr4-runtime\/antlr4-runtime.h/' build/mpact/tmp/mpact_sim_mpact_sim_decoder_bin_format_parser_source/BinFormatParser.cpp;
grep -q '^#pragma once' build/mpact/tmp/mpact_sim_mpact_sim_decoder_bin_format_parser_source/BinFormatParser.cpp && echo '
#endif  // ANTLR4_GEN_BINFORMATPARSER_CPP
' >> build/mpact/tmp/mpact_sim_mpact_sim_decoder_bin_format_parser_source/BinFormatParser.cpp;
sed -i'.bak' '/#pragma once/s//#ifndef ANTLR4_GEN_BINFORMATPARSER_CPP\n#define ANTLR4_GEN_BINFORMATPARSER_CPP/' build/mpact/tmp/mpact_sim_mpact_sim_decoder_bin_format_parser_source/BinFormatParser.cpp;
sed -i'.bak' '/antlr4-runtime.h/s//antlr4-runtime\/antlr4-runtime.h/' build/mpact/tmp/mpact_sim_mpact_sim_decoder_bin_format_parser_source/BinFormatBaseListener.cpp;
grep -q '^#pragma once' build/mpact/tmp/mpact_sim_mpact_sim_decoder_bin_format_parser_source/BinFormatBaseListener.cpp && echo '
#endif  // ANTLR4_GEN_BINFORMATBASELISTENER_CPP
' >> build/mpact/tmp/mpact_sim_mpact_sim_decoder_bin_format_parser_source/BinFormatBaseListener.cpp;
sed -i'.bak' '/#pragma once/s//#ifndef ANTLR4_GEN_BINFORMATBASELISTENER_CPP\n#define ANTLR4_GEN_BINFORMATBASELISTENER_CPP/' build/mpact/tmp/mpact_sim_mpact_sim_decoder_bin_format_parser_source/BinFormatBaseListener.cpp;
sed -i'.bak' '/antlr4-runtime.h/s//antlr4-runtime\/antlr4-runtime.h/' build/mpact/tmp/mpact_sim_mpact_sim_decoder_bin_format_parser_source/BinFormatListener.cpp;
grep -q '^#pragma once' build/mpact/tmp/mpact_sim_mpact_sim_decoder_bin_format_parser_source/BinFormatListener.cpp && echo '
#endif  // ANTLR4_GEN_BINFORMATLISTENER_CPP
' >> build/mpact/tmp/mpact_sim_mpact_sim_decoder_bin_format_parser_source/BinFormatListener.cpp;
sed -i'.bak' '/#pragma once/s//#ifndef ANTLR4_GEN_BINFORMATLISTENER_CPP\n#define ANTLR4_GEN_BINFORMATLISTENER_CPP/' build/mpact/tmp/mpact_sim_mpact_sim_decoder_bin_format_parser_source/BinFormatListener.cpp;
sed -i'.bak' '/antlr4-runtime.h/s//antlr4-runtime\/antlr4-runtime.h/' build/mpact/tmp/mpact_sim_mpact_sim_decoder_bin_format_parser_source/BinFormatBaseListener.h;
grep -q '^#pragma once' build/mpact/tmp/mpact_sim_mpact_sim_decoder_bin_format_parser_source/BinFormatBaseListener.h && echo '
#endif  // ANTLR4_GEN_BINFORMATBASELISTENER_H
' >> build/mpact/tmp/mpact_sim_mpact_sim_decoder_bin_format_parser_source/BinFormatBaseListener.h;
sed -i'.bak' '/#pragma once/s//#ifndef ANTLR4_GEN_BINFORMATBASELISTENER_H\n#define ANTLR4_GEN_BINFORMATBASELISTENER_H/' build/mpact/tmp/mpact_sim_mpact_sim_decoder_bin_format_parser_source/BinFormatBaseListener.h;
sed -i'.bak' '/antlr4-runtime.h/s//antlr4-runtime\/antlr4-runtime.h/' build/mpact/tmp/mpact_sim_mpact_sim_decoder_bin_format_parser_source/BinFormatListener.h;
grep -q '^#pragma once' build/mpact/tmp/mpact_sim_mpact_sim_decoder_bin_format_parser_source/BinFormatListener.h && echo '
#endif  // ANTLR4_GEN_BINFORMATLISTENER_H
' >> build/mpact/tmp/mpact_sim_mpact_sim_decoder_bin_format_parser_source/BinFormatListener.h;
sed -i'.bak' '/#pragma once/s//#ifndef ANTLR4_GEN_BINFORMATLISTENER_H\n#define ANTLR4_GEN_BINFORMATLISTENER_H/' build/mpact/tmp/mpact_sim_mpact_sim_decoder_bin_format_parser_source/BinFormatListener.h
cp build/mpact/tmp/mpact_sim_mpact_sim_decoder_bin_format_parser_source/BinFormatParser.h build/mpact/gen/mpact-sim/mpact/sim/decoder/BinFormatParser.h
cp build/mpact/tmp/mpact_sim_mpact_sim_decoder_bin_format_parser_source/BinFormatParser.cpp build/mpact/gen/mpact-sim/mpact/sim/decoder/BinFormatParser.cpp
cp build/mpact/tmp/mpact_sim_mpact_sim_decoder_bin_format_parser_source/BinFormatBaseListener.cpp build/mpact/gen/mpact-sim/mpact/sim/decoder/BinFormatBaseListener.cpp
cp build/mpact/tmp/mpact_sim_mpact_sim_decoder_bin_format_parser_source/BinFormatListener.cpp build/mpact/gen/mpact-sim/mpact/sim/decoder/BinFormatListener.cpp
cp build/mpact/tmp/mpact_sim_mpact_sim_decoder_bin_format_parser_source/BinFormatBaseListener.h build/mpact/gen/mpact-sim/mpact/sim/decoder/BinFormatBaseListener.h
cp build/mpact/tmp/mpact_sim_mpact_sim_decoder_bin_format_parser_source/BinFormatListener.h build/mpact/gen/mpact-sim/mpact/sim/decoder/BinFormatListener.h
