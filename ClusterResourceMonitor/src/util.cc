#include "util.h"
using std::string;
namespace moniter{

string Util::open_file(const string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr <<"[Util.cc:"<<__LINE__<<"]Failed to open " << filePath << std::endl;
        throw std::runtime_error("Failed to open file: " + filePath);
    }
    std::ostringstream oss;
    oss << file.rdbuf(); 
    return oss.str();
}

string Util::exec(const string& cmd) {

    std::array<char, 1024> buffer;
    string result;
    std::shared_ptr<FILE> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe){
        std::cerr <<"[Util.cc:"<<__LINE__<<"]popen() failed"<< std::endl;
        throw std::runtime_error("Failed to exectute command: " + cmd);
    }
    while (!feof(pipe.get())) {
        if (fgets(buffer.data(), 1024, pipe.get()) != nullptr)
            result += buffer.data();
    }
    return result;
}

string Util::find_value(const string&  text, const string& key, size_t pos){
    //find key and update pos
    pos = text.find(key, pos);
    if(pos == string::npos){
        std::cerr << "[Util.cc:"<<__LINE__<<"]key not found: "<< key << std::endl;
        throw std::runtime_error("Failed to find the key: " + key);
    }
    
    //find ":" and "\n",and judge whether it is legitimate
    size_t colon_pos = text.find(":",pos);
    size_t wrap_pos = text.find("\n",pos);
    if (colon_pos == string::npos or wrap_pos == string::npos or colon_pos > wrap_pos){
        std::cerr <<"[Util.cc:"<<__LINE__<<"]format error: "<< std::endl;
        throw std::runtime_error("Format error: " + key);
    }
// #if DEBUG
//         std::cout <<"[util.cc:"<<__LINE__<<"]"<< "colon_pos: "<<colon_pos <<std::endl;
//         std::cout <<"[util.cc:"<<__LINE__<<"]"<< "wrap_pos: "<<wrap_pos <<std::endl;
// #endif

    //find the corresponding value. strike the leading and trailing spaces
    size_t value_start_pos =  text.find_first_not_of(" ",colon_pos+1);
    if(value_start_pos == wrap_pos){
        std::cerr << "[Util.cc:"<<__LINE__<<"]No corresponding value found: "<< std::endl;
        throw std::runtime_error("No corresponding value found:" + key);
    }
// #if DEBUG
//         std::cout <<"[util.cc:"<<__LINE__<<"]"<< "value_start_pos: "<<value_start_pos <<std::endl;
// #endif
    size_t value_end_pos;
    for (size_t i = wrap_pos + 1; i > colon_pos; --i) {
        if (!std::isspace(text[i - 1])) {
            value_end_pos =  i;
// #if DEBUG
//         std::cout <<"[util.cc:"<<__LINE__<<"]"<< "value_end_pos: "<<value_end_pos <<std::endl;
// #endif
            return text.substr(value_start_pos, value_end_pos-value_start_pos);
        }
    }

}


string Util::remove_unit( const string& text){
    size_t start_pos = text.find_first_of("0123456789");
    size_t end_pos = text.find_first_not_of("0123456789.");
    return text.substr(start_pos,end_pos-start_pos);
};
}


