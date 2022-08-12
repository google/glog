#include <algorithm>
#include <array>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

std::string exec(const std::string& command) {
  std::array<char, 1048576> buffer{};
  std::string output;

  FILE* pipe = popen((command + " 2>&1").c_str(), "r");
  if (pipe == nullptr) {
    throw std::runtime_error("popen() failed!");
  }
  try {
    std::size_t bytesread;
    while ((bytesread = std::fread(buffer.data(), sizeof(buffer.at(0)),
                                   sizeof(buffer), pipe)) != 0) {
      output += std::string(buffer.data(), bytesread);
    }
  } catch (...) {
    pclose(pipe);
  }
  return output;
}

int main(int, char*[]) {
  auto output = exec(std::string(std::getenv("TARGET_APP")));

  std::vector<std::string> lines = {
      "stacktrace.cpp:3] Check failed: x == 1 (4 vs. 1) ",
      "google::LogMessage::Fail()",
      "google::LogMessage::SendToLog()",
      "google::LogMessage::Flush()",
      "google::LogMessageFatal::~LogMessageFatal()",
      "google::LogMessageFatal::~LogMessageFatal()",
      "bar()",
      "foo()",
      "main",
  };

  std::vector<std::string> failures;
  bool wasSuccess = std::accumulate(
      lines.begin(), lines.end(), true,
      [&output, &failures](bool acc, const std::string& line) {
        const bool found = (output.find(line.c_str(), 0) != std::string::npos);
        if (!found) {
          failures.push_back(line);
        }
        return acc && found;
      });

  if (!wasSuccess) {
    std::cout << "Failed to find stacktrace lines: " << std::endl;
    std::for_each(failures.begin(), failures.end(), [](std::string& line) {
      std::cout << "- \"" << line << "\"" << std::endl;
    });
    std::cout << "in output:" << std::endl << output << std::endl;
  }
  exit(wasSuccess ? 0 : 1);
}