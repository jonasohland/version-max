#include <boost/process.hpp>
#include <boost/algorithm/string.hpp>
#include <o.h>
#include <c74_min.h>

namespace o {

    class version : public c74::min::object<version> {
        
    public:
        
        
        version() = default;
        
        version(c74::min::atoms args){}

        c74::min::atoms get_version(c74::min::atoms args, int inlet) {

            c74::max::t_object* my_patcher;

            c74::max::t_max_err e = c74::max::object_obex_lookup(
                this->maxobj(), c74::max::gensym("#P"), &my_patcher);

            if (e) {
                cerr << "Could not get current patcher address"
                     << c74::min::endl;
                
                out.send("ERR_INTERNAL");
                return args;
            }

            c74::min::symbol path = c74::max::object_attr_getsym(
                my_patcher, c74::max::gensym("filepath"));

            std::string path_str = std::string(path.c_str());

            if (!boost::algorithm::find_first(path_str, "/")) {
                cerr << "You must save the patcher to a git repository to get "
                        "a version tag"
                     << c74::min::endl;
                out.send("ERR_NO_PATCHER");
                return args;
            }

            auto fixed_path =
                std::string(boost::algorithm::find_first(path_str, "/").begin(),
                            path_str.end());

            fixed_path = std::string(
                fixed_path.begin(),
                boost::algorithm::find_last(fixed_path, "/").begin());

            std::stringstream command;
            
            auto git = boost::process::search_path("git");
            
            if(git.empty()){
                cerr << "Could not find git on this machine" << c74::min::endl;
                out.send("ERR_GIT_NOT_FOUND");
                return args;
            }

            command << git << " -C \"" << fixed_path << "\" describe --tags";
            
            try {

                boost::process::ipstream input;

                boost::process::child ch{
                    command.str(), boost::process::std_out > input};

                std::string line;
                
                if(ch.joinable())
                    ch.join();
                
                if(ch.exit_code()){
                    cerr << "Could not get version tag from git, code: " << ch.exit_code() << c74::min::endl;
                    out.send("ERR_NO_VERSION_TAG");
                    return args;
                }
                
                while (input && std::getline(input, line) && !line.empty())
                    out.send(line);
                
            } catch (boost::process::process_error err){
                cerr << "Failed to execute git: " << err.what() << c74::min::endl;
                out.send("ERR_EXEC");
            }
            
            return args;
        }

        c74::min::message<> get_version_m { this, "bang", "Output current repository version", std::bind(&version::get_version, this, std::placeholders::_1, std::placeholders::_2) };
        
        c74::min::outlet<c74::min::thread_check::any, c74::min::thread_action::fifo> out {this, "version out", "list"};
    };
}

void ext_main(void* r) {
    c74::max::object_post(nullptr, "version external %s // (c) Jonas Ohland 2019", O_THIS_TARGET_VERSION());
    c74::min::wrap_as_max_external<o::version>("version", __FILE__, r);
}
