#include <iostream>
#include <vector>
#include <utility>
#include <algorithm>
#include <chrono>
using namespace std;

#include <stdio.h>
#include <Windows.h>


int nScreenWidth = 120;
int nScreenHeight = 40;

float fPlayerX = 8.0f;
float fPlayerY = 8.0f;
float fPlayerA = 0.0f;

int nMapWidth = 16;
int nMapHeight = 16;

float fFOV = 3.14159 / 4.0;
float fDepth = 16.0f;

int main()
{

    wchar_t* screen = new wchar_t[nScreenWidth * nScreenHeight];
    HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
    SetConsoleActiveScreenBuffer(hConsole);
    DWORD dwBytesWritten = 0;

    wstring map;
    
    map += L"################";
    map += L"#..............#";
    map += L"#..............#";
    map += L"#..............#";
    map += L"#..............#";
    map += L"#..............#";
    map += L"#..............#";
    map += L"#..............#";
    map += L"###.....####...#";
    map += L"#.......#......#";
    map += L"#.......#......#";
    map += L"#.......#......#";
    map += L"#.......########";
    map += L"#..............#";
    map += L"#..............#";
    map += L"################";

    auto tp1 = chrono::system_clock::now();
    auto tp2 = chrono::system_clock::now();


    while(1)
    {
        // Controle de quadros
        tp2 = chrono::system_clock::now();
        chrono::duration<float> elapsedTime = tp2 - tp1;
        tp1 = tp2;
        float fElapsedTime = elapsedTime.count();
        // Controles para lidar com rotação

        if (GetAsyncKeyState((unsigned short)'A') & 0x80000)
        {
            fPlayerA += (1.6f) * fElapsedTime;
        }
        if (GetAsyncKeyState((unsigned short)'D') & 0x80000)
        {
            fPlayerA -= (1.6f) * fElapsedTime;
        }
        if (GetAsyncKeyState((unsigned short)'W') & 0x80000)
        {
            if(map[(int)fPlayerY * nMapWidth + (int)fPlayerX != '#'])
            { 
                fPlayerX += sinf(fPlayerA) * 5.0f * fElapsedTime;
                fPlayerY += cosf(fPlayerA) * 5.0f * fElapsedTime;
            }

        }
        if (GetAsyncKeyState((unsigned short)'S') & 0x80000)
        {
            if (map[(int)fPlayerY * nMapWidth + (int)fPlayerX != '#'])
            {
                fPlayerX -= sinf(fPlayerA) * 5.0f * fElapsedTime;
                fPlayerY -= cosf(fPlayerA) * 5.0f * fElapsedTime;
            }
        }
        for (int x = 0; x < nScreenWidth; x++)
        {
            // Para cada coluna é calculado o angulo do raio projetado no espaço do mundo.
            float fRayAngle = (fPlayerA - fFOV / 2.0f) * ((float)x / (float)nScreenWidth) * fFOV;

            float fDistanceToWall = 0;
            bool bHitWall = false;
            bool bBoundary = false;

            float fEyeX = sinf(fRayAngle);
            float fEyeY = cosf(fRayAngle);

            while (!bHitWall && fDistanceToWall < fDepth)
            {
                fDistanceToWall += 0.1f;

                int nTestX = (int)(fPlayerX + fEyeX * fDistanceToWall);
                int nTestY = (int)(fPlayerY + fEyeY * fDistanceToWall);

                //Colisão de raios
                if (nTestX < 0 || nTestX >= nMapWidth || nTestY < 0 || nTestY >= nMapHeight)
                {
                    bHitWall = true;
                    fDistanceToWall = fDepth;
                }
                else
                {
                    if (map[nTestY * nMapWidth + nTestX] == '#')
                    {
                        bHitWall = true;

                        vector<pair<float, float>> p; //Faz a angulação entre as paredes com base em seus cantos, ao menos foi isso que entendi :)
                        for (int tx = 0; tx < 2; tx++)
                        {
                            for (int ty = 0; ty < 2; ty++)
                            {
                                float vy = (float)nTestY + ty - fPlayerY;
                                float vx = (float)nTestX + tx - fPlayerX;
                                float d = sqrt(vx * vx + vy * vy);
                                float dot = (fEyeX * vx / d) + (fEyeY * vy / d);
                                p.push_back(make_pair(d, dot));
                            }
                        }
                        sort(p.begin(), p.end(), [](const pair<float, float>& left, const pair<float, float>& right) {return left.first < right.first; });
                        
                        float fBound = 0.01;
                        if (acos(p.at(0).second) < fBound) bBoundary = true;
                        if (acos(p.at(1).second) < fBound) bBoundary = true;
                        if (acos(p.at(2).second) < fBound) bBoundary = true;

                    }
                }
            }

            // Distancia entre o teto e o chão

            int nCeiling = (float)(nScreenHeight / 2.0) - nScreenHeight / ((float)fDistanceToWall);
            int nFloor = nScreenHeight - nCeiling;

            short nShade = ' ';
            short nShadeFloor = ' ';

            if (fDistanceToWall <= fDepth / 4.0f)       nShade = 0x2588;
            else if (fDistanceToWall <= fDepth / 3.0f)  nShade = 0x2593;
            else if (fDistanceToWall <= fDepth / 2.0f)  nShade = 0x2592;
            else if (fDistanceToWall <= fDepth)         nShade = 0x2591;
            else                                        nShade = ' ';

            if (bBoundary)                              nShade = ' ';

            for (int y = 0; y < nScreenHeight; y++)
            {
                if (y < nCeiling)
                {
                    screen[y * nScreenWidth + x] = ' ';
                }
                else if(y > nCeiling && y <= nFloor)
                {
                    screen[y * nScreenWidth + x] = nShade;
                }
                else
                {
                    // Shades do chão pela distancia
                    float b = 1.0f - (((float)y -nScreenHeight / 2.0f) / ((float)nScreenHeight / 2.0f));
                    if (b < 0.25)       nShadeFloor = '#';
                    else if (b < 0.5)   nShadeFloor = 'x';
                    else if (b < 0.75)  nShadeFloor = '-';
                    else if (b < 0.9)   nShadeFloor = '-';
                    else                nShadeFloor = ' ';
                    screen[y * nScreenWidth + x] = nShadeFloor;
                }
            }
        }

        //HUD
        //Stats
        swprintf_s(screen,40, L"X=%3.2f, Y=%3.2f, A=%3.2f FPS=%3.2f ",fPlayerX, fPlayerY, fPlayerA, 1.0f / fElapsedTime);
        //Map
        for (int nx = 0; nx < nMapWidth; nx++)
        {
            for (int ny = 0; ny < nMapWidth; ny++)
            {
                screen[(ny + 1) * nScreenWidth + nx] = map[ny * nMapWidth + nx];
            }
        }
        screen[((int)fPlayerY + 1) * nScreenWidth + (int)fPlayerX] = 'P';

        screen[nScreenWidth * nScreenHeight - 1] = '\0';
        WriteConsoleOutputCharacter(hConsole, screen, nScreenWidth * nScreenHeight, { 0,0 }, &dwBytesWritten);
    }

    return 0;

    
}