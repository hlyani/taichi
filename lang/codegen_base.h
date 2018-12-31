#include "util.h"
#include "visitor.h"
#include "expr.h"

TLANG_NAMESPACE_BEGIN

class CodeGenBase : public Visitor {
 public:
  int var_count;
  int snode_count;
  int accessor_count;
  std::string code, code_suffix;
  std::string folder;
  std::string func_name;
  int num_groups;
  int id;
  std::string suffix;
  void *dll;

  static int get_code_gen_id() {
    static int id = 0;
    TC_ASSERT(id < 10000);
    return id++;
  }

  CodeGenBase() : Visitor(Visitor::Order::child_first) {
    code = "";
    id = get_code_gen_id();
    func_name = fmt::format("func{:06d}", id);

    dll = nullptr;

    folder = "_tlang_cache/";
    create_directories(folder);
    var_count = 0;
    snode_count = 0;
    accessor_count = 0;
    code_suffix = "\n";
  }

  template <typename T>
  static std::string vec_to_list(std::vector<T> val, std::string brace) {
    std::string members = brace;
    bool first = true;
    for (int i = 0; i < (int)val.size(); i++) {
      if (!first) {
        members += ",";
      }
      first = false;
      members += fmt::format("{}", val[i]);
    }
    if (brace == "<") {
      members += ">";
    } else if (brace == "{") {
      members += "}";
    } else if (brace == "(") {
      members += ")";
    } else {
      TC_P(brace);
      TC_NOT_IMPLEMENTED
    }
    return members;
  }

  std::string create_variable() {
    TC_ASSERT(var_count < 10000);
    return fmt::format("var_{:04d}", var_count++);
  }

  std::string create_snode() {
    TC_ASSERT(snode_count < 10000);
    return fmt::format("SNode{:04d}", snode_count++);
  }

  std::string create_accessor() {
    TC_ASSERT(accessor_count < 10000);
    return fmt::format("access_{:04d}", accessor_count++);
  }

  std::string get_source_fn() {
    return fmt::format("{}/tmp{:04d}.{}", folder, id, suffix);
  }

  std::string get_project_fn() {
    return fmt::format("{}/projects/taichi_lang/", get_repo_dir());
  }

  std::string get_library_fn() {
#if defined(TC_PLATFORM_OSX)
    // Note: use .so here will lead to wired behavior...
    return fmt::format("{}/tmp{:04d}.dylib", folder, id);
#else
    return fmt::format("{}/tmp{:04d}.so", folder, id);
#endif
  }

  template <typename... Args>
  void emit_code(std::string f, Args &&... args) {
    if (sizeof...(args)) {
      code += fmt::format(f, std::forward<Args>(args)...) + code_suffix;
    } else {
      code += f + code_suffix;
    }
  }

  void write_code_to_file() {
    {
      std::ofstream of(get_source_fn());
      of << code;
    }
    trash(std::system(
        fmt::format("cp {} {}_unformated", get_source_fn(), get_source_fn())
            .c_str()));
    auto format_ret =
        std::system(fmt::format("clang-format -i {}", get_source_fn()).c_str());
    trash(format_ret);
  }

  void load_dll() {
    auto dll = dlopen(("./" + get_library_fn()).c_str(), RTLD_LAZY);
    TC_ASSERT(dll != nullptr);
  }

  FunctionType load_function() {
    if (dll == nullptr) {
      load_dll();
    }
    auto ret = dlsym(dll, func_name.c_str());
    TC_ASSERT(ret != nullptr);
    return (FunctionType)ret;
  }

  void disassemble() {
#if defined(TC_PLATFORM_LINUX)
    auto objdump_ret = system(
        fmt::format("objdump {} -d > {}.s", get_library_fn(), get_library_fn())
            .c_str());
    trash(objdump_ret);
#endif
  }
};

TLANG_NAMESPACE_END
