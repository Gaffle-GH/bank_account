/*
 * Bank Account GUI — receipt-style native app (FLTK)
 * Visual match for web/index.html + web/styles.css
 */

#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Window.H>
#include <FL/fl_ask.H>
#include <FL/fl_draw.H>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "class.h"

using namespace std;

namespace
{
// --- CSS tokens (web/styles.css) ---
constexpr int kReceiptW = 364;
constexpr int kReceiptPadX = 24;
constexpr int kReceiptPadTop = 30;
constexpr int kPagePadX = 16;
constexpr int kPagePadTop = 44;
constexpr int kContentW = kReceiptW - 2 * kReceiptPadX; // 316

const Fl_Color kBgTop = fl_rgb_color(235, 229, 216);
const Fl_Color kBgBot = fl_rgb_color(221, 212, 197);
const Fl_Color kPaper = fl_rgb_color(255, 250, 241);
const Fl_Color kPaperTop = fl_rgb_color(255, 253, 248);
const Fl_Color kPaperEdge = fl_rgb_color(232, 223, 207);
const Fl_Color kInk = fl_rgb_color(25, 23, 19);
const Fl_Color kMuted = fl_rgb_color(120, 111, 99);
const Fl_Color kDash = fl_rgb_color(211, 200, 183);
const Fl_Color kAccent = fl_rgb_color(138, 45, 27);
const Fl_Color kAccentSoft = fl_rgb_color(138, 45, 27); // drawn with alpha
const Fl_Color kCredit = fl_rgb_color(31, 106, 63);
const Fl_Color kDebit = fl_rgb_color(154, 44, 44);
const Fl_Color kShadow = fl_rgb_color(50, 34, 16);

Fl_Font gFont = FL_COURIER;
Fl_Font gFontBold = FL_COURIER_BOLD;

void initFont()
{
    // FL_COURIER matches the web's "Courier New" fallback. FL::set_font with
    // "IBM Plex Mono" silently falls back to Helvetica on macOS when missing.
    gFont = FL_COURIER;
    gFontBold = FL_COURIER_BOLD;
}

constexpr int kReceiptRadius = 8;
constexpr int kInputRadius = 4;

int clampRadius(int r, int w, int h)
{
    return min(r, min(w, h) / 2);
}

void fillRoundedRect(int x, int y, int w, int h, int r)
{
    r = clampRadius(r, w, h);
    if (r <= 0)
    {
        fl_rectf(x, y, w, h);
        return;
    }

    fl_begin_polygon();
    fl_vertex(x + r, y);
    fl_vertex(x + w - r, y);
    fl_arc(x + w - 2 * r, y, 2 * r, 2 * r, 0.0, 90.0);
    fl_vertex(x + w, y + h - r);
    fl_arc(x + w - 2 * r, y + h - 2 * r, 2 * r, 2 * r, 90.0, 180.0);
    fl_vertex(x + r, y + h);
    fl_arc(x, y + h - 2 * r, 2 * r, 2 * r, 180.0, 270.0);
    fl_vertex(x, y + r);
    fl_arc(x, y, 2 * r, 2 * r, 270.0, 360.0);
    fl_end_polygon();
}

void fillTopRoundedRect(int x, int y, int w, int h, int r)
{
    r = clampRadius(r, w, h);
    if (r <= 0)
    {
        fl_rectf(x, y, w, h);
        return;
    }

    fl_begin_polygon();
    fl_vertex(x, y + h);
    fl_vertex(x, y + r);
    fl_arc(x, y, 2 * r, 2 * r, 180.0, 270.0);
    fl_vertex(x + w - r, y);
    fl_arc(x + w - 2 * r, y, 2 * r, 2 * r, 270.0, 360.0);
    fl_vertex(x + w, y + h);
    fl_end_polygon();
}

void strokeRoundedRect(int x, int y, int w, int h, int r)
{
    r = clampRadius(r, w, h);
    if (r <= 0)
    {
        fl_rect(x, y, w, h);
        return;
    }

    fl_begin_loop();
    fl_arc(x + w - 2 * r, y, 2 * r, 2 * r, 0.0, 90.0);
    fl_arc(x + w - 2 * r, y + h - 2 * r, 2 * r, 2 * r, 90.0, 180.0);
    fl_arc(x, y + h - 2 * r, 2 * r, 2 * r, 180.0, 270.0);
    fl_arc(x, y, 2 * r, 2 * r, 270.0, 360.0);
    fl_end_loop();
}

Fl_Color blend(Fl_Color a, Fl_Color b, float t)
{
    return fl_color_average(a, b, t);
}

int topRoundInset(int r, int rowFromTop)
{
    if (rowFromTop >= r)
    {
        return 0;
    }
    const double dy = static_cast<double>(r - rowFromTop);
    return static_cast<int>(ceil(static_cast<double>(r) - sqrt(static_cast<double>(r) * r - dy * dy)));
}

void drawVerticalGradientRounded(int x, int y, int w, int h, int r, Fl_Color top, Fl_Color bottom)
{
    r = clampRadius(r, w, h);
    for (int i = 0; i < h; ++i)
    {
        const float t = h <= 1 ? 0.f : static_cast<float>(i) / static_cast<float>(h - 1);
        const int inset = topRoundInset(r, i);
        fl_color(blend(top, bottom, t));
        fl_line(x + inset, y + i, x + w - 1 - inset, y + i);
    }
}

void drawVerticalGradient(int x, int y, int w, int h, Fl_Color top, Fl_Color bottom)
{
    for (int i = 0; i < h; ++i)
    {
        float t = h <= 1 ? 0.f : static_cast<float>(i) / static_cast<float>(h - 1);
        fl_color(blend(top, bottom, t));
        fl_line(x, y + i, x + w - 1, y + i);
    }
}

void drawPageBackground(int w, int h)
{
    drawVerticalGradient(0, 0, w, h, kBgTop, kBgBot);
}

void drawSpacedText(const char* text, int x, int y, int w, int h, int size, Fl_Font font,
                    Fl_Color color, float letterSpacingEm, Fl_Align align)
{
    fl_font(font, size);
    fl_color(color);

    string s(text);
    float spacing = size * letterSpacingEm;
    float totalW = 0.f;
    for (char c : s)
    {
        totalW += fl_width(c) + spacing;
    }
    totalW -= spacing;

    int startX = x;
    if (align & FL_ALIGN_CENTER)
    {
        startX = x + static_cast<int>((w - totalW) / 2.f);
    }
    else if (align & FL_ALIGN_RIGHT)
    {
        startX = x + static_cast<int>(w - totalW);
    }

    int cx = startX;
    for (char c : s)
    {
        char buf[2] = {c, '\0'};
        fl_draw(buf, cx, y + h - (h - fl_height()) / 2 - 2);
        cx += static_cast<int>(fl_width(c) + spacing);
    }
}

class PageBackground : public Fl_Widget
{
public:
    PageBackground(int w, int h) : Fl_Widget(0, 0, w, h) { box(FL_NO_BOX); }

    void draw() override
    {
        drawPageBackground(w(), h());
    }
};

class StoreTitle : public Fl_Widget
{
public:
    StoreTitle(int x, int y, int w, int h) : Fl_Widget(x, y, w, h) { box(FL_NO_BOX); }

    void draw() override
    {
        drawSpacedText("ACCOUNT PROGRAM", x(), y(), w(), h(), 17, gFontBold, kInk, 0.14f,
                       FL_ALIGN_CENTER);
    }
};

void drawReceiptShadow(int x, int y, int w, int h)
{
    for (int layer = 5; layer >= 1; --layer)
    {
        float t = static_cast<float>(layer) / 5.f;
        int offY = static_cast<int>(16.f * t);
        int spread = static_cast<int>(4.f * t);
        uchar alpha = static_cast<uchar>(30.f * t);
        fl_color(fl_color_average(kShadow, FL_WHITE, 1.f - alpha / 255.f));
        fillTopRoundedRect(x - spread / 2, y + offY, w + spread, h, kReceiptRadius);
    }
}

void drawTornEdge(int x, int y, int w)
{
    constexpr int tooth = 8;
    fl_color(kPaperEdge);
    fl_rectf(x, y, w, tooth);

    for (int i = 0; i < w; i += tooth * 2)
    {
        fl_color(kPaper);
        fl_begin_polygon();
        fl_vertex(x + i, y);
        fl_vertex(x + i + tooth, y);
        fl_vertex(x + i + tooth, y + tooth);
        fl_end_polygon();

        fl_begin_polygon();
        fl_vertex(x + i + tooth, y);
        fl_vertex(x + i + tooth * 2, y);
        fl_vertex(x + i + tooth, y + tooth);
        fl_end_polygon();
    }
}

class ReceiptChrome : public Fl_Widget
{
public:
    ReceiptChrome(int x, int y, int w, int h) : Fl_Widget(x, y, w, h) { box(FL_NO_BOX); }

    void draw() override
    {
        const int bodyH = h() - 18;
        drawReceiptShadow(x(), y(), w(), bodyH);

        drawVerticalGradientRounded(x(), y(), w(), bodyH, kReceiptRadius, kPaperTop, kPaper);

        fl_color(fl_color_average(FL_BLACK, FL_WHITE, 0.95f));
        fl_line(x() + 10, y() + 12, x() + w() - 10, y() + 12);

        drawTornEdge(x(), y() + bodyH, w());
    }
};

class SectionTitle : public Fl_Widget
{
public:
    SectionTitle(int x, int y, int w, const char* title) : Fl_Widget(x, y, w, 14)
    {
        box(FL_NO_BOX);
        string upper = title;
        transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
        title_ = std::move(upper);
    }

    void draw() override
    {
        drawSpacedText(title_.c_str(), x(), y(), w(), h(), 11, gFont, kMuted, 0.16f, FL_ALIGN_LEFT);
    }

private:
    string title_;
};

class MutedCaption : public Fl_Widget
{
public:
    MutedCaption(int x, int y, int w, int h, const char* text, float spacingEm = 0.08f)
        : Fl_Widget(x, y, w, h), text_(text), spacingEm_(spacingEm)
    {
        box(FL_NO_BOX);
    }

    void draw() override
    {
        drawSpacedText(text_.c_str(), x(), y(), w(), h(), 11, gFont, kMuted, spacingEm_, FL_ALIGN_CENTER);
    }

private:
    string text_;
    float spacingEm_;
};

class DashRule : public Fl_Widget
{
public:
    DashRule(int x, int y, int w) : Fl_Widget(x, y, w, 1) {}

    void draw() override
    {
        fl_line_style(FL_DOT);
        fl_color(kDash);
        fl_line(x(), y(), x() + w(), y());
        fl_line_style(0);
    }
};

class ReceiptButton : public Fl_Button
{
public:
    ReceiptButton(int x, int y, int w, int h, const char* label, bool outline = false)
        : Fl_Button(x, y, w, h, label), outline_(outline)
    {
        box(FL_FLAT_BOX);
        labelsize(13);
        labelfont(gFontBold);
    }

    void draw() override
    {
        if (outline_)
        {
            fl_color(kPaper);
            fl_rectf(x(), y(), w(), h());
            fl_color(kInk);
            fl_rect(x(), y(), w(), h());
            fl_color(kInk);
        }
        else
        {
            fl_color(kInk);
            fl_rectf(x(), y(), w(), h());
            fl_color(kPaper);
        }
        fl_font(labelfont(), labelsize());
        fl_draw(label(), x(), y(), w(), h(), FL_ALIGN_CENTER);
    }

private:
    bool outline_;
};

class ReceiptInput : public Fl_Input
{
public:
    ReceiptInput(int x, int y, int w, int h, const char* placeholder = "0.00")
        : Fl_Input(x, y, w, h), placeholder_(placeholder)
    {
        box(FL_NO_BOX);
        textfont(gFont);
        textsize(14);
        color(kPaper);
        textcolor(kInk);
        selection_color(fl_color_average(kAccentSoft, kPaper, 0.7f));
    }

    void draw() override
    {
        const bool focused = Fl::focus() == this;
        const Fl_Color fill = fl_color_average(FL_WHITE, kPaper, 0.12f);

        if (focused)
        {
            fl_color(fl_color_average(kAccentSoft, kPaper, 0.88f));
            fillRoundedRect(x() - 3, y() - 3, w() + 6, h() + 6, kInputRadius + 2);
            fl_color(kAccent);
            strokeRoundedRect(x(), y(), w(), h(), kInputRadius);
            fl_color(FL_WHITE);
            fillRoundedRect(x() + 1, y() + 1, w() - 2, h() - 2, max(1, kInputRadius - 1));
        }
        else
        {
            fl_color(fill);
            fillRoundedRect(x() + 1, y() + 1, w() - 2, h() - 2, kInputRadius);
            fl_line_style(FL_DOT);
            fl_color(kDash);
            strokeRoundedRect(x(), y(), w(), h(), kInputRadius);
            fl_line_style(0);
        }

        const int tx = x() + 12;
        const int ty = y() + 2;
        const int tw = w() - 16;
        const int th = h() - 4;

        if ((value() == nullptr || value()[0] == '\0') && !focused)
        {
            fl_font(textfont(), textsize());
            fl_color(kMuted);
            fl_draw(placeholder_, tx, ty, tw, th, FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
            return;
        }

        fl_push_clip(tx, ty, tw, th);
        drawtext(tx, ty, tw, th, focused);
        fl_pop_clip();
    }

private:
    const char* placeholder_;
};

struct HistoryLine
{
    string label;
    string detail;
    string amount;
    bool credit = false;
};

static HistoryLine parseHistoryEntry(const string& entry)
{
    HistoryLine line;
    bool isDeposit = entry.rfind("Deposit:", 0) == 0;
    bool isWithdraw = entry.rfind("Withdraw:", 0) == 0;
    line.label = isDeposit ? "Deposit" : isWithdraw ? "Withdraw" : "Entry";
    line.detail = entry;
    line.credit = isDeposit;

    size_t dollar = entry.find('$');
    if (dollar != string::npos)
    {
        line.amount = entry.substr(dollar);
    }
    return line;
}

class HistoryViewport : public Fl_Widget
{
public:
    HistoryViewport(int x, int y, int w, int h) : Fl_Widget(x, y, w, h)
    {
        box(FL_NO_BOX);
    }

    void setEntries(const vector<string>& entries)
    {
        entries_ = entries;
        scrollY_ = maxScroll();
        redraw();
    }

    int contentHeight() const
    {
        return entries_.empty() ? 40 : static_cast<int>(entries_.size()) * kRowH;
    }

    void scrollToBottom()
    {
        scrollY_ = maxScroll();
        redraw();
    }

    int handle(int event) override
    {
        if (event == FL_MOUSEWHEEL)
        {
            scrollY_ += Fl::event_dy() * 18;
            scrollY_ = max(0, min(scrollY_, maxScroll()));
            redraw();
            return 1;
        }
        return Fl_Widget::handle(event);
    }

    void draw() override
    {
        fl_push_clip(x(), y(), w(), h());
        fl_color(kPaper);
        fl_rectf(x(), y(), w(), h());

        if (entries_.empty())
        {
            fl_font(gFont, 12);
            fl_color(kMuted);
            fl_draw("No transactions on record", x(), y(), w(), h(), FL_ALIGN_CENTER);
            fl_pop_clip();
            return;
        }

        int rowY = y() - scrollY_;
        for (size_t i = 0; i < entries_.size(); ++i)
        {
            const HistoryLine line = parseHistoryEntry(entries_[i]);

            if (rowY + kRowH >= y() && rowY <= y() + h())
            {
                fl_font(gFont, 12);
                fl_color(kInk);
                fl_draw(line.label.c_str(), x(), rowY + 9, w() - 88, 14,
                        FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

                fl_font(gFont, 12);
                fl_color(kInk);
                fl_draw(line.detail.c_str(), x(), rowY + 24, w() - 88, 14,
                        FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);

                fl_font(gFontBold, 12);
                fl_color(line.credit ? kCredit : kDebit);
                fl_draw(line.amount.c_str(), x(), rowY + 9, w() - 4, 14,
                        FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);

                if (i + 1 < entries_.size())
                {
                    fl_line_style(FL_DOT);
                    fl_color(kDash);
                    fl_line(x(), rowY + kRowH - 1, x() + w(), rowY + kRowH - 1);
                    fl_line_style(0);
                }
            }

            rowY += kRowH;
        }

        fl_pop_clip();
    }

private:
    static constexpr int kRowH = 46;

    vector<string> entries_;
    int scrollY_ = 0;

    int maxScroll() const
    {
        return max(0, contentHeight() - h());
    }
};

static Fl_Box* makeLabel(int x, int y, int w, int h, const char* text, int size, Fl_Color color,
                         Fl_Font font, Fl_Align align = FL_ALIGN_LEFT | FL_ALIGN_INSIDE)
{
    auto* box = new Fl_Box(x, y, w, h);
    box->box(FL_NO_BOX);
    if (text != nullptr && text[0] != '\0')
    {
        box->copy_label(text);
    }
    box->labelcolor(color);
    box->labelsize(size);
    box->labelfont(font);
    box->align(align);
    return box;
}

struct BankGui
{
    unique_ptr<account> user;
    Fl_Window* window = nullptr;
    Fl_Box* accountTitle = nullptr;
    Fl_Box* subtitle = nullptr;
    Fl_Box* balanceValue = nullptr;
    HistoryViewport* history = nullptr;
    Fl_Input* amountInput = nullptr;
};

static string formatMoney(double value)
{
    ostringstream ss;
    ss << fixed << setprecision(2) << value;
    return ss.str();
}

static double parseAmount(const char* text)
{
    if (text == nullptr || *text == '\0')
    {
        return -1.0;
    }
    try
    {
        size_t consumed = 0;
        double value = stod(text, &consumed);
        return consumed == 0 ? -1.0 : value;
    }
    catch (...)
    {
        return -1.0;
    }
}

static void refreshDisplay(BankGui* app)
{
    if (!app->user)
    {
        return;
    }

    app->accountTitle->copy_label(("Account Display for " + app->user->getName() + ":").c_str());
    app->balanceValue->copy_label(("$" + formatMoney(app->user->getBalance())).c_str());

    app->history->setEntries(app->user->getHistory());
    app->history->scrollToBottom();
    app->window->redraw();
}

static void onDeposit(Fl_Widget*, void* data)
{
    auto* app = static_cast<BankGui*>(data);
    double amount = parseAmount(app->amountInput->value());
    if (!app->user->deposit(amount))
    {
        fl_alert("Enter a valid amount greater than zero.");
        return;
    }
    app->amountInput->value("");
    refreshDisplay(app);
}

static void onWithdraw(Fl_Widget*, void* data)
{
    auto* app = static_cast<BankGui*>(data);
    double amount = parseAmount(app->amountInput->value());
    string errorMsg;
    if (!app->user->withdraw(amount, errorMsg))
    {
        fl_alert("%s", errorMsg.c_str());
        return;
    }
    app->amountInput->value("");
    refreshDisplay(app);
}

static int receiptLeft()
{
    return kPagePadX;
}

static void addReceiptHeader(int x, int& y, int w, const char* tagline, const char* code)
{
    new StoreTitle(x, y, w, 20);
    y += 22;

    new MutedCaption(x, y, w, 14, tagline);
    y += 17;
    new MutedCaption(x, y, w, 14, code);
    y += 20;

    new DashRule(x, y, w);
    y += 17;
}

static void addSectionTitle(int x, int& y, int w, const char* title)
{
    new SectionTitle(x, y, w, title);
    y += 21;
}

static unique_ptr<BankGui> buildMainWindow(const string& accountName, bool loadedExisting)
{
    auto app = make_unique<BankGui>();
    app->user = make_unique<account>(accountName, 0);
    if (loadedExisting)
    {
        app->user->loadFile();
    }

    const int winW = kReceiptW + 2 * kPagePadX;
    const int rx = receiptLeft();
    const int ry = kPagePadTop;

    app->window = new Fl_Window(winW, 400, "Account Program");
    app->window->color(kBgTop);
    app->window->size_range(winW, 400, winW, 1200);

    new PageBackground(winW, 400);

    auto* chrome = new ReceiptChrome(rx, ry, kReceiptW, 700);

    int x = rx + kReceiptPadX;
    int y = ry + kReceiptPadTop;
    const int w = kContentW;

    addReceiptHeader(x, y, w, "Transaction receipt", "BANK LEDGER  /  LIVE ACCOUNT");

    app->accountTitle = makeLabel(x, y, w, 18, "", 13, kInk, gFont, FL_ALIGN_CENTER);
    y += 20;
    app->subtitle = makeLabel(x, y, w, 36, "", 13, kMuted, gFont,
                                FL_ALIGN_CENTER | FL_ALIGN_WRAP | FL_ALIGN_INSIDE);
    app->subtitle->copy_label(loadedExisting
                                  ? "File Found... Loaded Existing Account Information."
                                  : "No Existing Information found. Creating New Account...");
    y += 40;

    new DashRule(x, y, w);
    y += 17;

    makeLabel(x, y, w / 2, 18, "Current Balance", 13, kInk, gFont);
    app->balanceValue = makeLabel(x + w / 2, y - 2, w / 2, 22, "$0.00", 20, kInk, gFontBold,
                                  FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    y += 28;

    new DashRule(x, y, w);
    y += 17;

    addSectionTitle(x, y, w, "Options");

    makeLabel(x, y, w, 14, "Enter Amount", 12, kMuted, gFont);
    y += 20;

    app->amountInput = new ReceiptInput(x, y, w, 38);
    y += 50;

    auto* depositBtn = new ReceiptButton(x, y, w, 36, "1. Deposit");
    depositBtn->callback(onDeposit, app.get());
    y += 44;

    auto* withdrawBtn = new ReceiptButton(x, y, w, 36, "2. Withdraw", true);
    withdrawBtn->callback(onWithdraw, app.get());
    y += 44;

    new DashRule(x, y, w);
    y += 17;

    addSectionTitle(x, y, w, "3. History");

    app->history = new HistoryViewport(x, y, w, 220);
    y += 228;

    new DashRule(x, y, w);
    y += 17;

    auto* saveBtn = new ReceiptButton(x, y, w, 36, "4. Save and Exit");
    saveBtn->callback(
        [](Fl_Widget*, void* data)
        {
            auto* app = static_cast<BankGui*>(data);
            app->user->saveFile();
            app->window->hide();
        },
        app.get());
    y += 44;

    makeLabel(x, y, w, 16, "Thank you for banking with us", 11, kInk, gFont, FL_ALIGN_CENTER);
    y += 18;
    makeLabel(x, y, w, 14, "*** END OF RECEIPT ***", 11, kMuted, gFont, FL_ALIGN_CENTER);
    y += 22;

    const int receiptH = y - ry + 18;
    const int winH = y + 64;

    chrome->size(kReceiptW, receiptH);

    app->window->size(winW, winH);
    if (auto* bg = app->window->child(0))
    {
        bg->size(winW, winH);
    }

    app->window->end();
    refreshDisplay(app.get());
    app->window->show();
    Fl::focus(app->window);

    return app;
}

static bool runLoginDialog(string& accountNameOut)
{
    const int winW = kReceiptW + 2 * kPagePadX;
    const int winH = 430;

    const int rx = receiptLeft();
    const int ry = kPagePadTop;

    Fl_Window login(winW, winH, "Account Program");
    login.color(kBgTop);

    new PageBackground(winW, winH);

    auto* chrome = new ReceiptChrome(rx, ry, kReceiptW, 360);

    int x = rx + kReceiptPadX;
    int y = ry + kReceiptPadTop;
    int w = kContentW;

    addReceiptHeader(x, y, w, "Client access terminal", "TERMINAL 04  /  SESSION OPEN");

    addSectionTitle(x, y, w, "New session");

    makeLabel(x, y, w, 14, "Insert Name for the Account", 12, kMuted, gFont);
    y += 20;

    ReceiptInput nameInput(x, y, w, 38, "");
    nameInput.value("exampleAccount");
    y += 50;

    Fl_Box loginStatus(x, y, w, 16);
    loginStatus.box(FL_NO_BOX);
    loginStatus.labelcolor(kMuted);
    loginStatus.labelsize(12);
    loginStatus.labelfont(gFont);
    loginStatus.align(FL_ALIGN_CENTER);
    loginStatus.copy_label("");
    y += 22;

    bool clicked = false;
    ReceiptButton continueBtn(x, y, w, 36, "Continue");
    continueBtn.callback(
        [](Fl_Widget*, void* data)
        {
            *static_cast<bool*>(data) = true;
        },
        &clicked);

    y += 44;
    makeLabel(x, y, w, 14, "Retain for your records", 11, kInk, gFont, FL_ALIGN_CENTER);
    y += 20;

    chrome->size(kReceiptW, y - ry + 18);

    login.end();
    login.set_modal();
    login.show();

    while (login.shown())
    {
        if (clicked)
        {
            accountNameOut = nameInput.value();
            if (accountNameOut.empty())
            {
                loginStatus.labelcolor(kDebit);
                loginStatus.copy_label("Name is required");
                clicked = false;
            }
            else
            {
                login.hide();
                return true;
            }
        }
        Fl::wait();
    }

    return false;
}

} // namespace

int main(int argc, char* argv[])
{
    Fl::args(argc, argv);
    initFont();

    string accountName;
    if (!runLoginDialog(accountName))
    {
        return 1;
    }

    account probe(accountName, 0);
    bool loadedExisting = probe.loadFile();

    auto app = buildMainWindow(accountName, loadedExisting);

    return Fl::run();
}
