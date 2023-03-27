#include <magic/filesystem/file.h>
#include <magic/common/result/make.h>

#include <fstream>
#include <sstream>

namespace magic {

Result<File::TextLines> File::ReadAllLines(std::string_view path) {
  auto result = TextLines();
  auto file = std::ifstream(path);

  if (file.fail()) {
    return Fail(std::make_error_code(std::errc::invalid_argument));
  }

  std::string line;
  while (std::getline(file, line)) {
    result.push_back(std::move(line));
  }

  return Ok(result);
}

Status File::WriteAllText(std::string_view path, std::string_view text) {
  auto file = std::ofstream(path);
  if (file.fail()) {
    return Fail(std::make_error_code(std::errc::invalid_argument));
  }
  file << text;

  return Ok();
}

Status File::WriteAllLines(std::string_view path, const File::TextLines& lines) {
  auto file = std::ofstream(path);
  if (file.fail()) {
    return Fail(std::make_error_code(std::errc::invalid_argument));
  }
  for (auto&& line : lines) {
    file << line << "\n";
  }

  return Ok();
}


}  // namespace magic