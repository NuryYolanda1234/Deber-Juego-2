#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cwchar>
#include <random>
#include <string>
#include <vector>

namespace {
constexpr int WINDOW_W = 1120;
constexpr int WINDOW_H = 790;
constexpr int CARD_COUNT = 3;
constexpr UINT TIMER_ID = 1;
constexpr COLORREF INK = RGB(72, 56, 91);
constexpr COLORREF PURPLE = RGB(137, 104, 190);
constexpr COLORREF PINK = RGB(242, 160, 190);
constexpr COLORREF GOLD = RGB(247, 192, 71);

enum class Screen { Menu, Instructions, Quiz, Results, ChallengeIntro };
enum class Activity { Colors, Shapes, Dress, Sort, Count, Challenge };
enum class Shape { Circle, Square, Triangle, Crown, Flower, Star, Gift, Ball, Dress, Cape, Shoe };

struct Question {
    std::wstring prompt;
    std::wstring object;
    std::array<std::wstring, CARD_COUNT> labels;
    std::array<COLORREF, CARD_COUNT> colors;
    std::array<Shape, CARD_COUNT> shapes;
    int correct = 0;
    bool numberOptions = false;
    bool dropMode = false;
};

struct Sparkle { int x; int y; int life; };

HWND g_window = nullptr;
Screen g_screen = Screen::Menu;
Activity g_activity = Activity::Colors;
Question g_question;
std::vector<Question> g_questions;
std::vector<Sparkle> g_sparkles;
int g_questionIndex = 0;
int g_correct = 0;
int g_attempts = 0;
int g_lastAnswer = -1;
int g_score = 0;
int g_stars = 0;
bool g_answerLocked = false;
bool g_dragging = false;
POINT g_mouse{};
std::wstring g_message;
std::mt19937 g_random{ std::random_device{}() };

COLORREF colorFor(const std::wstring& name) {
    if (name == L"Rojo") return RGB(238, 99, 114);
    if (name == L"Azul") return RGB(91, 156, 220);
    if (name == L"Amarillo") return RGB(255, 205, 70);
    if (name == L"Verde") return RGB(109, 190, 139);
    if (name == L"Morado") return RGB(172, 130, 207);
    if (name == L"Rosa") return RGB(242, 160, 190);
    if (name == L"Dorado") return GOLD;
    return RGB(226, 192, 166);
}

void rounded(HDC dc, RECT r, COLORREF fill, COLORREF edge = RGB(255, 255, 255), int radius = 20) {
    HBRUSH brush = CreateSolidBrush(fill);
    HPEN pen = CreatePen(PS_SOLID, 2, edge);
    auto oldBrush = SelectObject(dc, brush);
    auto oldPen = SelectObject(dc, pen);
    RoundRect(dc, r.left, r.top, r.right, r.bottom, radius, radius);
    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);
}

void text(HDC dc, const std::wstring& value, RECT r, int size, COLORREF color = INK,
    bool bold = false, UINT alignment = DT_CENTER | DT_VCENTER | DT_SINGLELINE) {
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, color);
    HFONT font = CreateFontW(-size, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL,
        FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS,
        L"Trebuchet MS");
    HGDIOBJ oldFont = SelectObject(dc, font);
    DrawTextW(dc, value.c_str(), -1, &r, alignment);
    SelectObject(dc, oldFont);
    DeleteObject(font);
}

void ellipse(HDC dc, RECT r, COLORREF fill, COLORREF edge = RGB(255,255,255), int width = 2) {
    HBRUSH brush = CreateSolidBrush(fill);
    HPEN pen = CreatePen(PS_SOLID, width, edge);
    auto oldBrush = SelectObject(dc, brush);
    auto oldPen = SelectObject(dc, pen);
    Ellipse(dc, r.left, r.top, r.right, r.bottom);
    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);
}

void polygon(HDC dc, const std::vector<POINT>& points, COLORREF fill, COLORREF edge = RGB(255,255,255)) {
    HBRUSH brush = CreateSolidBrush(fill);
    HPEN pen = CreatePen(PS_SOLID, 2, edge);
    auto oldBrush = SelectObject(dc, brush);
    auto oldPen = SelectObject(dc, pen);
    Polygon(dc, points.data(), static_cast<int>(points.size()));
    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);
}

void drawCrown(HDC dc, int cx, int cy, int size, COLORREF color = GOLD) {
    polygon(dc, { {cx-size/2,cy}, {cx-size/2,cy-size/4}, {cx-size/5,cy-size/10},
                  {cx,cy-size/2}, {cx+size/5,cy-size/10}, {cx+size/2,cy-size/4},
                  {cx+size/2,cy}, {cx-size/2,cy} }, color, RGB(220, 157, 43));
    RECT band{cx-size/2, cy-size/10, cx+size/2, cy+size/5};
    rounded(dc, band, color, RGB(220,157,43), 8);
    for (int i = -1; i <= 1; ++i)
        ellipse(dc, {cx+i*size/4-3, cy-size/5-3, cx+i*size/4+4, cy-size/5+4},
            RGB(255,255,255), RGB(255,255,255), 1);
}

void drawShape(HDC dc, Shape shape, int cx, int cy, int size, COLORREF color) {
    const int h = size / 2;
    switch (shape) {
    case Shape::Circle:
    case Shape::Ball:
        ellipse(dc, {cx-h,cy-h,cx+h,cy+h}, color);
        if (shape == Shape::Ball) {
            HPEN p = CreatePen(PS_SOLID, 2, RGB(255,255,255));
            auto old = SelectObject(dc, p);
            Arc(dc,cx-h/2,cy-h,cx+h/2,cy+h,cx-h/2,cy-h,cx+h/2,cy+h);
            SelectObject(dc,old); DeleteObject(p);
        }
        break;
    case Shape::Square:
        rounded(dc, {cx-h,cy-h,cx+h,cy+h}, color, RGB(255,255,255), 10);
        break;
    case Shape::Triangle:
        polygon(dc, {{cx,cy-h},{cx+h,cy+h},{cx-h,cy+h}},color);
        break;
    case Shape::Crown:
        drawCrown(dc,cx,cy,size,color);
        break;
    case Shape::Star: {
        std::vector<POINT> pts;
        for (int i=0;i<10;++i) {
            const double a = -1.57079632679 + i*3.14159265359/5.0;
            const int r = i%2 ? size/4 : size/2;
            pts.push_back({cx+static_cast<int>(r*cos(a)),cy+static_cast<int>(r*sin(a))});
        }
        polygon(dc,pts,color);
        break;
    }
    case Shape::Flower:
        for (int i=0;i<5;++i) {
            const double a=i*1.25663706144;
            int x=cx+static_cast<int>(size*.23*cos(a)), y=cy+static_cast<int>(size*.23*sin(a));
            ellipse(dc,{x-size/5,y-size/5,x+size/5,y+size/5},color);
        }
        ellipse(dc,{cx-size/8,cy-size/8,cx+size/8,cy+size/8},GOLD);
        break;
    case Shape::Gift:
        rounded(dc,{cx-h,cy-h,cx+h,cy+h},color);
        rounded(dc,{cx-size/10,cy-h,cx+size/10,cy+h},GOLD, GOLD, 2);
        break;
    case Shape::Dress:
        polygon(dc,{{cx-size/5,cy-h},{cx+size/5,cy-h},{cx+size/3,cy-size/5},
            {cx+h,cy+h},{cx-h,cy+h},{cx-size/3,cy-size/5}},color);
        break;
    case Shape::Cape:
        polygon(dc,{{cx-size/4,cy-h},{cx+size/4,cy-h},{cx+h,cy+h},
            {cx,cy+size/3},{cx-h,cy+h}},color);
        break;
    case Shape::Shoe:
        polygon(dc,{{cx-h,cy+size/6},{cx+size/6,cy+size/6},{cx+size/3,cy-size/6},
            {cx+size/2,cy+size/6},{cx+h,cy+h},{cx-h,cy+h}},color);
        break;
    }
}

void drawCharacter(HDC dc, int cx, int cy, bool princess, int scale = 1) {
    const int s=scale;
    ellipse(dc,{cx-45*s,cy-105*s,cx+45*s,cy-15*s},
        princess?RGB(255,218,177):RGB(246,207,167),RGB(255,255,255),3);
    ellipse(dc,{cx-47*s,cy-116*s,cx+47*s,cy-45*s},
        princess?RGB(117,72,53):RGB(100,65,50),RGB(117,72,53),1);
    ellipse(dc,{cx-18*s,cy-78*s,cx-10*s,cy-70*s},INK,INK,1);
    ellipse(dc,{cx+10*s,cy-78*s,cx+18*s,cy-70*s},INK,INK,1);
    Arc(dc,cx-13*s,cy-69*s,cx+13*s,cy-53*s,cx-13*s,cy-61*s,cx+13*s,cy-61*s);
    if(princess) {
        polygon(dc,{{cx-29*s,cy-109*s},{cx-22*s,cy-142*s},{cx-5*s,cy-122*s},
                    {cx+4*s,cy-151*s},{cx+18*s,cy-120*s},{cx+31*s,cy-138*s},
                    {cx+36*s,cy-105*s}},GOLD);
        polygon(dc,{{cx-68*s,cy+55*s},{cx-44*s,cy-20*s},{cx+44*s,cy-20*s},
                    {cx+68*s,cy+55*s}},RGB(239,132,180));
        ellipse(dc,{cx-78*s,cy+39*s,cx-52*s,cy+65*s},GOLD);
        ellipse(dc,{cx+52*s,cy+39*s,cx+78*s,cy+65*s},GOLD);
    } else {
        drawCrown(dc,cx,cy-112*s,56*s);
        rounded(dc,{cx-54*s,cy-20*s,cx+54*s,cy+55*s},RGB(113,158,221),RGB(255,255,255),15);
        polygon(dc,{{cx-45*s,cy+55*s},{cx-56*s,cy+104*s},{cx-18*s,cy+104*s},
                    {cx,cy+63*s},{cx+18*s,cy+104*s},{cx+56*s,cy+104*s},
                    {cx+45*s,cy+55*s}},RGB(117,84,153));
    }
}

void drawCastle(HDC dc, int top = 175) {
    RECT lawn{0, 560, WINDOW_W, WINDOW_H};
    HBRUSH grass=CreateSolidBrush(RGB(220,241,222)); FillRect(dc,&lawn,grass); DeleteObject(grass);
    RECT body{358,top+65,762,610};
    rounded(dc,body,RGB(255,224,218),RGB(255,255,255),30);
    rounded(dc,{382,top+47,448,top+210},RGB(241,189,200),RGB(255,255,255),24);
    rounded(dc,{672,top+47,738,top+210},RGB(196,218,245),RGB(255,255,255),24);
    rounded(dc,{324,top+5,462,top+83},RGB(246,204,213),RGB(255,255,255),20);
    rounded(dc,{658,top+5,796,top+83},RGB(195,218,243),RGB(255,255,255),20);
    rounded(dc,{500,top+14,620,top+85},RGB(248,216,168),RGB(255,255,255),18);
    polygon(dc,{{324,top+6},{392,top-55},{462,top+6}},RGB(219,143,173));
    polygon(dc,{{658,top+6},{727,top-55},{796,top+6}},RGB(145,172,216));
    polygon(dc,{{493,top+19},{560,top-52},{627,top+19}},RGB(231,181,103));
    ellipse(dc,{495,top+225,625,top+358},RGB(180,153,211),RGB(255,255,255),3);
    rounded(dc,{523,top+346,597,610},RGB(181,152,211),RGB(255,255,255),32);
    for(int x: {399,691}) {
        rounded(dc,{x-19,top+99,x+19,top+155},RGB(255,248,223),RGB(255,255,255),18);
        rounded(dc,{x-19,top+242,x+19,top+298},RGB(255,248,223),RGB(255,255,255),18);
    }
    for(int i=0;i<5;++i) {
        int x=115+i*205;
        drawShape(dc,Shape::Flower,x,625,30, i%2?RGB(242,160,190):RGB(255,205,70));
    }
}

RECT buttonRect(int index) {
    const int row=index/2, col=index%2;
    return {180+col*390, 245+row*92, 540+col*390, 315+row*92};
}

RECT cardRect(int index) { return {110+index*305, 468, 375+index*305, 660}; }

std::wstring activityName(Activity a) {
    switch(a) {
    case Activity::Colors:return L"La corona de los colores";
    case Activity::Shapes:return L"Las formas del castillo";
    case Activity::Dress:return L"Viste a la princesa y al príncipe";
    case Activity::Sort:return L"Clasifica los objetos reales";
    case Activity::Count:return L"Cuenta las coronas";
    case Activity::Challenge:return L"Gran desafío de aprendizaje";
    }
    return L"";
}

Question makeQuestion(std::wstring prompt, std::wstring object, std::array<std::wstring,3> labels,
    std::array<COLORREF,3> colors, std::array<Shape,3> shapes, int correct, bool numbers=false) {
    return {std::move(prompt),std::move(object),std::move(labels),std::move(colors),
        std::move(shapes),correct,numbers};
}

Question colorQuestion(int n) {
    const std::array<std::wstring,4> names{L"Rojo",L"Azul",L"Amarillo",L"Verde"};
    const int target=n%4;
    const std::wstring thing=std::array<std::wstring,4>{L"la corona",L"la flor",L"el vestido",L"el globo"}[n%4];
    std::array<std::wstring,3> options{names[(target+1)%4],names[target],names[(target+2)%4]};
    std::array<COLORREF,3> colors{};
    for(int i=0;i<3;++i) colors[i]=colorFor(options[i]);
    const Shape objectShape=n%4==0?Shape::Crown:n%4==1?Shape::Flower:
        n%4==2?Shape::Dress:Shape::Ball;
    std::array<Shape,3> shapes{objectShape,objectShape,objectShape};
    return makeQuestion(L"¿Dónde está "+thing+L" "+names[target]+L"?",thing,options,colors,shapes,1);
}

Question shapeQuestion(int n) {
    const std::array<std::wstring,3> names{L"círculo",L"cuadrado",L"triángulo"};
    const int target=n%3;
    std::array<Shape,3> shapes{Shape::Circle,Shape::Square,Shape::Triangle};
    if(n%2) shapes={Shape::Triangle,Shape::Circle,Shape::Square};
    std::array<std::wstring,3> labels;
    for(int i=0;i<3;++i) labels[i]=names[static_cast<int>(shapes[i])];
    const int correct=n%2?((target+1)%3):target;
    const bool dropMode=n>=3;
    std::wstring prompt=dropMode?L"Coloca el "+names[target]+L" en su espacio.":L"¿Cuál es el "+names[target]+L"?";
    auto question=makeQuestion(prompt,L"Busca la forma",labels,
        {RGB(244,132,147),RGB(109,190,139),RGB(91,156,220)},shapes,correct);
    question.dropMode=dropMode;
    return question;
}

Question dressQuestion(int n) {
    const std::wstring garment=n%4==0?L"el vestido":n%4==1?L"la corona":
        n%4==2?L"los zapatos":L"la capa";
    const bool targetIsPrince=n%4==1 || n%4==3;
    const std::wstring color= (n%4==0)?L"Amarillo":(n%4==1)?L"Rojo":(n%4==2)?L"Azul":L"Verde";
    std::array<std::wstring,3> labels{L"Rojo",L"Amarillo",L"Azul"};
    std::array<COLORREF,3> colors{colorFor(L"Rojo"),colorFor(L"Amarillo"),colorFor(L"Azul")};
    int correct=color==L"Rojo"?0:color==L"Amarillo"?1:2;
    if(color==L"Verde") { colors[2]=colorFor(L"Verde"); labels[2]=L"Verde"; }
    const Shape garmentShape=n%4==0?Shape::Dress:n%4==1?Shape::Crown:
        n%4==2?Shape::Shoe:Shape::Cape;
    return makeQuestion(L"Busca "+garment+L" "+color+
        (targetIsPrince?L" para el príncipe.":L" para la princesa."),
        garment,labels,colors,{garmentShape,garmentShape,garmentShape},correct);
}

Question sortQuestion(int n) {
    const std::array<std::wstring,4> names{L"Rojo",L"Azul",L"Amarillo",L"Verde"};
    const int target=n%4;
    const std::wstring item=std::array<std::wstring,4>{L"la flor",L"la estrella",L"la pelota",L"el regalo"}[n%4];
    std::array<std::wstring,3> labels{names[(target+1)%4],names[target],names[(target+2)%4]};
    std::array<COLORREF,3> colors{colorFor(labels[0]),colorFor(labels[1]),colorFor(labels[2])};
    std::array<Shape,3> shapes{Shape::Gift,Shape::Gift,Shape::Gift};
    auto question=makeQuestion(L"¿En qué cesta va "+item+L"? Elige su color.",item,labels,colors,shapes,1);
    question.colors[1]=colorFor(names[target]);
    return question;
}

Question countQuestion(int n) {
    const int count=n%5+1;
    std::array<std::wstring,3> labels{std::to_wstring(count==1?2:count-1),
        std::to_wstring(count),std::to_wstring(count==5?4:count+1)};
    return makeQuestion(L"¿Cuántas coronas ves?",std::to_wstring(count),labels,
        {GOLD,GOLD,GOLD},{Shape::Crown,Shape::Crown,Shape::Crown},1,true);
}

void buildQuestions() {
    g_questions.clear();
    for(int n=0;n<6;++n) {
        if(g_activity==Activity::Challenge) {
            switch(n%3) {
            case 0:g_questions.push_back(colorQuestion(n));break;
            case 1:g_questions.push_back(shapeQuestion(n));break;
            default:g_questions.push_back(countQuestion(n));break;
            }
        } else switch(g_activity) {
        case Activity::Colors:g_questions.push_back(colorQuestion(n));break;
        case Activity::Shapes:g_questions.push_back(shapeQuestion(n));break;
        case Activity::Dress:g_questions.push_back(dressQuestion(n));break;
        case Activity::Sort:g_questions.push_back(sortQuestion(n));break;
        case Activity::Count:g_questions.push_back(countQuestion(n));break;
        default:break;
        }
    }
}

void loadQuestion() {
    if(g_questionIndex>=static_cast<int>(g_questions.size())) {
        g_screen=Screen::Results;
        g_message=g_score?L"¡Qué gran aventura!":L"¡Gracias por jugar!";
    } else {
        g_question=g_questions[g_questionIndex];
        g_attempts=0; g_lastAnswer=-1; g_answerLocked=false; g_dragging=false;
        g_message=L"Elige una respuesta. ¡Tienes tres intentos!";
    }
    InvalidateRect(g_window,nullptr,FALSE);
}

void startActivity(Activity activity) {
    g_activity=activity; g_questionIndex=0; g_correct=0; g_score=0;
    if(activity==Activity::Challenge) {
        g_screen=Screen::ChallengeIntro;
        g_message=L"Colores, formas y cantidades te esperan.";
    } else {
        buildQuestions(); g_screen=Screen::Quiz; loadQuestion();
    }
    InvalidateRect(g_window,nullptr,FALSE);
}

void celebrate() {
    g_sparkles.clear();
    for(int i=0;i<28;++i) g_sparkles.push_back({100+static_cast<int>(g_random()%900),
        140+static_cast<int>(g_random()%500),45});
    ++g_stars;
}

void answer(int selected) {
    if(g_answerLocked) {
        ++g_questionIndex;
        if(g_questionIndex>=static_cast<int>(g_questions.size())) {
            g_screen=Screen::Results;
            g_message=g_score?L"¡Qué gran aventura!":L"¡Gracias por jugar!";
            InvalidateRect(g_window,nullptr,FALSE);
        } else loadQuestion();
        return;
    }
    ++g_attempts; g_lastAnswer=selected;
    if(selected==g_question.correct) {
        g_answerLocked=true; ++g_correct; ++g_score;
        g_message=L"¡Muy bien! ¡Ganaste una corona!";
        celebrate();
    } else {
        if(g_attempts>=3) {
            g_answerLocked=true;
            g_message=L"¡Tú puedes! La respuesta era: "+g_question.labels[g_question.correct]+L".";
        } else {
            g_message=g_attempts==1?L"¡Inténtalo otra vez!":L"¡Tú puedes! Un intento más.";
        }
    }
    InvalidateRect(g_window,nullptr,FALSE);
}

bool inside(POINT p, RECT r) {
    return p.x>=r.left && p.x<r.right && p.y>=r.top && p.y<r.bottom;
}

void drawMenu(HDC dc) {
    drawCastle(dc,235);
    RECT title{130,22,990,100};
    text(dc,L"LA ACADEMIA REAL",title,39,RGB(102,72,139),true);
    RECT subtitle{180,85,940,132};
    text(dc,L"DE LOS COLORES Y LAS FORMAS",subtitle,27,RGB(197,109,149),true);
    drawCharacter(dc,93,412,true,1);
    drawCharacter(dc,1025,414,false,1);
    const std::array<std::wstring,8> labels{
        L"1. La corona de los colores",L"2. Las formas del castillo",
        L"3. Viste a la princesa y al príncipe",L"4. Clasifica los objetos reales",
        L"5. Cuenta las coronas",L"6. Gran desafío de aprendizaje",
        L"7. Instrucciones",L"8. Salir"};
    const std::array<COLORREF,8> colors{
        RGB(255,216,222),RGB(211,231,255),RGB(255,237,190),RGB(211,241,218),
        RGB(235,218,250),RGB(255,220,190),RGB(231,226,245),RGB(252,221,224)};
    for(int i=0;i<8;++i) {
        RECT r=buttonRect(i); rounded(dc,r,colors[i],RGB(255,255,255),24);
        text(dc,labels[i],{r.left+8,r.top+5,r.right-8,r.bottom-5},19,INK,true);
    }
    text(dc,L"¡Elige una aventura y comienza a jugar!",{260,685,860,737},19,PURPLE,true);
    drawCrown(dc,48,42,38);
    RECT stars{72,20,140,65}; text(dc,std::to_wstring(g_stars),stars,22,INK,true);
}

void drawInstructions(HDC dc) {
    drawCastle(dc,200);
    rounded(dc,{110,80,1010,672},RGB(255,250,245),RGB(255,255,255),32);
    text(dc,L"¡Bienvenidos al reino!",{180,105,940,160},34,RGB(127,91,164),true);
    const std::array<std::wstring,6> lines{
        L"1. Elige una aventura en el menú del castillo.",
        L"2. Mira la pregunta y toca una de las tres respuestas.",
        L"3. Cada pregunta tiene tres intentos. ¡Puedes hacerlo!",
        L"4. Cada respuesta correcta te regala una corona.",
        L"5. En «Viste», arrastra una prenda hasta el personaje",
        L"   o simplemente tócala. ¡Aprendemos jugando!"};
    for(int i=0;i<6;++i)
        text(dc,lines[i],{180,196+i*50,940,242+i*50},21,INK,false,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    rounded(dc,{410,575,710,634},RGB(255,221,233),RGB(255,255,255),22);
    text(dc,L"Volver al menú",{420,580,700,630},21,INK,true);
}

void drawQuiz(HDC dc) {
    RECT bg{0,0,WINDOW_W,WINDOW_H}; HBRUSH b=CreateSolidBrush(RGB(255,248,244));FillRect(dc,&bg,b);DeleteObject(b);
    drawCastle(dc,90);
    rounded(dc,{42,22,1078,146},RGB(255,255,255),RGB(240,222,233),28);
    text(dc,activityName(g_activity),{64,30,1056,76},27,RGB(115,82,154),true);
    text(dc,g_question.prompt,{70,79,1050,132},23,INK,true);
    text(dc,L"Pregunta "+std::to_wstring(g_questionIndex+1)+L" de 6     Intentos: "+
        std::to_wstring(g_answerLocked?g_attempts:3-g_attempts),{80,157,530,195},17,PURPLE,true,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    text(dc,L"Coronas: "+std::to_wstring(g_score),{800,157,1040,195},19,RGB(183,126,27),true);
    if(g_activity==Activity::Count) {
        const int crownCount=std::stoi(g_question.object);
        for(int i=0;i<crownCount;++i) drawCrown(dc,440+(i%5)*62,295+(i/5)*62,54);
    } else if(g_activity==Activity::Dress) {
        const bool princess=g_question.prompt.find(L"princesa")!=std::wstring::npos;
        drawCharacter(dc,546,363,princess,1);
        if(g_answerLocked && g_lastAnswer==g_question.correct) {
            const bool crown=g_question.shapes[g_question.correct]==Shape::Crown;
            const bool shoes=g_question.shapes[g_question.correct]==Shape::Shoe;
            drawShape(dc,g_question.shapes[g_question.correct],546,
                crown?248:shoes?450:378,crown?44:64,g_question.colors[g_question.correct]);
        }
        rounded(dc,{450,430,642,464},RGB(255,255,255),RGB(241,214,229),12);
        text(dc,L"¡Arrastra aquí tu elección!",{458,430,634,463},14,PURPLE,true);
    } else if(g_activity==Activity::Sort) {
        const std::array<Shape,4> objects{Shape::Flower,Shape::Star,Shape::Ball,Shape::Gift};
        drawShape(dc,objects[g_questionIndex%4],545,315,105,g_question.colors[1]);
        text(dc,g_question.object,{440,380,650,420},21,INK,true);
        rounded(dc,{410,420,680,460},RGB(255,255,255),RGB(235,214,228),14);
        text(dc,L"Elige la cesta de su color",{420,420,670,459},16,PURPLE,true);
    } else {
        drawCharacter(dc,127,340,true,1);
        drawCharacter(dc,990,345,false,1);
        drawCrown(dc,559,297,70);
    }
    if(g_activity==Activity::Shapes && g_question.dropMode) {
        rounded(dc,{440,210,660,340},RGB(255,255,255),RGB(190,170,208),22);
        drawShape(dc,g_question.shapes[g_question.correct],550,275,78,RGB(220,214,229));
    }
    for(int i=0;i<CARD_COUNT;++i) {
        RECT r=cardRect(i);
        COLORREF fill=RGB(255,255,255),edge=RGB(236,218,234);
        if(g_answerLocked && i==g_question.correct) { fill=RGB(228,250,228); edge=RGB(109,190,139); }
        else if(i==g_lastAnswer && g_lastAnswer!=g_question.correct) { fill=RGB(255,238,235); edge=RGB(235,160,151); }
        rounded(dc,r,fill,edge,26);
        if(g_question.numberOptions) {
            int n=std::stoi(g_question.labels[i]);
            for(int j=0;j<n;++j) drawCrown(dc,r.left+45+(j%4)*53,r.top+58+(j/4)*52,42);
            text(dc,g_question.labels[i],{r.left+5,r.top+125,r.right-5,r.top+180},30,INK,true);
        } else if(g_activity==Activity::Sort) {
            const int cx=(r.left+r.right)/2;
            rounded(dc,{cx-47,r.top+52,cx+47,r.top+112},g_question.colors[i],RGB(255,255,255),12);
            rounded(dc,{cx-54,r.top+47,cx+54,r.top+61},g_question.colors[i],RGB(255,255,255),8);
            text(dc,L"Cesta "+g_question.labels[i],{r.left+8,r.top+128,r.right-8,r.bottom-8},18,INK,true);
        } else {
            drawShape(dc,g_question.shapes[i],(r.left+r.right)/2,r.top+77,81,g_question.colors[i]);
            text(dc,g_question.labels[i],{r.left+8,r.top+132,r.right-8,r.top+178},20,INK,true);
            if(g_activity==Activity::Dress)
                text(dc,g_question.object,
                    {r.left+8,r.bottom-37,r.right-8,r.bottom-10},13,PURPLE,false);
        }
    }
    RECT status{150,682,970,740};
    rounded(dc,status,RGB(255,255,255),RGB(241,222,233),18);
    text(dc,g_message,{status.left+10,status.top+5,status.right-10,status.bottom-5},19,
        g_answerLocked&&g_lastAnswer==g_question.correct?RGB(68,143,83):INK,true);
    if(g_answerLocked) {
        rounded(dc,{964,685,1090,737},RGB(255,222,164),RGB(255,255,255),16);
        text(dc,L"¡Seguir!",{969,687,1085,735},17,INK,true);
    }
    if(g_dragging) {
        drawShape(dc,g_question.shapes[g_lastAnswer<0?0:g_lastAnswer],g_mouse.x,g_mouse.y,60,
            g_question.colors[g_lastAnswer<0?0:g_lastAnswer]);
    }
    for(const auto& s:g_sparkles) if(s.life>0) {
        RECT sr{s.x-7,s.y-7,s.x+7,s.y+7};
        text(dc,L"✦",sr,20,GOLD,true);
    }
}

void drawResults(HDC dc) {
    RECT bg{0,0,WINDOW_W,WINDOW_H};HBRUSH b=CreateSolidBrush(RGB(255,246,242));FillRect(dc,&bg,b);DeleteObject(b);
    drawCastle(dc,360);
    rounded(dc,{170,82,950,640},RGB(255,255,255),RGB(246,224,222),34);
    text(dc,g_message,{200,110,920,174},37,RGB(129,89,163),true);
    drawCharacter(dc,310,386,true,1);
    drawCharacter(dc,810,390,false,1);
    const std::array<COLORREF,3> crownColors{GOLD,RGB(232,177,213),RGB(176,199,236)};
    for(int i=0;i<std::min(g_score,5);++i)
        drawCrown(dc,445+(i%3)*83,260+(i/3)*70,58,crownColors[i%crownColors.size()]);
    text(dc,L"Respuestas correctas: "+std::to_wstring(g_correct)+L" de 6",
        {320,410,800,462},24,INK,true);
    text(dc,L"Coronas ganadas: "+std::to_wstring(g_score),
        {320,463,800,510},25,RGB(183,126,27),true);
    rounded(dc,{396,545,724,610},RGB(255,220,232),RGB(255,255,255),23);
    text(dc,L"Volver al menú real",{405,550,715,605},23,INK,true);
}

void drawIntro(HDC dc) {
    RECT bg{0,0,WINDOW_W,WINDOW_H};HBRUSH b=CreateSolidBrush(RGB(255,247,239));FillRect(dc,&bg,b);DeleteObject(b);
    drawCastle(dc,345);
    rounded(dc,{180,105,940,635},RGB(255,255,255),RGB(240,221,235),34);
    text(dc,L"¡Gran desafío real!",{220,140,900,205},37,RGB(125,89,161),true);
    drawCharacter(dc,370,400,true,1);drawCharacter(dc,750,405,false,1);
    text(dc,L"Colores, formas y coronas",{300,480,820,527},24,INK,true);
    text(dc,L"¡Responde y reúne tus coronas!",{300,520,820,561},20,PURPLE,false);
    rounded(dc,{395,563,725,621},RGB(255,218,170),RGB(255,255,255),22);
    text(dc,L"¡Empezar!",{405,566,715,617},24,INK,true);
}

void paint(HWND hwnd) {
    PAINTSTRUCT ps; HDC screen=BeginPaint(hwnd,&ps);
    RECT client{};GetClientRect(hwnd,&client);
    HDC dc=CreateCompatibleDC(screen);
    HBITMAP bitmap=CreateCompatibleBitmap(screen,client.right,client.bottom);
    auto old=SelectObject(dc,bitmap);
    RECT r{0,0,client.right,client.bottom};
    HBRUSH bg=CreateSolidBrush(RGB(255,248,244));FillRect(dc,&r,bg);DeleteObject(bg);
    if(g_screen==Screen::Menu) drawMenu(dc);
    else if(g_screen==Screen::Instructions) drawInstructions(dc);
    else if(g_screen==Screen::Quiz) drawQuiz(dc);
    else if(g_screen==Screen::Results) drawResults(dc);
    else drawIntro(dc);
    BitBlt(screen,0,0,client.right,client.bottom,dc,0,0,SRCCOPY);
    SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);
    EndPaint(hwnd,&ps);
}

void handleMenuClick(POINT p) {
    for(int i=0;i<8;++i) if(inside(p,buttonRect(i))) {
        switch(i) {
        case 0:startActivity(Activity::Colors);break;
        case 1:startActivity(Activity::Shapes);break;
        case 2:startActivity(Activity::Dress);break;
        case 3:startActivity(Activity::Sort);break;
        case 4:startActivity(Activity::Count);break;
        case 5:startActivity(Activity::Challenge);break;
        case 6:g_screen=Screen::Instructions;break;
        case 7:DestroyWindow(g_window);break;
        }
        return;
    }
}

LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch(msg) {
    case WM_PAINT:paint(hwnd);return 0;
    case WM_TIMER:
        for(auto& s:g_sparkles) --s.life;
        g_sparkles.erase(std::remove_if(g_sparkles.begin(),g_sparkles.end(),
            [](const Sparkle& s){return s.life<=0;}),g_sparkles.end());
        InvalidateRect(hwnd,nullptr,FALSE);return 0;
    case WM_MOUSEMOVE:
        g_mouse={GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};
        if(g_dragging) InvalidateRect(hwnd,nullptr,FALSE);
        return 0;
    case WM_LBUTTONDOWN: {
        POINT p{GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};
        if(g_screen==Screen::Menu) handleMenuClick(p);
        else if(g_screen==Screen::Instructions) {
            if(inside(p,{350,540,770,660})) {
                g_screen=Screen::Menu;
            }
        } else if(g_screen==Screen::ChallengeIntro) {
            if(inside(p,{350,530,770,655})) {
                buildQuestions();g_screen=Screen::Quiz;g_questionIndex=0;g_correct=0;g_score=0;loadQuestion();
            }
        } else if(g_screen==Screen::Results) {
            if(inside(p,{350,520,770,640})) {
                g_screen=Screen::Menu;
            }
        } else if(g_screen==Screen::Quiz) {
            if(g_answerLocked && inside(p,{940,675,1100,750})) {
                answer(0);return 0;
            }
            if(!g_answerLocked) for(int i=0;i<CARD_COUNT;++i) {
                if(inside(p,cardRect(i))) {
                    if(g_activity==Activity::Dress || (g_activity==Activity::Shapes && g_question.dropMode)) {
                        g_dragging=true;g_lastAnswer=i;SetCapture(hwnd);InvalidateRect(hwnd,nullptr,FALSE);
                    } else answer(i);
                    break;
                }
            }
        }
        return 0;
    }
    case WM_LBUTTONUP:
        if(g_dragging) {
            POINT p{GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};
            ReleaseCapture();g_dragging=false;
            RECT target=g_activity==Activity::Shapes?RECT{430,200,670,355}:RECT{430,225,665,470};
            if(inside(p,target) || (g_lastAnswer>=0 && inside(p,cardRect(g_lastAnswer))))
                answer(g_lastAnswer);
            InvalidateRect(hwnd,nullptr,FALSE);
        }
        return 0;
    case WM_DESTROY:
        KillTimer(hwnd,TIMER_ID);
        PostQuitMessage(0);return 0;
    }
    return DefWindowProcW(hwnd,msg,wp,lp);
}
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    WNDCLASSW wc{};
    wc.lpfnWndProc=windowProc;wc.hInstance=instance;
    wc.hCursor=LoadCursorW(nullptr, MAKEINTRESOURCEW(32649));
    wc.lpszClassName=L"AcademiaRealColoresFormas";
    wc.hIcon=LoadIconW(nullptr,MAKEINTRESOURCEW(32512));
    if(!RegisterClassW(&wc)) return 1;
    RECT rect{0,0,WINDOW_W,WINDOW_H};
    AdjustWindowRect(&rect,WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,FALSE);
    g_window=CreateWindowExW(0,wc.lpszClassName,L"La Academia Real de los Colores y las Formas",
        WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,
        rect.right-rect.left,rect.bottom-rect.top,nullptr,nullptr,instance,nullptr);
    if(!g_window) return 1;
    ShowWindow(g_window,show);SetTimer(g_window,TIMER_ID,45,nullptr);
    MSG message{};
    while(GetMessageW(&message,nullptr,0,0)>0) {
        TranslateMessage(&message);DispatchMessageW(&message);
    }
    return static_cast<int>(message.wParam);
}