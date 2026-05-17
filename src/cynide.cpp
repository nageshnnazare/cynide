#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <lexer.h>

static void printUsage(const char *prog) {
  std::cerr << "Usage: " << prog
            << " <input.cy> [options]\n"
               "\n"
               "  -o, --output <name>   Output name (default: input basename)\n"
               "  -h, --help            Show this message.\n";
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

  if (!isCynideFile(inputPath)) {
    std::cerr << "Error: " << inputPath << " is not a cynide file\n";
  }

  for (size_t i = 1; i < args.size(); ++i) {
    if ((args[i] == "--output" || args[i] == "-o") && (i + 1) < args.size()) {
      outputBase = args[++i];
    } else {
      std::cerr << "Unknown argument: " << args[i] << "\n";
      printUsage(argv[0]);
      return 1;
    }
  }

  if (outputBase.empty())
    outputBase = basename(inputPath);

  std::cout << outputBase << "\n";

  return 0;
}