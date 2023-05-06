#include "utils.hpp"
#include "PacketStruct.hpp"
#define _USE_MATH_DEFINES
#include <math.h>
#include <cmath>
#include <stdio.h>
namespace Utils
{
    float d2r(float d) { return (d / 180.0f) * ((float)M_PI); }
    float r2d(float r) { return r * (180.0f / (float)M_PI); }

    bool CheckCircleCollide(double x1, double y1, float r1, double x2, double y2, float r2) {
        return std::abs((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2)) < (r1 + r2) * (r1 + r2);
    }

    bool linePoint(float x1, float y1, float x2, float y2, float px, float py) {

        // get distance from the point to the two ends of the line
        float d1 = sqrt(pow(px - x1, 2) + pow(py - y1, 2));
        float d2 = sqrt(pow(px - x2, 2) + pow(py - y2, 2));

        // get the length of the line
        float lineLen = sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2));

        // since floats are so minutely accurate, add
        // a little buffer zone that will give collision
        float buffer = 0.1;    // higher # = less accurate

        // if the two distances are equal to the line's
        // length, the point is on the line!
        // note we use the buffer here to give a range,
        // rather than one #
        if (d1 + d2 >= lineLen - buffer && d1 + d2 <= lineLen + buffer) {
            return true;
        }
        return false;
    }

    bool pointCircle(float px, float py, float cx, float cy, float r) {

        // get distance between the point and circle's center
        // using the Pythagorean Theorem
        float distX = px - cx;
        float distY = py - cy;
        float distance = std::sqrt((distX * distX) + (distY * distY));

        // if the distance is less than the circle's
        // radius the point is inside!
        if (distance <= r) {
            return true;
        }
        return false;
    }

    bool LineCircleCollide(float x1, float y1, float x2, float y2, float cx, float cy, float r, Vector2f& intersect) {

        // is either end INSIDE the circle?
        // if so, return true immediately
        bool inside1 = pointCircle(x1, y1, cx, cy, r);
        bool inside2 = pointCircle(x2, y2, cx, cy, r);
        if (inside1 || inside2) return true;

        // get length of the line
        float distX = x1 - x2;
        float distY = y1 - y2;
        float len = std::sqrt((distX * distX) + (distY * distY));

        // get dot product of the line and circle
        float dot = (((cx - x1) * (x2 - x1)) + ((cy - y1) * (y2 - y1))) / pow(len, 2);

        // find the closest point on the line
        float closestX = x1 + (dot * (x2 - x1));
        float closestY = y1 + (dot * (y2 - y1));

        // is this point actually on the line segment?
        // if so keep going, but if not, return false
        bool onSegment = linePoint(x1, y1, x2, y2, closestX, closestY);
        if (!onSegment) return false;

        // optionally, draw a circle at the closest
        // point on the line
        intersect.x = closestX;
        intersect.y = closestY;

        // get distance to closest point
        distX = closestX - cx;
        distY = closestY - cy;
        float distance = sqrt((distX * distX) + (distY * distY));

        if (distance <= r) {
            return true;
        }
        return false;
    }

    // LINE/RECTANGLE
    bool LineRect(float x1, float y1, float x2, float y2, float rx, float ry, float rw, float rh, Vector2f& intersection) {

        // check if the line has hit any of the rectangle's sides
        // uses the Line/Line function below
        if (lineLine(x1, y1, x2, y2, rx, ry, rx, ry + rh, intersection)
            || lineLine(x1, y1, x2, y2, rx + rw, ry, rx + rw, ry + rh, intersection)
            || lineLine(x1, y1, x2, y2, rx, ry, rx + rw, ry, intersection)
            || lineLine(x1, y1, x2, y2, rx, ry + rh, rx + rw, ry + rh, intersection))
            return true;
        return false;
    }


    // LINE/LINE
    bool lineLine(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, Vector2f& intersection) {

        // calculate the direction of the lines
        float uA = ((x4 - x3) * (y1 - y3) - (y4 - y3) * (x1 - x3)) / ((y4 - y3) * (x2 - x1) - (x4 - x3) * (y2 - y1));
        float uB = ((x2 - x1) * (y1 - y3) - (y2 - y1) * (x1 - x3)) / ((y4 - y3) * (x2 - x1) - (x4 - x3) * (y2 - y1));
        //printf("collide ran...\n");
        // if uA and uB are between 0-1, lines are colliding
        if (uA >= 0 && uA <= 1 && uB >= 0 && uB <= 1) {

            // optionally, draw a circle where the lines meet
            intersection.x = x1 + (uA * (x2 - x1));
            intersection.y = y1 + (uA * (y2 - y1));
            return true;
        }
        return false;
    }
}