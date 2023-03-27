#pragma once

#include <magic/common/result.h>

#include <vector>
#include <string>

namespace magic {

//////////////////////////////////////////////////////////////////////

// Thread-safe methods that may be used concurrently from any thread

struct File {
  using TextLines = std::vector<std::string>;

  // Opens a text file, reads all lines of the file into a string array, and then closes the file
  static Result<TextLines> ReadAllLines(std::string_view path);

  // Creates a new file, writes the specified string to the file, and then closes the file
  // If the target file already exists, it is overwritten
  static Status WriteAllText(std::string_view path, std::string_view text);

  static Status WriteAllLines(std::string_view path, const TextLines& lines);

};


//////////////////////////////////////////////////////////////////////

}  // namespace magic