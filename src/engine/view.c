#include "renderer.h"
#include "xy.h"

#define my_min(a,b) ((a) < (b) ? (a) : (b))
#define my_max(a,b) ((a) > (b) ? (a) : (b))

#define WALL_COL 0XFFFFFF

#define L_FRAME 18
#define R_FRAME 564

// depth 0
#define X_0_BL 60
#define X_0_TL 40
#define X_0_BR 522
#define X_0_TR 542
#define Y_0_T 18
#define Y_0_B 364

// depth 1
#define X_1_L 104
#define X_1_R 478
#define Y_1_T 40
#define Y_1_B 320

// depth 2
#define X_2_L 180
#define X_2_R 402
#define Y_2_T 64
#define Y_2_B 244

// depth 3
#define X_3_L 220
#define X_3_R 362
#define Y_3_T 78
#define Y_3_B 204


// up, right, down, left
const struct xy increment[4] = { { 0,-1 }, { 1, 0 }, { 0, 1 }, { -1, 0 } };

// left is direction+3 mod 4, right direction +1 mod 4:
const struct xy left[4] = {  {-1, 0 }, { 0, -1 }, { 1, 0 }, { 0, 1 } };
const struct xy right[4] = { { 1, 0 }, { 0, 1 }, { -1, 0 }, { 0, -1 } };

const int X_LEFT[3] = { X_1_L, X_2_L, X_3_L };
const int X_RIGHT[3] = { X_1_R, X_2_R, X_3_R };
const int Y_TOP[3] = { Y_1_T, Y_2_T, Y_3_T };
const int Y_BOTTOM[3] = { Y_1_B, Y_2_B, Y_3_B };

const int WIDTH[3] = { 374, 222, 142 };
const int HEIGHT[3] = { 280, 180, 126 };

// for masking occluded areas
const int OCCLUSION[2] = { 75, 39 };


static void DrawDecoration(int dist, int offset, char decoration, int direction)
{
    dist--; // for zero indexed data access
    direction++; // for non-zero cases

    switch (decoration)
    {
        case 1:
        case 2:
        case 3:
        case 4:
        {
            if (decoration == direction) // front facing door
            {
                if (dist == -1) { return; }
                if (dist == 0  && offset != 0) { return; }
                if (dist == 1 && (offset == 2 || offset == -2)) { return; }

                int width = WIDTH[dist];
                int height = HEIGHT[dist];

                int xInset = width / 4;
                int yInset = height / (7 - dist);

                int left = my_max(X_LEFT[dist] + (width * offset) + xInset, L_FRAME);
                int right = my_min(X_RIGHT[dist] + (width * offset) - xInset, R_FRAME);
                int top = Y_TOP[dist] + yInset;
                int bottom = Y_BOTTOM[dist];

                DrawThickLine(left, top, left, bottom, WALL_COL, 2);
                DrawThickLine(right, top, right, bottom, WALL_COL, 2);
                DrawThickLine(left, top, right, top, WALL_COL, 2);
            }

            else if (decoration != (direction + 2) % 4) // adjacent door
            {   
                if (offset < 0) // left side
                {
                    switch (dist)
                    {
                        case -1:
                        {
                            DrawThickLine(L_FRAME, Y_0_T + 37, X_LEFT[dist + 1] - 50, Y_0_T + 45, WALL_COL, 2);
                            DrawThickLine(X_LEFT[dist + 1] - 50, Y_0_T + 45, X_LEFT[dist + 1] - 50, Y_0_B, WALL_COL, 2);
                        } break;

                        case 0:
                        {
                            DrawThickLine(X_LEFT[dist] + 22, Y_TOP[dist] + 35, X_LEFT[dist + 1] - 20, Y_TOP[dist] + 45, WALL_COL, 2);
                            DrawThickLine(X_LEFT[dist] + 22, Y_TOP[dist] + 35, X_LEFT[dist] + 22, 298, WALL_COL, 2);
                            DrawThickLine(X_LEFT[dist + 1] - 20, Y_TOP[dist] + 45, X_LEFT[dist + 1] - 20, 264, WALL_COL, 2);
                        } break;

                        case 1:
                        {
                            if (offset == -2)
                            {
                                DrawThickLine(L_FRAME, Y_3_T + 14, X_3_L - 170, Y_3_T + 15, WALL_COL, 2);
                                DrawThickLine(X_3_L - 170, Y_3_T + 15, X_3_L - 170, 217, WALL_COL, 2);
                                break;
                            }

                            DrawThickLine(X_LEFT[dist] + 15, Y_TOP[dist] + 25, X_LEFT[dist + 1] - 10, Y_TOP[dist] + 30, WALL_COL, 2);
                            DrawThickLine(X_LEFT[dist] + 15, Y_TOP[dist] + 25, X_LEFT[dist] + 15, 229, WALL_COL, 2);
                            DrawThickLine(X_LEFT[dist + 1] - 10, Y_TOP[dist] + 30, X_LEFT[dist + 1] - 10, 213, WALL_COL, 2);
                        } break;
                    }
                }
                else if (offset > 0) // right side
                {
                    switch (dist)
                    {
                        case -1:
                        {
                            DrawThickLine(R_FRAME, Y_0_T + 37, X_RIGHT[dist + 1] + 50, Y_0_T + 45, WALL_COL, 2);
                            DrawThickLine(X_RIGHT[dist + 1] + 50, Y_0_T + 45, X_RIGHT[dist + 1] + 50, Y_0_B, WALL_COL, 2);
                        } break;

                        case 0:
                        {
                            DrawThickLine(X_RIGHT[dist] - 22, Y_TOP[dist] + 35, X_RIGHT[dist + 1] + 20, Y_TOP[dist] + 45, WALL_COL, 2);
                            DrawThickLine(X_RIGHT[dist] - 22, Y_TOP[dist] + 35, X_RIGHT[dist] - 22, 298, WALL_COL, 2);
                            DrawThickLine(X_RIGHT[dist + 1] + 20, Y_TOP[dist] + 45, X_RIGHT[dist + 1] + 20, 264, WALL_COL, 2);
                        } break;

                        case 1:
                        {
                            if (offset == 2)
                            {
                                DrawThickLine(R_FRAME, Y_3_T + 14, X_3_R + 170, Y_3_T + 15, WALL_COL, 2);
                                DrawThickLine(X_3_R + 170, Y_3_T + 15, X_3_R + 170, 217, WALL_COL, 2);
                                break;
                            }

                            DrawThickLine(X_RIGHT[dist] - 15, Y_TOP[dist] + 25, X_RIGHT[dist + 1] + 10, Y_TOP[dist] + 30, WALL_COL, 2);
                            DrawThickLine(X_RIGHT[dist] - 15, Y_TOP[dist] + 25, X_RIGHT[dist] - 15, 229, WALL_COL, 2);
                            DrawThickLine(X_RIGHT[dist + 1] + 10, Y_TOP[dist] + 30, X_RIGHT[dist + 1] + 10, 213, WALL_COL, 2);
                        } break;
                    }
                }
            }
        } break;
    }
}


static void DrawWall(int dist, int offset)
{
    if (dist == 0)
    {
        switch (offset)
        {
            case -1: // adjacent left wall
            {
                DrawGreyscaleRectangle(L_FRAME, 39, 86, 283, 0);

                DrawThickLine(X_1_L, Y_1_T, X_1_L, Y_1_B, WALL_COL, 2);
                DrawThickLine(X_0_TL, Y_0_T, X_1_L, Y_1_T, WALL_COL, 2);
                DrawThickLine(X_0_BL, Y_0_B, X_1_L, Y_1_B, WALL_COL, 2);
            } break;

            case 1: // adjacent right wall
            {
                DrawGreyscaleRectangle(X_1_R, 39, 86, 283, 0);

                DrawThickLine(X_1_R, Y_1_T, X_1_R, Y_1_B, WALL_COL, 2);
                DrawThickLine(X_0_TR, Y_0_T, X_1_R, Y_1_T, WALL_COL, 2);
                DrawThickLine(X_0_BR, Y_0_B, X_1_R, Y_1_B, WALL_COL, 2);
            } break;
        }
        return;
    }
    else if (dist == 2)
    {
        switch (offset)
        {
            case -2:
            {
                DrawGreyscaleRectangle(L_FRAME, 77, 59, 128, 0);
                DrawThickLine(L_FRAME, Y_3_B + 30, X_3_L - 142, Y_3_B, WALL_COL, 2); // BL
                DrawThickLine(L_FRAME, Y_3_T - 4, X_3_L - 142, Y_3_T, WALL_COL, 2); // TL
                return;
            }

            case 2:
            {
                DrawGreyscaleRectangle(R_FRAME - 59, 77, 59, 128, 0);
                DrawThickLine(R_FRAME, Y_3_B + 30, X_3_R + 142, Y_3_B, WALL_COL, 2);
                DrawThickLine(R_FRAME, Y_3_T - 4, X_3_R + 143, Y_3_T, WALL_COL, 2);
                return;
            }
        }
    }

    dist--;

    int width = WIDTH[dist];
    int height = HEIGHT[dist];

    int left = my_max(X_LEFT[dist] + (width * offset), L_FRAME);
    int right = my_min(X_RIGHT[dist] + (width * offset), R_FRAME);
    int top = Y_TOP[dist];
    int bottom = Y_BOTTOM[dist];

    if (offset == -1 && dist < 2)
    {
        int rightPlus = X_RIGHT[dist + 1] + (WIDTH[dist + 1] * offset);
        int topPlus = Y_TOP[dist + 1];
        int bottomPlus = Y_BOTTOM[dist + 1];

        DrawGreyscaleRectangle(left, top, (right - left) + OCCLUSION[dist], height, 0);
        DrawThickLine(rightPlus, topPlus, rightPlus, bottomPlus, WALL_COL, 2);
        DrawThickLine(right, top, rightPlus, topPlus, WALL_COL, 2);
        DrawThickLine(right, bottom, rightPlus, bottomPlus, WALL_COL, 2);
    }

    else if (offset == 1 && dist < 2)
    {
        int leftPlus = X_LEFT[dist + 1] + (WIDTH[dist + 1] * offset);
        int topPlus = Y_TOP[dist + 1];
        int bottomPlus = Y_BOTTOM[dist + 1];

        DrawGreyscaleRectangle(left - OCCLUSION[dist], top, (right - left) + OCCLUSION[dist], height, 0);
        DrawThickLine(leftPlus, topPlus, leftPlus, bottomPlus, WALL_COL, 2);
        DrawThickLine(left, top, leftPlus, topPlus, WALL_COL, 2);
        DrawThickLine(left, bottom, leftPlus, bottomPlus, WALL_COL, 2);
    }

    else
    {
        DrawGreyscaleRectangle(left, top, (right - left), height, 0);
    }

    DrawThickLine(left, top, right, top, WALL_COL, 2); // top
    DrawThickLine(left, bottom, right, bottom, WALL_COL, 2); // bottom
    DrawThickLine(left, top, left, bottom, WALL_COL, 2); // left
    DrawThickLine(right, top, right, bottom, WALL_COL, 2); // right
}


void DrawView(char (*map)[16], char(*decor)[16], struct xy pos, int direction)
{
    DrawGreyscaleRectangle(18, 18, 546, 346, 0); // clear frame

    struct xy forward = increment[direction];
    struct xy side = increment[(direction + 3) % 4];

    int frontWall = 0;
    char frontWallDecor = 0;

    for (int depth = 3; depth >= 0; depth--)
    {
        int range = (depth >= 2) ? 2 : 1; // Define the lateral range for this depth.
        int numOffsets = 2 * range + 1;
        int offsets[5] = { 0 };
        int idx = 0;
        
        // Generate painters algorithm ordering: farthest offsets first, then inward.
        for (int i = range; i >= 1; i--)
        {
            offsets[idx++] = -i; // left-most offset first
            offsets[idx++] = i;  // right-most offset second
        }
        offsets[idx++] = 0; // center last

        // Process each offset in this painter algo order.
        for (int j = 0; j < numOffsets; j++)
        {
            int offset = offsets[j];

            struct xy block = { 0 };
            block.x = pos.x + depth * forward.x + offset * (-side.x);
            block.y = pos.y + depth * forward.y + offset * (-side.y);

            if (block.x < 0 || block.x >= 16 || block.y < 0 || block.y >= 16)
                continue;

            if (map[block.y][block.x])
            {
                if (offset == 0)
                {
                    frontWall = depth; // Draw nearest centre tile last
                    frontWallDecor = decor[block.y][block.x] != 0 ? decor[block.y][block.x] : 0;
                    continue;
                }
                else
                {
                    DrawWall(depth, offset);
                    
                    if (decor[block.y][block.x])
                    {
                        DrawDecoration(depth, offset, decor[block.y][block.x], direction);
                    }
                }
            }
        }
    }
    if (frontWall != 0)
    {
        DrawWall(frontWall, 0);
        if (frontWallDecor) { DrawDecoration(frontWall, 0, frontWallDecor, direction); }
    }
}
