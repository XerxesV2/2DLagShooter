#pragma once
#include <string>
extern struct Vector2f;

struct Rect
{
    float left;   
    float top;  
    float width;
    float height;
};

namespace Utils
{
    float d2r(float d);
    float r2d(float r);
    bool CheckCircleCollide(double x1, double y1, float r1, double x2, double y2, float r2);
    bool linePoint(float x1, float y1, float x2, float y2, float px, float py);
    bool pointCircle(float px, float py, float cx, float cy, float r);
    bool LineCircleCollide(float x1, float y1, float x2, float y2, float cx, float cy, float r, Vector2f& intersect);
    bool LineRect(float x1, float y1, float x2, float y2, float rx, float ry, float rw, float rh, Vector2f& intersection);
    bool lineLine(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, Vector2f& intersection);
    std::string TimeToString(long long currentTime);
    std::string CurrentTimeToString();
    std::string CurrentTimeToShortString();
}