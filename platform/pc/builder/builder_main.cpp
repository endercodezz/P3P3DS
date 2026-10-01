// P3P3DS Builder: Windows wizard (Win32, no external UI library) around
// pipeline.cpp. Portable: sdk/, work/ and output/ live next to the .exe.
// `P3P3DS-Builder.exe --cli --iso <iso> [--devkitpro <dir>] [--out <file>]
// [--jobs <n>]` runs the same build in a console (used for testing).
#include "fun_facts.hpp"
#include "pipeline.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlobj.h>

#include <chrono>
#include <cstdio>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;
using namespace p3p3ds::builder;

namespace {

constexpr int kWidth = 620, kHeight = 470, kHeader = 64;
constexpr UINT WM_BUILD_PROGRESS = WM_APP + 1, WM_BUILD_LOG = WM_APP + 2, WM_BUILD_DONE = WM_APP + 3;
constexpr UINT_PTR kFactTimer = 1;
enum Id : int {
    IdBack = 100, IdNext, IdCancel,
    IdIso, IdIsoBrowse, IdIsoStatus, IdDkp, IdDkpBrowse, IdDkpStatus, IdDkpLink, IdOut, IdOutBrowse, IdSdCheck, IdSdDrive,
    IdStep, IdBar, IdTime, IdFact, IdNextFact, IdLog, IdOpenFolder,
};
enum Page { Welcome, Setup, Build, Finish };

struct App {
    HINSTANCE inst{};
    HWND wnd{};
    HFONT font{}, bold{}, title{};
    Page page{Welcome};
    std::vector<std::pair<Page, HWND>> controls;
    HWND back{}, next{}, cancel{};
    HWND iso{}, iso_status{}, dkp{}, dkp_status{}, out{}, sd_check{}, sd_drive{};
    HWND step{}, bar{}, time{}, fact{}, log{}, finish_text{}, open_folder{};
    fs::path exe_dir;
    std::unique_ptr<Pipeline> pipeline;
    std::thread worker;
    bool building{}, success{};
    std::string error;
    std::chrono::steady_clock::time_point started;
    unsigned fact_index{};
} app;

std::wstring widen(const std::string &s) {
    if (s.empty()) return {};
    const int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    std::wstring w(static_cast<std::size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), w.data(), n);
    return w;
}
std::wstring text_of(HWND h) {
    std::wstring s(static_cast<std::size_t>(GetWindowTextLengthW(h)) + 1, L'\0');
    GetWindowTextW(h, s.data(), static_cast<int>(s.size()));
    s.resize(wcslen(s.c_str()));
    return s;
}

HWND add(Page page, const wchar_t *cls, const wchar_t *text, DWORD style, int x, int y, int w, int h, int id = 0, HFONT f = nullptr) {
    HWND c = CreateWindowExW(0, cls, text, WS_CHILD | style, x, y, w, h, app.wnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), app.inst, nullptr);
    SendMessageW(c, WM_SETFONT, reinterpret_cast<WPARAM>(f ? f : app.font), TRUE);
    app.controls.emplace_back(page, c);
    return c;
}

void show_page(Page p) {
    app.page = p;
    for (auto &[page, c] : app.controls) ShowWindow(c, page == p ? SW_SHOW : SW_HIDE);
    EnableWindow(app.back, p == Setup);
    SetWindowTextW(app.next, p == Setup ? L"Build" : p == Finish ? L"Close" : L"Next >");
    EnableWindow(app.next, p != Build);
    SetWindowTextW(app.cancel, p == Build ? L"Cancel" : L"Exit");
    EnableWindow(app.cancel, p != Finish);
    InvalidateRect(app.wnd, nullptr, TRUE);
}

void validate() {
    const auto iso = fs::path(text_of(app.iso));
    const auto iso_problem = iso.empty() ? std::string("Choose your Persona 3 Portable ISO.") : Pipeline::check_iso(iso);
    SetWindowTextW(app.iso_status, iso_problem.empty() ? L"✔ Persona 3 Portable (ULUS-10512)" : widen(iso_problem).c_str());
    const auto dkp_problem = Pipeline::check_devkitpro(fs::path(text_of(app.dkp)));
    SetWindowTextW(app.dkp_status, dkp_problem.empty() ? L"✔ devkitPro with 3DS development tools" : widen(dkp_problem).c_str());
    EnableWindow(app.next, iso_problem.empty() && dkp_problem.empty() && !text_of(app.out).empty());
}

void fill_drives() {
    SendMessageW(app.sd_drive, CB_RESETCONTENT, 0, 0);
    const DWORD mask = GetLogicalDrives();
    for (int i = 0; i < 26; ++i) {
        if (!(mask & (1u << i))) continue;
        const wchar_t root[4] = {static_cast<wchar_t>(L'A' + i), L':', L'\\', 0};
        if (GetDriveTypeW(root) != DRIVE_REMOVABLE) continue;
        wchar_t label[64] = L"";
        GetVolumeInformationW(root, label, 64, nullptr, nullptr, nullptr, nullptr, 0);
        std::wstring item = std::wstring(root, 2) + L"  " + label;
        SendMessageW(app.sd_drive, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(item.c_str()));
    }
    SendMessageW(app.sd_drive, CB_SETCURSEL, 0, 0);
}

void next_fact() {
    constexpr unsigned n = sizeof kFunFacts / sizeof kFunFacts[0];
    SetWindowTextW(app.fact, kFunFacts[app.fact_index++ % n]);
}

std::wstring browse_file(const wchar_t *filter, bool save, const std::wstring &initial) {
    wchar_t buf[MAX_PATH] = L"";
    wcsncpy(buf, initial.c_str(), MAX_PATH - 1);
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof ofn;
    ofn.hwndOwner = app.wnd;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = buf;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = save ? OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST : OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (save) ofn.lpstrDefExt = L"3dsx";
    const BOOL ok = save ? GetSaveFileNameW(&ofn) : GetOpenFileNameW(&ofn);
    return ok ? std::wstring(buf) : std::wstring();
}
std::wstring browse_folder(const wchar_t *title) {
    BROWSEINFOW bi{};
    bi.hwndOwner = app.wnd;
    bi.lpszTitle = title;
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
    if (!pidl) return {};
    wchar_t buf[MAX_PATH];
    const bool ok = SHGetPathFromIDListW(pidl, buf);
    CoTaskMemFree(pidl);
    return ok ? std::wstring(buf) : std::wstring();
}

void start_build() {
    Settings s;
    s.iso = fs::path(text_of(app.iso));
    s.devkitpro = fs::path(text_of(app.dkp));
    s.output = fs::path(text_of(app.out));
    s.sdk = app.exe_dir / "sdk";
    s.work = app.exe_dir / "work";
    std::wstring sd;
    if (SendMessageW(app.sd_check, BM_GETCHECK, 0, 0) == BST_CHECKED) sd = text_of(app.sd_drive).substr(0, 2);
    const HWND wnd = app.wnd;
    app.pipeline = std::make_unique<Pipeline>(
        s,
        [wnd](const Progress &p) { PostMessageW(wnd, WM_BUILD_PROGRESS, 0, reinterpret_cast<LPARAM>(new Progress(p))); },
        [wnd](const std::string &line) { PostMessageW(wnd, WM_BUILD_LOG, 0, reinterpret_cast<LPARAM>(new std::string(line))); });
    app.building = true;
    app.started = std::chrono::steady_clock::now();
    show_page(Build);
    next_fact();
    SetTimer(app.wnd, kFactTimer, 15000, nullptr);
    app.worker = std::thread([s, sd, wnd] {
        auto *result = new std::string;
        try {
            app.pipeline->run();
            if (!sd.empty()) {
                const fs::path root = fs::path(sd + L"\\");
                PostMessageW(wnd, WM_BUILD_PROGRESS, 0, reinterpret_cast<LPARAM>(new Progress{"Copying to the SD card", 1.0}));
                fs::create_directories(root / "3ds");
                fs::create_directories(root / "p3p3ds");
                fs::copy_file(s.output, root / "3ds" / "p3p3ds.3dsx", fs::copy_options::overwrite_existing);
                const fs::path iso_target = root / "p3p3ds" / s.iso.filename();
                std::error_code ec;
                if (!fs::exists(iso_target) || fs::file_size(iso_target, ec) != fs::file_size(s.iso, ec)) {
                    PostMessageW(wnd, WM_BUILD_PROGRESS, 0,
                                 reinterpret_cast<LPARAM>(new Progress{"Copying the game image to the SD card (1.3 GB)", 1.0}));
                    fs::copy_file(s.iso, iso_target, fs::copy_options::overwrite_existing);
                }
            }
        } catch (const std::exception &e) {
            *result = e.what();
        }
        PostMessageW(wnd, WM_BUILD_DONE, 0, reinterpret_cast<LPARAM>(result));
    });
}

std::wstring minutes(double seconds) {
    const int s = static_cast<int>(seconds + 0.5);
    wchar_t buf[32];
    swprintf(buf, 32, L"%d:%02d", s / 60, s % 60);
    return buf;
}

void create_controls() {
    const int left = 24, width = kWidth - 48;
    int y = kHeader + 20;
    // Welcome
    add(Welcome, L"STATIC", L"Welcome to the P3P3DS Builder", SS_LEFT, left, y, width, 28, 0, app.title);
    add(Welcome, L"STATIC",
        L"This wizard builds P3P3DS - Persona 3 Portable running natively on the New Nintendo 3DS - "
        L"from your own copy of the game (ULUS-10512).\n\n"
        L"It reads the game's executable from your ISO, translates it to native 3DS code and packages "
        L"p3p3ds.3dsx for the Homebrew Launcher. Nothing is downloaded.\n\n"
        L"You need: your Persona 3 Portable ISO, and devkitPro with the 3DS development tools installed. "
        L"The build takes about 5-15 minutes depending on your CPU.\n\n"
        L"The result contains code made from your game: it is for you only. Please do not share it.\n\n"
        L"P3P3DS by enderlit aka endercodezz. Persona 3 Portable © ATLUS. This project is not affiliated with ATLUS or SEGA.",
        SS_LEFT, left, y + 40, width, 260);
    // Setup
    add(Setup, L"STATIC", L"Persona 3 Portable image (ULUS-10512, .iso):", SS_LEFT, left, y, width, 18);
    app.iso = add(Setup, L"EDIT", L"", WS_BORDER | ES_AUTOHSCROLL, left, y + 20, width - 100, 24, IdIso);
    add(Setup, L"BUTTON", L"Browse...", BS_PUSHBUTTON, left + width - 92, y + 19, 92, 26, IdIsoBrowse);
    app.iso_status = add(Setup, L"STATIC", L"", SS_LEFT, left, y + 48, width, 18, IdIsoStatus);
    y += 78;
    add(Setup, L"STATIC", L"devkitPro folder (with the 3DS development tools):", SS_LEFT, left, y, width, 18);
    app.dkp = add(Setup, L"EDIT", L"", WS_BORDER | ES_AUTOHSCROLL, left, y + 20, width - 100, 24, IdDkp);
    add(Setup, L"BUTTON", L"Browse...", BS_PUSHBUTTON, left + width - 92, y + 19, 92, 26, IdDkpBrowse);
    app.dkp_status = add(Setup, L"STATIC", L"", SS_LEFT, left, y + 48, width - 150, 18, IdDkpStatus);
    add(Setup, L"BUTTON", L"Get devkitPro...", BS_PUSHBUTTON, left + width - 140, y + 46, 140, 24, IdDkpLink);
    y += 82;
    add(Setup, L"STATIC", L"Save p3p3ds.3dsx to:", SS_LEFT, left, y, width, 18);
    app.out = add(Setup, L"EDIT", L"", WS_BORDER | ES_AUTOHSCROLL, left, y + 20, width - 100, 24, IdOut);
    add(Setup, L"BUTTON", L"Browse...", BS_PUSHBUTTON, left + width - 92, y + 19, 92, 26, IdOutBrowse);
    y += 60;
    app.sd_check = add(Setup, L"BUTTON", L"Also copy P3P3DS and the ISO to my SD card:", BS_AUTOCHECKBOX, left, y, 300, 22, IdSdCheck);
    app.sd_drive = add(Setup, L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_VSCROLL, left + 310, y, 160, 200, IdSdDrive);
    y += 30;
    wchar_t jobs[96];
    swprintf(jobs, 96, L"The build will use %u parallel compiler jobs on this PC.", Pipeline::automatic_jobs());
    add(Setup, L"STATIC", jobs, SS_LEFT, left, y, width, 18);
    // Build
    y = kHeader + 20;
    app.step = add(Build, L"STATIC", L"Preparing", SS_LEFT, left, y, width, 20, IdStep, app.bold);
    app.bar = add(Build, PROGRESS_CLASSW, L"", 0, left, y + 26, width, 22, IdBar);
    SendMessageW(app.bar, PBM_SETRANGE32, 0, 1000);
    app.time = add(Build, L"STATIC", L"", SS_LEFT, left, y + 54, width, 18, IdTime);
    add(Build, L"BUTTON", L"Did you know?", BS_GROUPBOX, left, y + 84, width, 130);
    app.fact = add(Build, L"STATIC", L"", SS_LEFT, left + 14, y + 106, width - 28, 76, IdFact);
    add(Build, L"BUTTON", L"Another fact", BS_PUSHBUTTON, left + width - 124, y + 182, 110, 24, IdNextFact);
    app.log = add(Build, L"EDIT", L"", WS_BORDER | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | WS_VSCROLL, left, y + 226, width, 80, IdLog);
    // Finish
    app.finish_text = add(Finish, L"STATIC", L"", SS_LEFT, left, y, width, 260);
    app.open_folder = add(Finish, L"BUTTON", L"Open output folder", BS_PUSHBUTTON, left, y + 270, 170, 28, IdOpenFolder);

    app.back = CreateWindowExW(0, L"BUTTON", L"< Back", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, kWidth - 300, kHeight - 44, 86, 28, app.wnd,
                               reinterpret_cast<HMENU>(IdBack), app.inst, nullptr);
    app.next = CreateWindowExW(0, L"BUTTON", L"Next >", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, kWidth - 208, kHeight - 44, 86, 28, app.wnd,
                               reinterpret_cast<HMENU>(IdNext), app.inst, nullptr);
    app.cancel = CreateWindowExW(0, L"BUTTON", L"Exit", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, kWidth - 108, kHeight - 44, 86, 28, app.wnd,
                                 reinterpret_cast<HMENU>(IdCancel), app.inst, nullptr);
    for (HWND b : {app.back, app.next, app.cancel}) SendMessageW(b, WM_SETFONT, reinterpret_cast<WPARAM>(app.font), TRUE);

    // Defaults: the single ISO next to the builder, devkitPro guess, output folder next to the builder.
    for (const auto &e : fs::directory_iterator(app.exe_dir))
        if (e.path().extension() == ".iso") { SetWindowTextW(app.iso, e.path().wstring().c_str()); break; }
    SetWindowTextW(app.dkp, Pipeline::find_devkitpro().wstring().c_str());
    SetWindowTextW(app.out, (app.exe_dir / "output" / "p3p3ds.3dsx").wstring().c_str());
    fill_drives();
}

void paint_header(HDC dc) {
    RECT r{0, 0, kWidth, kHeader};
    HBRUSH white = CreateSolidBrush(RGB(255, 255, 255));
    FillRect(dc, &r, white);
    DeleteObject(white);
    RECT band{0, 0, 8, kHeader};
    HBRUSH blue = CreateSolidBrush(RGB(30, 70, 160));
    FillRect(dc, &band, blue);
    DeleteObject(blue);
    SetBkMode(dc, TRANSPARENT);
    SelectObject(dc, app.bold);
    SetTextColor(dc, RGB(20, 20, 20));
    static const wchar_t *titles[] = {L"P3P3DS Builder", L"Your game and tools", L"Building P3P3DS", L"Finished"};
    static const wchar_t *subtitles[] = {L"Persona 3 Portable for New Nintendo 3DS", L"Choose the ISO, devkitPro and where to save the result",
                                         L"Translating and compiling the game code for the 3DS", L"Your build"};
    TextOutW(dc, 24, 14, titles[app.page], static_cast<int>(wcslen(titles[app.page])));
    SelectObject(dc, app.font);
    SetTextColor(dc, RGB(80, 80, 80));
    TextOutW(dc, 36, 36, subtitles[app.page], static_cast<int>(wcslen(subtitles[app.page])));
    HPEN pen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
    SelectObject(dc, pen);
    MoveToEx(dc, 0, kHeader, nullptr);
    LineTo(dc, kWidth, kHeader);
    MoveToEx(dc, 0, kHeight - 58, nullptr);
    LineTo(dc, kWidth, kHeight - 58);
    DeleteObject(pen);
}

LRESULT CALLBACK wnd_proc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(h, &ps);
        paint_header(dc);
        EndPaint(h, &ps);
        return 0;
    }
    case WM_CTLCOLORSTATIC: {
        HDC dc = reinterpret_cast<HDC>(wp);
        SetBkMode(dc, TRANSPARENT);
        const HWND c = reinterpret_cast<HWND>(lp);
        const std::wstring t = (c == app.iso_status || c == app.dkp_status) ? text_of(c) : std::wstring();
        if (!t.empty()) SetTextColor(dc, t[0] == L'✔' ? RGB(0, 120, 40) : RGB(180, 30, 30));
        return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_BTNFACE));
    }
    case WM_COMMAND: {
        const int id = LOWORD(wp);
        if (id == IdNext) {
            if (app.page == Welcome) { show_page(Setup); validate(); }
            else if (app.page == Setup) start_build();
            else if (app.page == Finish) DestroyWindow(h);
        } else if (id == IdBack && app.page == Setup) {
            show_page(Welcome);
        } else if (id == IdCancel) {
            if (app.building) { app.pipeline->cancel(); EnableWindow(app.cancel, FALSE); }
            else DestroyWindow(h);
        } else if (id == IdIsoBrowse) {
            const auto f = browse_file(L"PSP disc image (*.iso)\0*.iso\0All files\0*.*\0", false, text_of(app.iso));
            if (!f.empty()) SetWindowTextW(app.iso, f.c_str());
        } else if (id == IdDkpBrowse) {
            const auto f = browse_folder(L"Choose the devkitPro folder (contains devkitARM, libctru, tools)");
            if (!f.empty()) SetWindowTextW(app.dkp, f.c_str());
        } else if (id == IdOutBrowse) {
            const auto f = browse_file(L"3DS homebrew (*.3dsx)\0*.3dsx\0", true, text_of(app.out));
            if (!f.empty()) SetWindowTextW(app.out, f.c_str());
        } else if (id == IdDkpLink) {
            ShellExecuteW(h, L"open", L"https://devkitpro.org/wiki/Getting_Started", nullptr, nullptr, SW_SHOWNORMAL);
        } else if (id == IdNextFact) {
            next_fact();
        } else if (id == IdOpenFolder) {
            const auto dir = fs::path(text_of(app.out)).parent_path();
            ShellExecuteW(h, L"open", dir.wstring().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        } else if ((id == IdIso || id == IdDkp || id == IdOut) && HIWORD(wp) == EN_CHANGE && app.page == Setup) {
            validate();
        }
        return 0;
    }
    case WM_TIMER:
        if (wp == kFactTimer) next_fact();
        return 0;
    case WM_BUILD_PROGRESS: {
        std::unique_ptr<Progress> p(reinterpret_cast<Progress *>(lp));
        std::wstring step = widen(p->step);
        if (p->units_total) step += L"  (" + std::to_wstring(p->units_done) + L" / " + std::to_wstring(p->units_total) + L" files)";
        SetWindowTextW(app.step, step.c_str());
        SendMessageW(app.bar, PBM_SETPOS, static_cast<WPARAM>(p->fraction * 1000), 0);
        const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - app.started).count();
        std::wstring t = L"Elapsed " + minutes(elapsed);
        if (p->eta_seconds >= 0) t += L"   ·   about " + minutes(p->eta_seconds) + L" left for compiling";
        SetWindowTextW(app.time, t.c_str());
        return 0;
    }
    case WM_BUILD_LOG: {
        std::unique_ptr<std::string> line(reinterpret_cast<std::string *>(lp));
        const std::wstring w = widen(*line) + L"\r\n";
        const int n = GetWindowTextLengthW(app.log);
        SendMessageW(app.log, EM_SETSEL, n, n);
        SendMessageW(app.log, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(w.c_str()));
        return 0;
    }
    case WM_BUILD_DONE: {
        std::unique_ptr<std::string> err(reinterpret_cast<std::string *>(lp));
        if (app.worker.joinable()) app.worker.join();
        KillTimer(h, kFactTimer);
        app.building = false;
        const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - app.started).count();
        std::wstring text;
        if (err->empty()) {
            text = L"P3P3DS is ready (built in " + minutes(elapsed) + L").\n\n" + text_of(app.out) +
                   L"\n\nTo play on your New 3DS, put on the SD card:\n"
                   L"    \\3ds\\p3p3ds.3dsx\n"
                   L"    \\p3p3ds\\<your game>.iso\n"
                   L"and start P3P3DS from the Homebrew Launcher.\n\n"
                   L"Keep this file to yourself: it contains code made from your copy of the game.";
            ShowWindow(app.open_folder, SW_SHOW);
        } else {
            text = L"The build stopped.\n\n" + widen(*err) + L"\n\nDetails are in the work folder next to the builder.";
        }
        show_page(Finish);
        SetWindowTextW(app.finish_text, text.c_str());
        if (!err->empty()) ShowWindow(app.open_folder, SW_HIDE);
        return 0;
    }
    case WM_CLOSE:
        if (app.building) { app.pipeline->cancel(); return 0; }
        DestroyWindow(h);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(h, msg, wp, lp);
    }
}

int run_cli(int argc, wchar_t **argv) {
    // Keep a redirected stdout (file/pipe); otherwise write to the parent console.
    const HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    if (out == nullptr || out == INVALID_HANDLE_VALUE || GetFileType(out) == FILE_TYPE_UNKNOWN) {
        if (!AttachConsole(ATTACH_PARENT_PROCESS)) AllocConsole();
        std::freopen("CONOUT$", "w", stdout);
    }
    Settings s;
    s.sdk = app.exe_dir / "sdk";
    s.work = app.exe_dir / "work";
    s.output = app.exe_dir / "output" / "p3p3ds.3dsx";
    s.devkitpro = Pipeline::find_devkitpro();
    for (int i = 1; i + 1 < argc; ++i) {
        const std::wstring a = argv[i];
        if (a == L"--iso") s.iso = argv[++i];
        else if (a == L"--devkitpro") s.devkitpro = argv[++i];
        else if (a == L"--out") s.output = argv[++i];
        else if (a == L"--jobs") s.jobs = static_cast<unsigned>(_wtoi(argv[++i]));
        else if (a == L"--work") s.work = argv[++i];
    }
    const auto start = std::chrono::steady_clock::now();
    std::string last;
    Pipeline p(s, [&](const Progress &pr) {
        if (pr.step != last || pr.units_done % 20 == 0) {
            std::printf("[%5.1f%%] %s %u/%u eta=%.0fs\n", pr.fraction * 100, pr.step.c_str(), pr.units_done, pr.units_total, pr.eta_seconds);
            std::fflush(stdout);
            last = pr.step;
        }
    }, [](const std::string &l) { std::printf("%s\n", l.c_str()); std::fflush(stdout); });
    try {
        p.run();
    } catch (const std::exception &e) {
        std::printf("FAILED: %s\n", e.what());
        return 1;
    }
    std::printf("OK in %.0f s: %s\n", std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count(),
                s.output.string().c_str());
    return 0;
}

} // namespace

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE, PWSTR, int show) {
    app.inst = inst;
    wchar_t self[MAX_PATH];
    GetModuleFileNameW(nullptr, self, MAX_PATH);
    app.exe_dir = fs::path(self).parent_path();
    int argc = 0;
    wchar_t **argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    for (int i = 1; i < argc; ++i)
        if (std::wstring(argv[i]) == L"--cli") return run_cli(argc, argv);

    SetProcessDPIAware();
    INITCOMMONCONTROLSEX icc{sizeof icc, ICC_PROGRESS_CLASS | ICC_STANDARD_CLASSES};
    InitCommonControlsEx(&icc);
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    NONCLIENTMETRICSW ncm{};
    ncm.cbSize = sizeof ncm;
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof ncm, &ncm, 0);
    app.font = CreateFontIndirectW(&ncm.lfMessageFont);
    LOGFONTW lf = ncm.lfMessageFont;
    lf.lfWeight = FW_BOLD;
    app.bold = CreateFontIndirectW(&lf);
    lf.lfHeight = lf.lfHeight * 3 / 2;
    app.title = CreateFontIndirectW(&lf);

    WNDCLASSEXW wc{sizeof wc};
    wc.lpfnWndProc = wnd_proc;
    wc.hInstance = inst;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = GetSysColorBrush(COLOR_BTNFACE);
    wc.lpszClassName = L"P3P3DSBuilder";
    wc.hIcon = LoadIconW(inst, MAKEINTRESOURCEW(1));
    RegisterClassExW(&wc);
    RECT r{0, 0, kWidth, kHeight};
    const DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    AdjustWindowRect(&r, style, FALSE);
    app.wnd = CreateWindowExW(0, wc.lpszClassName, L"P3P3DS Builder", style, CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top,
                              nullptr, nullptr, inst, nullptr);
    create_controls();
    show_page(Welcome);
    ShowWindow(app.wnd, show);
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        if (!IsDialogMessageW(app.wnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    if (app.worker.joinable()) { app.pipeline->cancel(); app.worker.join(); }
    return 0;
}
