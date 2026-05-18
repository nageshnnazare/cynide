#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <lexer.h>

static void printUsage(const char *prog) {
  std::cerr
      << "Usage: " << prog
      << " <input.cy> [options]\n"
         "\n"
         "Compilation options:\n"
         "  -o, --output <name>   Output name (default: input basename)\n"

         "\n"
         "Debug options:\n"
         "  --emit-ir             Print llvm IR & write it out to .ll file\n"
         "  --emit-tokens         Print out the tokens from the lexer\n"
         "  --emit-ast            Print out the AST from the parser\n"
         "  --emit-sema           Print type-annotated report from "
         "semantic analysis\n"
         "\n"
         "  -h, --help            Show this message\n";
}

static bool readFile(const std::string &path, std::string &out) {
  std::ifstream in(path);
  if (!in)
    return false;

  std::ostringstream ss;
  ss << in.rdbuf();
  out = ss.str();
  return true;
}

static std::string basename(const std::string &path) {
  size_t slash = path.find_last_of("/\\");
  std::string name =
      (slash == std::string::npos) ? (path) : (path.substr(slash + 1));
  size_t dot = name.find_last_of('.');
  if (dot != std::string::npos)
    name = name.substr(0, dot);
  return name.empty() ? ("out") : (name);
}

static bool isCynideFile(const std::string &path) {
  size_t slash = path.find_last_of("/\\");
  std::string name =
      (slash == std::string::npos) ? (path) : (path.substr(slash + 1));
  size_t dot = name.find_last_of('.');
  if (dot != std::string::npos) {
    std::string ext = name.substr(dot, name.size());
    if (ext == ".cy")
      return true;
  }
  return false;
}

int main(int argc, char **argv) {
  std::vector<std::string> args(argv + 1, argv + argc);

  if (!args.empty() && (args[0] == "-h" || args[0] == "--help")) {
    printUsage(argv[0]);
    return 0;
  }

  if (args.empty()) {
    printUsage(argv[0]);
    return 1;
  }

  std::string inputPath = args[0];
  std::string outputBase;
  bool emitIR = false;
  bool emitTokens = false;
  bool emitAST = false;
  bool emitSema = false;

  for (size_t i = 1; i < args.size(); ++i) {
    if ((args[i] == "--output" || args[i] == "-o") && (i + 1) < args.size()) {
      outputBase = args[++i];
    } else if (args[i] == "--emit-ir") {
      emitIR = true;
    } else if (args[i] == "--emit-tokens") {
      emitTokens = true;
    } else if (args[i] == "--emit-ast") {
      emitAST = true;
    } else if (args[i] == "--emit-sema") {
      emitSema = true;
    } else {
      std::cerr << "Unknown argument: " << args[i] << "\n";
      printUsage(argv[0]);
      return 1;
    }
  }

  if (outputBase.empty())
    outputBase = basename(inputPath);

  std::string source;
  if (!readFile(inputPath, source)) {
    std::cerr << "Error: " << "cannot read input file '" << inputPath << "'.\n";
    return 1;
  }
  if (!isCynideFile(inputPath)) {
    std::cerr << "Error: " << "input file '" << inputPath
              << "' is not a cynide file.\n";
    return 1;
  }

  /* ---- Stage 1 : Lexer ---- */
  Lexer lexer(std::move(source));
  std::vector<Token> tokens = lexer.tokenize();
  if (lexer.hasError()) {
    std::cerr << "Lexer error: " << lexer.errorMessage() << "\n";
    return 1;
  }
  if (emitTokens) {
    Lexer::dumpTokens(tokens);
  }

  /* ---- Stage 2 : Parser ---- */

  /* ---- Stage 3 : Semantic Analysis ---- */

  /* ---- Stage 4 : Code Generation ---- */

  /* ---- Stage 5 : Object Emission + Linking ---- */

  return 0;
}