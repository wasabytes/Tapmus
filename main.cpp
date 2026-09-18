#include <iostream>
#include <chrono>
#include <thread>
#include <termios.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fstream>
#include <sys/select.h>
using namespace std;
using namespace std::this_thread;
using namespace std::chrono;
#define f_music "test.flac"
#define chart "test.txt"
class note
{
public:
    double time;
    int status;
};
void get_keyboard(Mix_Music *music, note notes[7][128], int &score, float &num, int orbit)
{
    // its name is get_keybroad but actually it involves getting keybroad and judging scores
    for (int i = 0; i < 128; i++)
    {
        if (notes[orbit][i].status == 1)
        {
            num = (Mix_GetMusicPosition(music) - notes[orbit][i].time);
            if (num < -1)
            {
                break;
            }
            // for printing these word can cause a lot of mistakes,I give up
            else if (num > 0.5 || (-1 < num && num < -0.5))
            {
                // cout << "miss!!!\r\n";
                // cout << num;
            }
            else if ((-0.5 < num && num < -0.3) || (0.3 < num && num < 0.5))
            {
                // cout << "good!!!\r\n";
                // cout << num;
                score += 5;
            }
            else if ((-0.3 < num && num < -0.15) || (0.15 < num && num < 0.3))
            {
                // cout << "great!!\r\n";
                // cout << num;
                score += 10;
            }
            else if (-0.15 < num && num < 0.15)
            {
                // cout << "perfect\r\n";
                // cout << num;
                score += 20;
            }
            notes[orbit][i].status = 0;
            break;
        }
    }
}
termios enable_raw()
{
    struct termios raw;
    tcgetattr(STDIN_FILENO, &raw);
    termios old = raw;

    cfmakeraw(&raw);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    return old;
}
void disable_raw(termios raw)
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}
struct print_needs
{
    int len_col, len_row, mv_col;
};
print_needs before_print(int column, int row)
{
    // it will print all the things that is static
    winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    int row_target = ws.ws_row;
    int col_target = ws.ws_col;
    int len_col = 0;
    int mv_col = 0;
    int sig = 0;
    if (col_target < column + 4)
    {
        cout << "now column :" << col_target << "<" << column + 4 << ",not enough" << endl;
    }
    if (row_target < row + 4)
    {
        cout << "now row:" << row_target << "<" << row + 4 << ",not enough" << endl;
    }
    for (int j = 0; j < row_target; j++)
    {

        if (j == 0)
        {
            cout << "┌";
            for (int k = 0; k < col_target - 2; k++)
            {
                cout << "─";
            }
            cout << "┐";
        }
        else if (j == row_target - 1)
        {
            cout << "└";
            for (int k = 0; k < col_target - 2; k++)
            {
                cout << "─";
            }
            cout << "┘";
        }
        else
        {
            cout << "│";
            for (int k = 0; k < col_target - 2; k++)
            {
                cout << " ";
            }
            cout << "│";
        }
        cout << "\r\n";
    }
    // 12 =column+1 +2
    if ((col_target - 12) % column == 0)
    {
        len_col = (col_target - 12) / column;
    }
    else if (((col_target - 12) % column) % 2 == 0)
    {
        mv_col = (((col_target - 12) % column) - ((col_target - 12) % column) % 2) / 2;
        len_col = (col_target - 12 - (col_target - 12) % column) / column;
    }
    else
    {
        mv_col = (((col_target - 12) % column) - ((col_target - 12) % column) % 2) / 2;
        len_col = (col_target - 12 - (col_target - 12) % column) / column;
    }
    int m = 0;
    for (int i = 0; i < row_target - 5; i++) // i行
    {

        for (int j = 0; j < col_target - 4; j++) // j列
        {
            cout << "\033[" << i + 3 << ";" << j + 3 + mv_col << "H";
            if (j == 0)
            {
                cout << "┃";
            }
            else if (j % (len_col + 1) != 0)
            {
                cout << " ";
            }
            else
            {
                cout << "┃";
            }
            m++;
        }
    }
    int len_row = row_target - 4;
    print_needs pn;
    pn.len_col = len_col;
    pn.len_row = len_row;
    pn.mv_col = mv_col;
    return pn;
}
void print(char *screen, int mv_col, int column, int row, int len_col, int len_row, int score)
{
    // only print judgement line and notes
    cout << "\033[2;2H";
    cout << score;
    cout << "\033[" << len_row + 2 << ";" << 3 + mv_col << "H";
    cout << "●";
    for (int i = 0; i < len_col * column + 6; i++)
    {
        cout << "━";
    }
    cout << "●";
    for (int i = 0; i < row; i++)
    {
        int m = 1;
        for (int j = 0; j < column; j++)
        {
            // int j=1;j<len_col*column+8;j=j+len_col+1z
            // screen[j][i]i->row
            cout << "\033[" << i + 3 << ";" << m + 3 + mv_col << "H";
            if (*(screen + j * row + i) == 'X')
            {
                for (int k = 0; k < len_col; k++)
                {
                    cout << "█";
                }
            }
            else
            {
                for (int k = 0; k < len_col; k++)
                {
                    cout << " ";
                }
            }
            m = m + len_col + 1;
        }
    }
}
int main()
{
    // init
    winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    int row_target = ws.ws_row;
    int col_target = ws.ws_col;
    const int row = row_target - 4;
    const int column = 7;
    const int max_notes = 128;
    int speed = 40; // how many blocks it will drop in a second
    char screen[column][row];
    for (int i = 0; i < column; i++)
    {
        for (int j = 0; j < row; j++)
        {
            screen[i][j] = ' ';
        }
    }
    cout << "\033[2J\033[?25l";

    int lastnote[column];
    for (int i = 0; i < column; i++)
    {
        lastnote[i] = 0;
    }
    termios raw = enable_raw();
    char buf[1];
    float num = 0;
    int score = 0;
    note notes[column][max_notes];
    for (int i = 0; i < column; i++)
    {
        for (int j = 0; j < max_notes; j++)
        {
            // notes[i][j].x = 0;
            notes[i][j].time = 0;
            notes[i][j].status = 0;
        }
    }
    ifstream f(chart);
    string line;
    int l = 1;
    int ln[column];
    for (int i = 0; i < column; i++)
    {
        ln[i] = 0;
    }
    while (getline(f, line))
    {
        for (int i = 0; i < column&&i<line.length(); i++)
        {
            if (line[i] == '1')
            {
                notes[i][ln[i]].time = l * 0.01;

                notes[i][ln[i]].status = 1;
                ln[i]++;
            }
        }
        l++;
    }
    f.close();
    print_needs pn = before_print(column, row);
    int len_col = pn.len_col;
    int len_row = pn.len_row;
    int mv_col = pn.mv_col;
    SDL_Init(SDL_INIT_AUDIO);
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 512);
    Mix_Init(MIX_INIT_FLAC);
    auto music = Mix_LoadMUS(f_music);
    Mix_PlayMusic(music, 0);

    while (1)
    {
        // 123 4 567->sdf  gh  jkl
        auto now_t = chrono::steady_clock::now();
        int n = read(STDIN_FILENO, buf, 1);
        while (n != 0)
        {
            if (n > 0)
            {
                switch (buf[0])
                {
                case 's':
                {
                    get_keyboard(music, notes, score, num, 0);
                    break;
                }
                case 'd':
                {
                    get_keyboard(music, notes, score, num, 1);
                    break;
                }
                case 'f':
                {
                    get_keyboard(music, notes, score, num, 2);
                    break;
                }
                case 'g':
                {
                    get_keyboard(music, notes, score, num, 3);
                    break;
                }
                case 'h':
                {
                    get_keyboard(music, notes, score, num, 3);
                    break;
                }
                case 'j':
                {
                    get_keyboard(music, notes, score, num, 4);
                    break;
                }
                case 'k':
                {
                    get_keyboard(music, notes, score, num, 5);
                    break;
                }
                case 'l':
                {
                    get_keyboard(music, notes, score, num, 6);
                    break;
                }
                case 'q':
                    disable_raw(raw);
                    Mix_FreeMusic(music);
                    Mix_CloseAudio();
                    Mix_Quit();
                    return 0;
                }
            }
            n = read(STDIN_FILENO, buf, 1);
        }

        for (int i = 0; i < column; i++)
        {
            for (int j = 0; j < row; j++)
            {
                screen[i][j] = ' ';
            }
        }
        double time = Mix_GetMusicPosition(music);
        for (int i = 0; i < column; i++)
        {
            for (int k = lastnote[i]; k < max_notes; k++)
            {
                if (notes[i][k].status == 0)
                {
                }
                else
                {
                    int y = int((time - notes[i][k].time) * speed) + row - 1;
                    if (y >= 0 && y < row)
                    {
                        screen[i][y] = 'X';
                    }
                    if (y >= row + 1)
                    {
                        if (notes[i][k].status != 0)
                        {
                            // cout << "miss\r\n";
                        }
                        notes[i][k].status = 0;

                        lastnote[i] = k + 1;
                    }
                }
            }
        }
        print(&screen[0][0], mv_col, column, row, len_col, len_row, score);

        // it controls fps/sample rate
        auto frametime = chrono::duration<float>(chrono::steady_clock::now() - now_t).count();
        if (frametime < 0.01f)
        {
            sleep_for(chrono::milliseconds(int((0.01f - frametime) * 1000)));
        }
    }
    disable_raw(raw);
    Mix_FreeMusic(music);
    Mix_CloseAudio();
    Mix_Quit();
    return 0;
}
