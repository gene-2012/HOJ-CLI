#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <array>
#include <numeric>
#include <filesystem>
#include <thread>
#include <chrono>

#include "lib/json.hpp"
#include "lib/http.hpp"
#include "lib/CLI11.hpp"

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#else
#include <unistd.h>
#include <pwd.h>
#endif

using json = nlohmann::json;

/* ================== constants ================== */

#define OJ_HOST "ssf.hdoi.cn"
#define OJ_PORT 80

/* ================== console ================== */

void init_console() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
#endif
}

/* ================== path utils ================== */

std::string get_home_dir() {
#ifdef _WIN32
    const char* dir = getenv("USERPROFILE");
    return dir ? dir : "";
#else
    struct passwd* pw = getpwuid(getuid());
    return pw ? pw->pw_dir : "";
#endif
}

std::string workspace_config_path() {
    return ".hojcli.json";
}

std::string global_config_path() {
    return get_home_dir() + "/.config/hojcli/user.json";
}

/* ================== token io ================== */

std::string load_token() {
    std::ifstream ifs;

    if (std::filesystem::exists(workspace_config_path())) {
        ifs.open(workspace_config_path());
    } else {
        ifs.open(global_config_path());
    }

    if (!ifs.is_open()) {
        throw std::runtime_error("no config file found, please login first");
    }

    json j = json::parse(ifs);

    if (!j.contains("JSESSIONID") || !j["JSESSIONID"].is_string()) {
        throw std::runtime_error("invalid config: JSESSIONID missing");
    }

    return j["JSESSIONID"].get<std::string>();
}

void save_token(const std::string& token, bool local) {
    std::string path;

    if (local) {
        path = workspace_config_path();
    } else {
        std::string dir = get_home_dir() + "/.config/hojcli";
        std::filesystem::create_directories(dir);
        path = dir + "/user.json";
    }

    std::ofstream ofs(path);
    ofs << json({{"JSESSIONID", token}}).dump(2);
}

/* ================== network: login ================== */

std::string get_token_from_post(const std::string& username,
                                const std::string& password) {
    httplib::Client client(OJ_HOST, OJ_PORT);

    json body = {
        {"username", username},
        {"password", password}
    };

    auto res = client.Post(
        "/api/login",
        body.dump(),
        "application/json"
    );

    if (!res) {
        throw std::runtime_error("network error");
    }

    json resp = json::parse(res->body);

    if (!resp.contains("status") || resp["status"] != 200) {
        throw std::runtime_error("login failed");
    }

    auto it = res->headers.find("Set-Cookie");
    if (it == res->headers.end()) {
        throw std::runtime_error("no Set-Cookie in response");
    }

    const std::string& cookie = it->second;
    auto pos = cookie.find("JSESSIONID=");
    if (pos == std::string::npos) {
        throw std::runtime_error("JSESSIONID not found");
    }

    pos += std::string("JSESSIONID=").length();
    auto end = cookie.find(';', pos);
    return cookie.substr(pos, end - pos);
}

/* ================== utils ================== */

std::string read_file(const std::string& path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs.is_open()) {
        throw std::runtime_error("cannot open file: " + path);
    }
    return std::string(
        (std::istreambuf_iterator<char>(ifs)),
        std::istreambuf_iterator<char>()
    );
}

/* ================== domain ================== */

struct Problem {
    std::string id;
    std::vector<std::string> languages; 
    bool remote;
};

/* ================== HOJ Client ================== */

class HojClient {
public:
    explicit HojClient(std::string token)
        : token_(std::move(token)),
          client_(OJ_HOST, OJ_PORT) {}

    Problem get_problem(const std::string& problem_id) {
        auto res = client_.Get(
            "/api/get-problem-detail?problemId=" + problem_id,
            headers()
        );

        if (!res) {
            throw std::runtime_error("network error");
        }

        json j = json::parse(res->body);

        if (!j.contains("status") || j["status"] != 200) {
            throw std::runtime_error("invalid token or problem_id");
        }

        return Problem{
            problem_id,
            j["data"]["languages"].get<std::vector<std::string>>(),
            j["data"]["problem"]["isRemote"].get<bool>()
        };
    }

    int submit(const std::string& problem_id,
               const std::string& language,
               const std::string& code) {
        Problem p = get_problem(problem_id);

        if (!std::any_of(
            p.languages.begin(), p.languages.end(),
            [&](const std::string& l) { return l == language; })) {

            throw std::runtime_error(
                "invalid language. Available: " +
                std::accumulate(
                    p.languages.begin(), p.languages.end(),
                    std::string(),
                    [](auto& s, auto& l) { return s + l + ", "; }
                )
            );
        }

        json body = {
            {"pid", problem_id},
            {"code", code},
            {"language", language},
            {"isRemote", p.remote}
        };

        auto res = client_.Post(
            "/api/submit-problem-judge",
            headers(),
            body.dump(),
            "application/json"
        );

        if (!res) {
            throw std::runtime_error("network error");
        }

        json j = json::parse(res->body);

        if (!j.contains("status") || j["status"] != 200) {
            throw std::runtime_error("submit failed");
        }

        return j["data"]["submitId"].get<int>();
    }

    int wait_result(int submit_id, int timeout_round = 100) {
        while (timeout_round--) {
            auto res = client_.Get(
                "/api/get-submission-detail?submitId=" + std::to_string(submit_id),
                headers()
            );

            if (!res) {
                throw std::runtime_error("network error");
            }

            json j = json::parse(res->body);

            if (!j.contains("status") || j["status"] != 200) {
                throw std::runtime_error("get status failed");
            }

            int status = j["data"]["submission"]["status"].get<int>();

            if (status == 5 || status == 6 || status == 7) {
                std::cout << "Waiting for judge...\n";
                std::this_thread::sleep_for(std::chrono::seconds(1));
            } else {
                return status;
            }
        }
        return 4;
    }

private:
    std::string token_;
    httplib::Client client_;

    httplib::Headers headers() const {
        return {
            {"Content-Type", "application/json"},
            {"User-Agent", "HOJ-CLI_ByLJE/1.0"},
            {"Cookie", "JSESSIONID=" + token_}
        };
    }
};

/* ================== status map ================== */

const std::array<std::string, 14> STATUS_MAP = {
    "Accepted",
    "Time Limit Exceeded",
    "Memory Limit Exceeded",
    "Runtime Error",
    "System Error",
    "Pending",
    "Compiling",
    "Judging",
    "Partial Accepted",
    "Submitted Failed",
    "Canceled",
    "Presentation Error",
    "Compile Error",
    "Wrong Answer"
};

std::string map_status(int code) {
    if (code >= 0 && code < (int)STATUS_MAP.size()) {
        return STATUS_MAP[code];
    }
    return "Unknown Status";
}

/* ================== main ================== */

int main(int argc, char** argv) {
    init_console();

    CLI::App app{"HOJ-CLI"};

    if (argc == 1) {
        std::cout << "HOJ-CLI by LJE, Compiled at " << __DATE__ << " " << __TIME__ << "\n";
        return 0;
    }

    /* -------- login -------- */
    auto login = app.add_subcommand("login", "Login to HOJ");

    std::string username, password, jsessionid;
    bool local = false;

    login->add_option("-u,--username", username);
    login->add_option("-p,--password", password);
    login->add_option("--jsessionid", jsessionid);
    login->add_flag("-l,--local", local);

    login->callback([&]() {
        std::string token;

        if (!jsessionid.empty()) {
            token = jsessionid;
        } else {
            if (username.empty() || password.empty()) {
                throw CLI::RequiredError(
                    "need (--username AND --password) or --jsessionid"
                );
            }
            token = get_token_from_post(username, password);
        }

        save_token(token, local);
        std::cout << "Login success\n";
    });

    /* -------- submit -------- */
    auto submit = app.add_subcommand("submit", "Submit solution");

    std::string problem_id, language, file;

    submit->add_option("problem_id", problem_id)->required();
    submit->add_option("-l,--language", language)->required();
    submit->add_option("-f,--file", file)->required();

    submit->callback([&]() {
        HojClient hoj(load_token());
        std::string code = read_file(file);

        int id = hoj.submit(problem_id, language, code);
        std::cout << "SubmitId = " << id << "\n";

        int status = hoj.wait_result(id);
        std::cout << "Result = " << map_status(status) << "\n";
    });

    /* -------- whoami -------- */
    auto whoami = app.add_subcommand("whoami", "Show token");
    whoami->callback([&]() {
        std::cout << "JSESSIONID = " << load_token() << "\n";
    });

    CLI11_PARSE(app, argc, argv);
    return 0;
}
