/**
 * @InFileStream.h
 *
 * Helper class for reading configuration files of options.
 * It is based on `std::ifstream`, i.e. all values that should
 * be read must be streamable with that class. In case of an
 * error, an exception is thrown. Unfortunately, this simple
 * implementation does not provide helpful error messages.
 *
 * Example:
 *
 * Assume the following declaration:
 *
 *     option(AnOption, load((int) anIntValue,
 *                           (float) aFloatValue)) {...
 *
 * Then, a file with the name "AnOption.cfg" must exist in
 * current directory, containing data such as this:
 *
 *      anIntValue: 17
 *      aFloatValue: 3.14
 *
 * There must be a newline present after each line.
 *
 * @author Thomas Röfer
 */

#pragma once

#include <cctype>
#include <fstream>
#include <string>

namespace cabsl
{
  class InFileStream
  {
    std::ifstream stream; /**< The stream to read from. */

    /**
     * Expect a certain string in the input. If the string is not present,
     * the operation fails with an exception.
     * @param pattern The string.
     */
    void expect(const char* pattern)
    {
      char c;
      while(std::isspace(static_cast<unsigned char>(stream.peek())) && *pattern != stream.peek())
        stream.get(c);
      while(*pattern && stream && *pattern == stream.peek())
      {
        stream.get(c);
        ++pattern;
      }
      if(*pattern)
        stream.setstate(std::ios::failbit);
    }

  public:
    /**
     * Open a file for reading. If opening fails, an exception is thrown.
     * @param basename The basename of the file. This implementation appends
     * `.cfg` to the basename and then opens the file.
     */
    InFileStream(const std::string& basename)
    {
      stream.exceptions(std::ios::failbit | std::ios::badbit);
      stream.open(basename + ".cfg");
    }

    /**
     * Read a name/value pair. They are separated by a colon. The value must
     * be followed by a newline, even in the last line of the file.
     * An exception is thrown if reading fails.
     * @tparam U The type of the value. An operator `>>` must exist to read a
     * value of this type.
     * @param name The name that is expected. CABSL will actually pass a
     * longer string, where the name is just the suffix. Everything up to the
     * last space or closing parenthesis is skipped.
     * @param value The variable the read value is written to.
     */
    template<typename U> void read(const char* name, U& value)
    {
      name += 1 + static_cast<int>(std::string(name).find_last_of(" )"));
      expect(name);
      expect(":");
      stream >> value;
      expect("\n");
    }
  };
}

