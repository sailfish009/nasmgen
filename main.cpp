#include "program.hpp"
#include "inst.hpp"
#include "common.hpp"
#include <fstream>
#include <string>
#include <vector>


static bool read_file(const char* fn, std::vector<std::string>& rv) {
	std::ifstream file(fn);
	if (!file) {
		return false;
	}
	std::string str;
	while (std::getline(file, str)) {
		rv.push_back(str);
	}
	return true;
}


int main(int argc, char** argv) {
	std::vector<std::string> lines;
	if (argc != 2 || !read_file(argv[1], lines)) {
		LOG("usage: %s <file>", argv[0]);
		return 1;
	}

	Program program;

	for (std::vector<std::string>::const_iterator s=lines.begin(); s!=lines.end(); ++s) {
		Inst* i = Inst::parse((*s).c_str());
		if (i) {
			program.push(i);
		} else {
			LOG("cannot parse line #%zu", s-lines.begin()+1);
			return 1;
		}
	}

	if (!program.interpret()) {
		LOG("cannot interpret");
		return 1;
	}

	return 0;
}
