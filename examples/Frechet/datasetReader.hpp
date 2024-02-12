#ifndef _READER_HPP
#define _READER_HPP

#include <vector>
#include <tuple>
#include <fstream>

struct reader {
	const double NO_VALUE=0.0;
	std::string file_name;
	long id;
	int  RorS;
	 
	std::fstream f;  
	reader():file_name(""),id(-1),RorS(-1) {}
	reader(const std::string& file_name):file_name(file_name) {};
	void read_init(){
		id = -1;
		RorS = -1;
		f.open(file_name.c_str(),std::fstream::in);
	}

    bool eof(){return f.eof();}

    void read(std::vector<std::tuple<double,double,double>>& trajectories) {
		std::string tmp;
		f >> tmp;
		id = stol(tmp);
		f >> tmp;
		RorS = stoi(tmp);
		f >> tmp;
		if (!(tmp.find_first_of("[")==std::string::npos)) {
			tmp.replace(0, 1, "");
			tmp.replace(tmp.length() - 1, tmp.length(), "");
			bool ext = false;
			while (!ext) {
				std::string extrait = tmp.substr(tmp.find("["), tmp.find("]") + 1);
				std::string extrait1 = extrait.substr(1, extrait.find(",") - 1);
				std::string extrait2 = extrait.substr(extrait.find(",") + 1);
				extrait2 = extrait2.substr(0, extrait2.length() - 1);
				double e1 = stod(extrait1);
				double e2 = stod(extrait2);
				trajectories.push_back(std::tuple<double,double,double>(e1,e2,NO_VALUE));
				ext = (tmp.length() == extrait.length());
				if (!ext)
					tmp = tmp.substr(tmp.find_first_of("]") + 2, tmp.length());
			}
		}
	}
	void read_stop(){
		f.close();
		f.clear();
	}
};

#endif
