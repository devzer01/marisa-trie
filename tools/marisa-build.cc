#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

#include <marisa.h>

#include "cmdopt.h"

namespace {

int param_num_tries = MARISA_DEFAULT_NUM_TRIES;
marisa::TailMode param_tail_mode = MARISA_DEFAULT_TAIL;
marisa::NodeOrder param_node_order = MARISA_DEFAULT_ORDER;
const char *output_filename = NULL;

void print_help(const char *cmd) {
  std::cerr << "Usage: " << cmd << " [OPTION]... [FILE]...\n\n"
      "Options:\n"
      "  -n, --num-tries=[N]  limits the number of tries to N"
      " (default: 3)\n"
      "  -t, --text-tail      build a dictionary with text TAIL (default)\n"
      "  -b, --binary-tail    build a dictionary with binary TAIL\n"
      "  -w, --weight-order   arranges siblings in weight order (default)\n"
      "  -l, --label-order    arranges siblings in label order\n"
      "  -o, --output=[FILE]  write tries to FILE (default: stdout)\n"
      "  -h, --help           print this help\n"
      << std::endl;
}

void read_keys(std::istream &input, marisa::Keyset *keyset) {
  std::string line;
  while (std::getline(input, line)) {
    const std::string::size_type delim_pos = line.find_last_of('\t');
    float weight = 1.0F;
    if (delim_pos != line.npos) {
      char *end_of_value;
      weight = (float)std::strtod(&line[delim_pos + 1], &end_of_value);
      if (*end_of_value == '\0') {
        line.resize(delim_pos);
      }
    }
    keyset->push_back(line.c_str(), line.length(), weight);
  }
}

int build(const char * const *args, std::size_t num_args) {
  marisa::Keyset keyset;
  if (num_args == 0) try {
    read_keys(std::cin, &keyset);
  } catch (const marisa::Exception &ex) {
    std::cerr << ex.what() << ": failed to read keys" << std::endl;
    return 10;
  }

  for (std::size_t i = 0; i < num_args; ++i) try {
    std::ifstream input_file(args[i], std::ios::binary);
    if (!input_file) {
      std::cerr << "error: failed to open: " << args[i] << std::endl;
      return 11;
    }
    read_keys(input_file, &keyset);
  } catch (const marisa::Exception &ex) {
    std::cerr << ex.what() << ": failed to read keys" << std::endl;
    return 12;
  }

  marisa::Trie trie;
  try {
    trie.build(keyset, param_num_tries | param_tail_mode | param_node_order);
  } catch (const marisa::Exception &ex) {
    std::cerr << ex.what() << ": failed to build a dictionary" << std::endl;
    return 20;
  }

  std::cerr << "#keys: " << trie.num_keys() << std::endl;
  std::cerr << "#nodes: " << trie.num_nodes() << std::endl;
  std::cerr << "size: " << trie.io_size() << std::endl;

  if (output_filename != NULL) {
    try {
      trie.save(output_filename);
    } catch (const marisa::Exception &ex) {
      std::cerr << ex.what() << ": failed to write a dictionary to file: "
          << output_filename << std::endl;
      return 30;
    }
  } else {
    try {
      std::cout << trie;
    } catch (const marisa::Exception &ex) {
      std::cerr << ex.what()
          << ": failed to write a dictionary to standard output" << std::endl;
      return 31;
    }
  }
  return 0;
}

}  // namespace

int main(int argc, char *argv[]) {
  std::ios::sync_with_stdio(false);

  ::cmdopt_option long_options[] = {
    { "max-num-tries", 1, NULL, 'n' },
    { "text-tail", 0, NULL, 't' },
    { "binary-tail", 0, NULL, 'b' },
    { "weight-order", 0, NULL, 'w' },
    { "label-order", 0, NULL, 'l' },
    { "output", 1, NULL, 'o' },
    { "help", 0, NULL, 'h' },
    { NULL, 0, NULL, 0 }
  };
  ::cmdopt_t cmdopt;
  ::cmdopt_init(&cmdopt, argc, argv, "n:tbwlo:h", long_options);
  int label;
  while ((label = ::cmdopt_get(&cmdopt)) != -1) {
    switch (label) {
      case 'n': {
        char *end_of_value;
        const long value = std::strtol(cmdopt.optarg, &end_of_value, 10);
        if ((*end_of_value != '\0') || (value <= 0) ||
            (value > MARISA_MAX_NUM_TRIES)) {
          std::cerr << "error: option `-n' with an invalid argument: "
              << cmdopt.optarg << std::endl;
        }
        param_num_tries = (int)value;
        break;
      }
      case 't': {
        param_tail_mode = MARISA_TEXT_TAIL;
        break;
      }
      case 'b': {
        param_tail_mode = MARISA_BINARY_TAIL;
        break;
      }
      case 'w': {
        param_node_order = MARISA_WEIGHT_ORDER;
        break;
      }
      case 'l': {
        param_node_order = MARISA_LABEL_ORDER;
        break;
      }
      case 'o': {
        output_filename = cmdopt.optarg;
        break;
      }
      case 'h': {
        print_help(argv[0]);
        return 0;
      }
      default: {
        return 1;
      }
    }
  }
  return build(cmdopt.argv + cmdopt.optind, cmdopt.argc - cmdopt.optind);
}
