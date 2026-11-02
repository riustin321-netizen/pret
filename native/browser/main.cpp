#include "windows.hpp"
#include <commctrl.h>
#include <algorithm>
#include <memory>
#include <stdexcept>
#include <thread>

namespace hb = hellboost::browser;

namespace {
constexpr UINT fetched_message = WM_APP + 1;
constexpr int list_id = 101, tabs_id = 102, search_id = 103, refresh_id = 104;
constexpr int favorite_id = 105, local_id = 106, play_id = 107, name_id = 108;
constexpr int address_id = 109, windowed_id = 110, save_id = 111;
constexpr int steam_mode_id = 112;

std::wstring control_text(HWND control) {
    const int size = GetWindowTextLengthW(control);
    std::wstring result(static_cast<std::size_t>(size) + 1, L'\0');
    GetWindowTextW(control, result.data(), size + 1);
    result.resize(static_cast<std::size_t>(size));
    return result;
}

std::wstring trim(std::wstring value) {
    const auto first = value.find_first_not_of(L" \t\r\n");
    if (first == std::wstring::npos) return {};
    const auto last = value.find_last_not_of(L" \t\r\n");
    return value.substr(first, last - first + 1);
}

struct FetchResult {
    std::vector<hb::Server> servers;
    std::wstring error;
};

class Application {
public:
    HWND window = nullptr;
    HWND title = nullptr, subtitle = nullptr, search_label = nullptr, search = nullptr;
    HWND tabs = nullptr, list = nullptr, refresh = nullptr, favorite = nullptr, local = nullptr;
    HWND name_label = nullptr, name = nullptr, address_label = nullptr, address = nullptr;
    HWND windowed = nullptr, steam_mode = nullptr, save = nullptr, play = nullptr, status = nullptr, notice = nullptr;
    HFONT font = nullptr, title_font = nullptr;
    int dpi = 96;
    hb::Settings settings;
    std::vector<hb::Server> online, displayed;
    std::thread fetch_thread;
    std::atomic_bool cancelled{false};
    bool closing = false, loading = false, rendering = false, steam_dirty = false;

    ~Application() {
        cancelled.store(true);
        if (fetch_thread.joinable()) fetch_thread.join();
        if (font) DeleteObject(font);
        if (title_font) DeleteObject(title_font);
    }

    int scale(int value) const { return MulDiv(value, dpi, 96); }

    HWND create(const wchar_t* kind, const wchar_t* text, DWORD style, int id = 0, DWORD extended = 0) {
        HWND child = CreateWindowExW(extended, kind, text, WS_CHILD | WS_VISIBLE | style,
            0, 0, 10, 10, window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), GetModuleHandleW(nullptr), nullptr);
        if (!child) throw std::runtime_error("Nu pot crea interfata Windows");
        SendMessageW(child, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        return child;
    }

    void initialize() {
        settings = hb::load_settings();
        HDC dc = GetDC(window);
        dpi = GetDeviceCaps(dc, LOGPIXELSX);
        ReleaseDC(window, dc);
        font = CreateFontW(-scale(14), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        title_font = CreateFontW(-scale(30), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        title = create(L"STATIC", L"HELLBOOST", 0);
        SendMessageW(title, WM_SETFONT, reinterpret_cast<WPARAM>(title_font), TRUE);
        subtitle = create(L"STATIC", L"Client propriu / Browser extern 0.3 / Steam legacy 8684", 0);
        search_label = create(L"STATIC", L"Cauta server:", 0);
        search = create(L"EDIT", L"", ES_AUTOHSCROLL | WS_TABSTOP, search_id, WS_EX_CLIENTEDGE);
        SendMessageW(search, EM_SETLIMITTEXT, 128, 0);
        tabs = create(WC_TABCONTROLW, L"", WS_TABSTOP, tabs_id);
        int tab_index = 0;
        for (auto text : {L"Internet", L"Favorite", L"Istoric"}) {
            TCITEMW item{};
            item.mask = TCIF_TEXT;
            item.pszText = const_cast<wchar_t*>(text);
            TabCtrl_InsertItem(tabs, tab_index++, &item);
        }
        list = create(WC_LISTVIEWW, L"", LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | WS_TABSTOP, list_id, WS_EX_CLIENTEDGE);
        ListView_SetExtendedListViewStyle(list, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_LABELTIP);
        int column_index = 0;
        for (auto text : {L"#", L"Server", L"Jucatori", L"Harta", L"Boost", L"Tara", L"Adresa"}) {
            LVCOLUMNW column{};
            column.mask = LVCF_TEXT | LVCF_WIDTH;
            column.pszText = const_cast<wchar_t*>(text);
            column.cx = scale(column_index == 1 ? 260 : column_index == 6 ? 210 : 75);
            ListView_InsertColumn(list, column_index++, &column);
        }
        refresh = create(L"BUTTON", L"REINCARCA LISTA", WS_TABSTOP, refresh_id);
        favorite = create(L"BUTTON", L"+ / - FAVORIT", WS_TABSTOP, favorite_id);
        local = create(L"BUTTON", L"ALEGE HL.EXE", WS_TABSTOP, local_id);
        name_label = create(L"STATIC", L"Nume jucator", 0);
        name = create(L"EDIT", settings.nickname.c_str(), ES_AUTOHSCROLL | WS_TABSTOP, name_id, WS_EX_CLIENTEDGE);
        SendMessageW(name, EM_SETLIMITTEXT, 32, 0);
        address_label = create(L"STATIC", L"Server optional (IP:port sau domeniu:port)", 0);
        address = create(L"EDIT", L"", ES_AUTOHSCROLL | WS_TABSTOP, address_id, WS_EX_CLIENTEDGE);
        SendMessageW(address, EM_SETLIMITTEXT, 259, 0);
        windowed = create(L"BUTTON", L"In fereastra", BS_AUTOCHECKBOX | WS_TABSTOP, windowed_id);
        SendMessageW(windowed, BM_SETCHECK, settings.windowed ? BST_CHECKED : BST_UNCHECKED, 0);
        steam_mode = create(L"BUTTON", L"Integrare Steam (daca exista)", BS_AUTOCHECKBOX | WS_TABSTOP, steam_mode_id);
        SendMessageW(steam_mode, BM_SETCHECK, settings.prefer_steam ? BST_CHECKED : BST_UNCHECKED, 0);
        save = create(L"BUTTON", L"SALVEAZA", WS_TABSTOP, save_id);
        play = create(L"BUTTON", L"PORNESTE CS 1.6", BS_DEFPUSHBUTTON | WS_TABSTOP, play_id);
        status = create(L"STATIC", L"Pregatit. Reincarca lista pentru a contacta API-ul HellBoost.", 0);
        notice = create(L"STATIC", L"Browser extern HellBoost: Find Servers din joc si statisticile intrarilor nu sunt inca integrate.", 0);
        render();
        layout();
    }

    void layout() {
        if (!list) return;
        RECT client{};
        GetClientRect(window, &client);
        const int margin = scale(22);
        const int width = client.right - margin * 2;
        const int bottom = client.bottom;
        const auto move = [](HWND control, int x, int y, int w, int h) { MoveWindow(control, x, y, w, h, TRUE); };
        move(title, margin, scale(15), width, scale(42));
        move(subtitle, margin, scale(60), width, scale(24));
        move(search_label, margin, scale(97), scale(100), scale(24));
        move(search, margin + scale(104), scale(92), width - scale(104), scale(29));
        move(tabs, margin, scale(134), width, scale(30));
        const int controls_top = bottom - scale(234);
        move(list, margin, scale(166), width, std::max(scale(100), controls_top - scale(176)));
        move(refresh, margin, controls_top, scale(170), scale(32));
        move(favorite, margin + scale(180), controls_top, scale(150), scale(32));
        move(local, margin + scale(340), controls_top, scale(150), scale(32));
        move(name_label, margin, controls_top + scale(48), scale(210), scale(22));
        move(address_label, margin + scale(240), controls_top + scale(48), width - scale(240), scale(22));
        move(name, margin, controls_top + scale(71), scale(220), scale(29));
        move(address, margin + scale(240), controls_top + scale(71), width - scale(240), scale(29));
        move(windowed, margin, controls_top + scale(112), scale(140), scale(30));
        move(steam_mode, margin + scale(145), controls_top + scale(112), scale(240), scale(30));
        move(save, margin + scale(395), controls_top + scale(110), scale(110), scale(34));
        move(play, margin + scale(515), controls_top + scale(110), width - scale(515), scale(34));
        move(status, margin, controls_top + scale(158), width, scale(32));
        move(notice, margin, controls_top + scale(198), width, scale(30));
    }

    void set_status(const std::wstring& text) { SetWindowTextW(status, text.c_str()); }

    void render() {
        const int selected_tab = TabCtrl_GetCurSel(tabs);
        auto rows = selected_tab == 0 ? online : hb::saved_servers(selected_tab == 1 ? settings.favorites : settings.history, online);
        displayed = hb::filter_servers(rows, hb::utf8(control_text(search)));
        rendering = true;
        SendMessageW(list, WM_SETREDRAW, FALSE, 0);
        ListView_DeleteAllItems(list);
        for (std::size_t i = 0; i < displayed.size(); ++i) {
            const auto& server = displayed[i];
            const auto rank = server.known ? std::to_wstring(server.rank) : L"-";
            LVITEMW item{};
            item.mask = LVIF_TEXT;
            item.iItem = static_cast<int>(i);
            item.pszText = const_cast<wchar_t*>(rank.c_str());
            ListView_InsertItem(list, &item);
            const std::vector<std::wstring> cells = {
                hb::wide(server.name), server.known ? std::to_wstring(server.players) + L"/" + std::to_wstring(server.max_players) : L"-",
                hb::wide(server.map), hb::wide(server.promo), hb::wide(server.country), hb::wide(server.address)
            };
            for (std::size_t j = 0; j < cells.size(); ++j)
                ListView_SetItemText(list, static_cast<int>(i), static_cast<int>(j + 1), const_cast<wchar_t*>(cells[j].c_str()));
        }
        SendMessageW(list, WM_SETREDRAW, TRUE, 0);
        rendering = false;
        InvalidateRect(list, nullptr, TRUE);
    }

    void selected() {
        if (rendering) return;
        const int index = ListView_GetNextItem(list, -1, LVNI_SELECTED);
        if (index >= 0 && static_cast<std::size_t>(index) < displayed.size())
            SetWindowTextW(address, hb::wide(displayed[static_cast<std::size_t>(index)].address).c_str());
    }

    void sync_steam_choice() {
        if (!steam_mode || steam_dirty) return;
        settings.prefer_steam = hb::load_settings().prefer_steam;
        SendMessageW(steam_mode, BM_SETCHECK, settings.prefer_steam ? BST_CHECKED : BST_UNCHECKED, 0);
    }

    void read_fields() {
        sync_steam_choice();
        settings.nickname = trim(control_text(name));
        settings.windowed = SendMessageW(windowed, BM_GETCHECK, 0, 0) == BST_CHECKED;
        settings.prefer_steam = SendMessageW(steam_mode, BM_GETCHECK, 0, 0) == BST_CHECKED;
        if (!hb::valid_nickname(hb::utf8(settings.nickname)))
            throw std::runtime_error("Nume invalid: foloseste 1-32 litere, cifre, spatii sau . - _");
    }

    void save_fields() {
        read_fields();
        hb::save_settings(settings);
        steam_dirty = false;
        set_status(L"Setarile au fost salvate in profilul tau Windows.");
    }

    void toggle_favorite() {
        const auto endpoint = hb::utf8(trim(control_text(address)));
        if (!hb::valid_address(endpoint)) throw std::runtime_error("Alege un server sau introdu IP:port");
        auto copy = settings;
        const auto found = std::find(copy.favorites.begin(), copy.favorites.end(), endpoint);
        if (found != copy.favorites.end()) copy.favorites.erase(found);
        else {
            if (copy.favorites.size() >= 100) throw std::runtime_error("Limita este de 100 favorite");
            copy.favorites.push_back(endpoint);
        }
        hb::save_settings(copy, false);
        settings = std::move(copy);
        render();
        set_status(L"Favorite actualizate.");
    }

    void select_local() {
        const auto path = hb::choose_local_game(window);
        if (path.empty()) return;
        auto copy = settings;
        copy.local_game = path;
        hb::save_settings(copy, false);
        settings = std::move(copy);
        set_status(L"Instalare locala salvata. Modul de pornire se configureaza prin bifa Steam.");
    }

    void launch() {
        read_fields();
        const auto endpoint = hb::utf8(trim(control_text(address)));
        if (!endpoint.empty() && !hb::valid_address(endpoint)) throw std::runtime_error("Server invalid: foloseste domeniu:port sau IP:port");
        if ((!settings.prefer_steam || hb::find_steam().empty()) && hb::find_local_game(settings).empty()) {
            const auto path = hb::choose_local_game(window);
            if (path.empty()) return;
            settings.local_game = path;
        }
        hb::save_settings(settings);
        steam_dirty = false;
        const auto mode = hb::start_game(settings, endpoint);
        if (!endpoint.empty()) {
            settings.history.erase(std::remove(settings.history.begin(), settings.history.end(), endpoint), settings.history.end());
            settings.history.insert(settings.history.begin(), endpoint);
            if (settings.history.size() > 100) settings.history.resize(100);
                try { hb::save_settings(settings, false); }
            catch (const std::exception&) {
                set_status(L"Pornire solicitata; istoricul nu a putut fi salvat. Intrarea nu este confirmata.");
                render();
                return;
            }
            render();
        }
        set_status(L"Pornire solicitata prin " + mode + L". Intrarea efectiva in joc nu este inca monitorizata.");
    }

    void refresh_list() {
        if (loading) return;
        if (fetch_thread.joinable()) fetch_thread.join();
        cancelled.store(false);
        loading = true;
        EnableWindow(refresh, FALSE);
        set_status(L"Se incarca https://hellboost.net/api.php ...");
        try {
            fetch_thread = std::thread([this] {
                auto result = std::make_unique<FetchResult>();
                try { result->servers = hb::fetch_platform(cancelled); }
                catch (const std::exception& error) {
                    try { result->error = hb::wide(error.what()); }
                    catch (...) { result->error = L"Cererea API a esuat"; }
                }
                if (cancelled.load()) return;
                if (PostMessageW(window, fetched_message, 0, reinterpret_cast<LPARAM>(result.get()))) result.release();
            });
        } catch (...) {
            loading = false;
            EnableWindow(refresh, TRUE);
            throw;
        }
    }

    void receive(FetchResult* raw) {
        std::unique_ptr<FetchResult> result(raw);
        if (fetch_thread.joinable()) fetch_thread.join();
        loading = false;
        EnableWindow(refresh, TRUE);
        if (!result->error.empty()) { set_status(result->error + L". Lista precedenta este pastrata."); return; }
        online = std::move(result->servers);
        render();
        set_status(std::to_wstring(online.size()) + L" servere online, in ordinea returnata de platforma.");
    }

    void close() {
        if (closing) return;
        closing = true;
        cancelled.store(true);
        set_status(L"Se inchide cererea API...");
        if (fetch_thread.joinable()) fetch_thread.join();
        MSG message{};
        while (PeekMessageW(&message, window, fetched_message, fetched_message, PM_REMOVE))
            delete reinterpret_cast<FetchResult*>(message.lParam);
        DestroyWindow(window);
    }
};

LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    Application* app = reinterpret_cast<Application*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        app = static_cast<Application*>(reinterpret_cast<CREATESTRUCTW*>(lparam)->lpCreateParams);
        app->window = window;
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    }
    if (!app) return DefWindowProcW(window, message, wparam, lparam);
    try {
        switch (message) {
            case WM_CREATE: app->initialize(); return 0;
            case WM_SIZE: app->layout(); return 0;
            case WM_ACTIVATE:
                if (LOWORD(wparam) != WA_INACTIVE) app->sync_steam_choice();
                break;
            case WM_GETMINMAXINFO: {
                auto info = reinterpret_cast<MINMAXINFO*>(lparam);
                info->ptMinTrackSize.x = app->scale(820);
                info->ptMinTrackSize.y = app->scale(640);
                return 0;
            }
            case WM_COMMAND: {
                const int id = LOWORD(wparam);
                if (id == search_id && HIWORD(wparam) == EN_CHANGE) { if (app->list) app->render(); return 0; }
                if (HIWORD(wparam) != BN_CLICKED) break;
                switch (id) {
                    case steam_mode_id:
                        app->steam_dirty = true;
                        app->set_status(L"Preferinta Steam se aplica la urmatoarea pornire. Salveaza sau porneste jocul pentru aplicare.");
                        return 0;
                    case refresh_id: app->refresh_list(); return 0;
                    case favorite_id: app->toggle_favorite(); return 0;
                    case local_id: app->select_local(); return 0;
                    case play_id: app->launch(); return 0;
                    case save_id: app->save_fields(); return 0;
                }
                break;
            }
            case WM_NOTIFY: {
                auto header = reinterpret_cast<NMHDR*>(lparam);
                if (header->idFrom == tabs_id && header->code == TCN_SELCHANGE) { app->render(); return 0; }
                if (header->idFrom == list_id && header->code == LVN_ITEMCHANGED) { app->selected(); return 0; }
                if (header->idFrom == list_id && header->code == NM_DBLCLK) {
                    if (reinterpret_cast<NMITEMACTIVATE*>(lparam)->iItem >= 0) { app->selected(); app->launch(); }
                    return 0;
                }
                break;
            }
            case fetched_message: app->receive(reinterpret_cast<FetchResult*>(lparam)); return 0;
            case WM_CLOSE: app->close(); return 0;
            case WM_DESTROY: PostQuitMessage(0); return 0;
            case WM_CTLCOLORSTATIC: {
                HDC dc = reinterpret_cast<HDC>(wparam);
                SetBkMode(dc, TRANSPARENT);
                SetTextColor(dc, reinterpret_cast<HWND>(lparam) == app->title ? RGB(185, 32, 40) : RGB(35, 39, 47));
                return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_WINDOW));
            }
        }
    } catch (const std::exception& error) {
        std::wstring text;
        try { text = hb::wide(error.what()); } catch (...) { text = L"Eroare HellBoost"; }
        MessageBoxW(window, text.c_str(), L"HellBoost", MB_OK | MB_ICONERROR);
        if (message == WM_CREATE) return -1;
    }
    return DefWindowProcW(window, message, wparam, lparam);
}
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR arguments, int show) {
    if (std::wstring(arguments) == L"--check-transport") {
        try { hb::verify_platform_configuration(); return 0; }
        catch (...) { return 2; }
    }
    INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_LISTVIEW_CLASSES | ICC_TAB_CLASSES};
    if (!InitCommonControlsEx(&controls)) return 1;
    WNDCLASSEXW klass{};
    klass.cbSize = sizeof(klass);
    klass.lpfnWndProc = window_proc;
    klass.hInstance = instance;
    klass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    klass.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    klass.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
    klass.lpszClassName = L"HellBoostBrowser";
    if (!RegisterClassExW(&klass)) return 1;
    Application app;
    HWND window = CreateWindowExW(0, klass.lpszClassName, L"HellBoost - Client propriu 0.3", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1080, 740, nullptr, nullptr, instance, &app);
    if (!window) return 1;
    ShowWindow(window, show);
    UpdateWindow(window);
    MSG message{};
    int code = 0;
    while ((code = static_cast<int>(GetMessageW(&message, nullptr, 0, 0))) > 0) {
        if (!IsDialogMessageW(window, &message)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }
    return code == -1 ? 1 : static_cast<int>(message.wParam);
}
