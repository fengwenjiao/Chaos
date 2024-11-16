#ifndef MONITER_Util_H_
#define MONITER_Util_H_
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <array>
#include <memory>
#include <sstream>

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define LOG(msg) std::cout << "[" << __FILENAME__ << ":" << __LINE__ << "]:" << msg << std::endl;
namespace moniter{

class Util{
    public:
    /**
     *\brief open file and return a content 
     *\param filepathPath Path to the file to be open, like "/proc/cpuinfo"
     *\return file content
     */
    static std::string open_file(const std::string& filePath);
    /**
     *\brief execute command in shell and return the result 
     *\param cmd Command to be executed
     *\return result of exeution 
     */
    static std::string exec(const std::string& cmd);
    /**
     *\brief find the value corresponding to key in text, like" Max   : 20 % " start from pos
     *\param text Text to find value in
     *\param key Key word
     *\param pos search value from pos
     *\return value corresponding to the key
     */
    static std::string find_value(const std::string& text, const std::string& key, size_t pos=0);
    /**
     *\brief remove the unit in a string ,like "1234.2 MB"->"1234.2"
     *\param text Text to remove unit in.
     */
    static std::string remove_unit(const std::string& text);

};
}

#endif // MONITER_Util_H_