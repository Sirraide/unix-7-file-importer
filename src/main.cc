#include <clopts.hh>
#include <filesystem>
#include <ranges>
#include <stream/stream.hh>
#include <utils.hh>

using FileHandle = std::unique_ptr<FILE, decltype(&std::fclose)>;

namespace fs = std::filesystem;
namespace vws = std::ranges::views;

namespace detail {
using namespace command_line_options;
using options = clopts< // clang-format off
    help<>,
    positional<"archive", "The archive to unpack", file<std::string, std::string>>,
    option<"--prefix", "The directory to unpack into">
>; // clang-format on
}

template <typename... Args>
[[noreturn]] void Error(std::format_string<Args...> fmt, Args&&... args) {
    throw std::runtime_error(std::format(fmt, std::forward<Args>(args)...));
}

auto TakeLine(streams::stream& s) -> std::string_view {
    auto l = s.take_until('\n');
    if (not s.consume('\n')) Error("Expected newline");
    return l;
}

void WriteFile(const fs::path& name, std::string_view contents) {
    FileHandle f{std::fopen(name.c_str(), "wb"), std::fclose};
    if (not f) Error("Failed to open file '{}' for writing", name.string());
    if (std::fwrite(contents.data(), 1, contents.size(), f.get()) != contents.size())
        Error("Failed to write file");
}

auto HexVal(int c) -> int {
    c = std::toupper(c);
    if (c >= 'A') return c - 'A' + 10;
    if (c >= '0') return c - '0';
    Error("Invalid hex digit '{}'", c);
}

auto Unhexify(std::string_view in) -> std::string {
    std::string data;
    if ((in.size() & 1) != 0) Error("Expected even amount of hex digits, got '{}' ({})", in, in.size());
    for (auto chunk : in | vws::chunk(2))
        data += char(HexVal(chunk[0]) << 4 | HexVal(chunk[1]));
    return data;
}

// PERM NL
// FILENAME NL
// SIZE NL
// HEXDATA ...{NL}...
void Unpack(std::string_view in, std::string_view pre) {
    fs::path prefix{streams::stream{pre}.trim().text()};
    if (prefix.empty()) Error("Prefix may not be empty!");
    streams::stream s{in};
    while (not s.empty()) {
        auto perm = std::stoull(std::string{TakeLine(s)}, 0, 8);
        auto filename = TakeLine(s);
        auto size = std::stoll(std::string{TakeLine(s)}) * 2; // Two hex chars per byte.
        std::string data;
        while (size != 0) {
            if (s.empty()) Error("Unexpected EOF");
            auto line = TakeLine(s);
            data += Unhexify(line);
            size -= std::ssize(line);
            if (size < 0) Error("Data size mismatch");
        }

        if (filename.contains("..")) Error("Illegal '..' in path");
        fs::create_directories((prefix / filename).remove_filename());
        WriteFile(prefix / filename, data);
        fs::permissions(prefix / filename, static_cast<fs::perms>(perm));
    }
}

int main(int argc, char** argv) {
    auto opts = detail::options::parse(argc, argv);
    auto& data = opts.get<"archive">()->contents;
    try {
        Unpack(data, opts.get_or<"--prefix">("."));
    } catch (const std::runtime_error& e) {
        std::println(stderr, "Error: {}", e.what());
        std::exit(1);
    }
}
