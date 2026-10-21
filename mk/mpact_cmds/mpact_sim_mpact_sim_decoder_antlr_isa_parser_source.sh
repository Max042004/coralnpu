#!/bin/bash
# Code generation step for @mpact-sim//mpact/sim/decoder:antlr_isa_parser_source
set -euo pipefail
rm -rf build/mpact/tmp/mpact_sim_mpact_sim_decoder_antlr_isa_parser_source && mkdir -p build/mpact/tmp/mpact_sim_mpact_sim_decoder_antlr_isa_parser_source build/mpact/gen/mpact-sim/mpact/sim/decoder
java -jar "$ANTLR_JAR" vendor/mpact-sim/mpact/sim/decoder/InstructionSet.g4 -no-visitor -listener -Dlanguage=Cpp -package sim -o build/mpact/tmp/mpact_sim_mpact_sim_decoder_antlr_isa_parser_source -Xexact-output-dir;
sed -i'.bak'  '/namespace sim {/ a\
namespace machine_description {\
namespace instruction_set {\
namespace generated {' build/mpact/tmp/mpact_sim_mpact_sim_decoder_antlr_isa_parser_source/InstructionSetParser.h;
sed -i'.bak'  '/}  \/\/ namespace sim/i\
}  \/\/ namespace machine_description
' build/mpact/tmp/mpact_sim_mpact_sim_decoder_antlr_isa_parser_source/InstructionSetParser.h;
sed -i'.bak'  '/}  \/\/ namespace sim/i\
}  \/\/ namespace instruction_set
' build/mpact/tmp/mpact_sim_mpact_sim_decoder_antlr_isa_parser_source/InstructionSetParser.h;
sed -i'.bak'  '/}  \/\/ namespace sim/i\
}  \/\/ namespace generated
' build/mpact/tmp/mpact_sim_mpact_sim_decoder_antlr_isa_parser_source/InstructionSetParser.h;
sed -i'.bak'  's/using namespace sim;/using namespace sim::machine_description::instruction_set::generated;/' build/mpact/tmp/mpact_sim_mpact_sim_decoder_antlr_isa_parser_source/InstructionSetParser.cpp;
sed -i'.bak'  's/using namespace sim;/using namespace sim::machine_description::instruction_set::generated;/' build/mpact/tmp/mpact_sim_mpact_sim_decoder_antlr_isa_parser_source/InstructionSetBaseListener.cpp;
sed -i'.bak'  's/using namespace sim;/using namespace sim::machine_description::instruction_set::generated;/' build/mpact/tmp/mpact_sim_mpact_sim_decoder_antlr_isa_parser_source/InstructionSetListener.cpp;
sed -i'.bak'  '/namespace sim {/ a\
namespace machine_description {\
namespace instruction_set {\
namespace generated {' build/mpact/tmp/mpact_sim_mpact_sim_decoder_antlr_isa_parser_source/InstructionSetBaseListener.h;
sed -i'.bak'  '/}  \/\/ namespace sim/i\
}  \/\/ namespace machine_description
' build/mpact/tmp/mpact_sim_mpact_sim_decoder_antlr_isa_parser_source/InstructionSetBaseListener.h;
sed -i'.bak'  '/}  \/\/ namespace sim/i\
}  \/\/ namespace instruction_set
' build/mpact/tmp/mpact_sim_mpact_sim_decoder_antlr_isa_parser_source/InstructionSetBaseListener.h;
sed -i'.bak'  '/}  \/\/ namespace sim/i\
}  \/\/ namespace generated
' build/mpact/tmp/mpact_sim_mpact_sim_decoder_antlr_isa_parser_source/InstructionSetBaseListener.h;
sed -i'.bak'  '/namespace sim {/ a\
namespace machine_description {\
namespace instruction_set {\
namespace generated {' build/mpact/tmp/mpact_sim_mpact_sim_decoder_antlr_isa_parser_source/InstructionSetListener.h;
sed -i'.bak'  '/}  \/\/ namespace sim/i\
}  \/\/ namespace machine_description
' build/mpact/tmp/mpact_sim_mpact_sim_decoder_antlr_isa_parser_source/InstructionSetListener.h;
sed -i'.bak'  '/}  \/\/ namespace sim/i\
}  \/\/ namespace instruction_set
' build/mpact/tmp/mpact_sim_mpact_sim_decoder_antlr_isa_parser_source/InstructionSetListener.h;
sed -i'.bak'  '/}  \/\/ namespace sim/i\
}  \/\/ namespace generated
' build/mpact/tmp/mpact_sim_mpact_sim_decoder_antlr_isa_parser_source/InstructionSetListener.h;
sed -i'.bak' '/antlr4-runtime.h/s//antlr4-runtime\/antlr4-runtime.h/' build/mpact/tmp/mpact_sim_mpact_sim_decoder_antlr_isa_parser_source/InstructionSetParser.h;
grep -q '^#pragma once' build/mpact/tmp/mpact_sim_mpact_sim_decoder_antlr_isa_parser_source/InstructionSetParser.h && echo '
#endif  // ANTLR4_GEN_INSTRUCTIONSETPARSER_H
' >> build/mpact/tmp/mpact_sim_mpact_sim_decoder_antlr_isa_parser_source/InstructionSetParser.h;
sed -i'.bak' '/#pragma once/s//#ifndef ANTLR4_GEN_INSTRUCTIONSETPARSER_H\n#define ANTLR4_GEN_INSTRUCTIONSETPARSER_H/' build/mpact/tmp/mpact_sim_mpact_sim_decoder_antlr_isa_parser_source/InstructionSetParser.h;
sed -i'.bak' '/antlr4-runtime.h/s//antlr4-runtime\/antlr4-runtime.h/' build/mpact/tmp/mpact_sim_mpact_sim_decoder_antlr_isa_parser_source/InstructionSetParser.cpp;
grep -q '^#pragma once' build/mpact/tmp/mpact_sim_mpact_sim_decoder_antlr_isa_parser_source/InstructionSetParser.cpp && echo '
#endif  // ANTLR4_GEN_INSTRUCTIONSETPARSER_CPP
' >> build/mpact/tmp/mpact_sim_mpact_sim_decoder_antlr_isa_parser_source/InstructionSetParser.cpp;
sed -i'.bak' '/#pragma once/s//#ifndef ANTLR4_GEN_INSTRUCTIONSETPARSER_CPP\n#define ANTLR4_GEN_INSTRUCTIONSETPARSER_CPP/' build/mpact/tmp/mpact_sim_mpact_sim_decoder_antlr_isa_parser_source/InstructionSetParser.cpp;
sed -i'.bak' '/antlr4-runtime.h/s//antlr4-runtime\/antlr4-runtime.h/' build/mpact/tmp/mpact_sim_mpact_sim_decoder_antlr_isa_parser_source/InstructionSetBaseListener.cpp;
grep -q '^#pragma once' build/mpact/tmp/mpact_sim_mpact_sim_decoder_antlr_isa_parser_source/InstructionSetBaseListener.cpp && echo '
#endif  // ANTLR4_GEN_INSTRUCTIONSETBASELISTENER_CPP
' >> build/mpact/tmp/mpact_sim_mpact_sim_decoder_antlr_isa_parser_source/InstructionSetBaseListener.cpp;
sed -i'.bak' '/#pragma once/s//#ifndef ANTLR4_GEN_INSTRUCTIONSETBASELISTENER_CPP\n#define ANTLR4_GEN_INSTRUCTIONSETBASELISTENER_CPP/' build/mpact/tmp/mpact_sim_mpact_sim_decoder_antlr_isa_parser_source/InstructionSetBaseListener.cpp;
sed -i'.bak' '/antlr4-runtime.h/s//antlr4-runtime\/antlr4-runtime.h/' build/mpact/tmp/mpact_sim_mpact_sim_decoder_antlr_isa_parser_source/InstructionSetListener.cpp;
grep -q '^#pragma once' build/mpact/tmp/mpact_sim_mpact_sim_decoder_antlr_isa_parser_source/InstructionSetListener.cpp && echo '
#endif  // ANTLR4_GEN_INSTRUCTIONSETLISTENER_CPP
' >> build/mpact/tmp/mpact_sim_mpact_sim_decoder_antlr_isa_parser_source/InstructionSetListener.cpp;
sed -i'.bak' '/#pragma once/s//#ifndef ANTLR4_GEN_INSTRUCTIONSETLISTENER_CPP\n#define ANTLR4_GEN_INSTRUCTIONSETLISTENER_CPP/' build/mpact/tmp/mpact_sim_mpact_sim_decoder_antlr_isa_parser_source/InstructionSetListener.cpp;
sed -i'.bak' '/antlr4-runtime.h/s//antlr4-runtime\/antlr4-runtime.h/' build/mpact/tmp/mpact_sim_mpact_sim_decoder_antlr_isa_parser_source/InstructionSetBaseListener.h;
grep -q '^#pragma once' build/mpact/tmp/mpact_sim_mpact_sim_decoder_antlr_isa_parser_source/InstructionSetBaseListener.h && echo '
#endif  // ANTLR4_GEN_INSTRUCTIONSETBASELISTENER_H
' >> build/mpact/tmp/mpact_sim_mpact_sim_decoder_antlr_isa_parser_source/InstructionSetBaseListener.h;
sed -i'.bak' '/#pragma once/s//#ifndef ANTLR4_GEN_INSTRUCTIONSETBASELISTENER_H\n#define ANTLR4_GEN_INSTRUCTIONSETBASELISTENER_H/' build/mpact/tmp/mpact_sim_mpact_sim_decoder_antlr_isa_parser_source/InstructionSetBaseListener.h;
sed -i'.bak' '/antlr4-runtime.h/s//antlr4-runtime\/antlr4-runtime.h/' build/mpact/tmp/mpact_sim_mpact_sim_decoder_antlr_isa_parser_source/InstructionSetListener.h;
grep -q '^#pragma once' build/mpact/tmp/mpact_sim_mpact_sim_decoder_antlr_isa_parser_source/InstructionSetListener.h && echo '
#endif  // ANTLR4_GEN_INSTRUCTIONSETLISTENER_H
' >> build/mpact/tmp/mpact_sim_mpact_sim_decoder_antlr_isa_parser_source/InstructionSetListener.h;
sed -i'.bak' '/#pragma once/s//#ifndef ANTLR4_GEN_INSTRUCTIONSETLISTENER_H\n#define ANTLR4_GEN_INSTRUCTIONSETLISTENER_H/' build/mpact/tmp/mpact_sim_mpact_sim_decoder_antlr_isa_parser_source/InstructionSetListener.h
cp build/mpact/tmp/mpact_sim_mpact_sim_decoder_antlr_isa_parser_source/InstructionSetParser.h build/mpact/gen/mpact-sim/mpact/sim/decoder/InstructionSetParser.h
cp build/mpact/tmp/mpact_sim_mpact_sim_decoder_antlr_isa_parser_source/InstructionSetParser.cpp build/mpact/gen/mpact-sim/mpact/sim/decoder/InstructionSetParser.cpp
cp build/mpact/tmp/mpact_sim_mpact_sim_decoder_antlr_isa_parser_source/InstructionSetBaseListener.cpp build/mpact/gen/mpact-sim/mpact/sim/decoder/InstructionSetBaseListener.cpp
cp build/mpact/tmp/mpact_sim_mpact_sim_decoder_antlr_isa_parser_source/InstructionSetListener.cpp build/mpact/gen/mpact-sim/mpact/sim/decoder/InstructionSetListener.cpp
cp build/mpact/tmp/mpact_sim_mpact_sim_decoder_antlr_isa_parser_source/InstructionSetBaseListener.h build/mpact/gen/mpact-sim/mpact/sim/decoder/InstructionSetBaseListener.h
cp build/mpact/tmp/mpact_sim_mpact_sim_decoder_antlr_isa_parser_source/InstructionSetListener.h build/mpact/gen/mpact-sim/mpact/sim/decoder/InstructionSetListener.h
