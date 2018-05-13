//
//  Canvas.cpp
//  Z-Renderer
//
//  Created by SATAN_Z on 2018/4/20.
//  Copyright © 2018年 SATAN_Z. All rights reserved.
//

#include "Canvas.hpp"
#include <algorithm>
#include <vector>
#include "Box.hpp"
#include <iostream>

using namespace std;

const int SCREEN_WIDTH     = 800;
const int SCREEN_HEIGHT    = 600;

Canvas * Canvas::s_pCanvas = nullptr;

Canvas::Canvas(int width , int height):
_surface(nullptr),
_width(width),
_height(height),
_drawMode(DrawMode::Fill),
_bufferSize(height * width),
_PC(true),
_shader(nullptr) {
    _depthBuffer = new Ldouble[_bufferSize]();
    _shader = new Shader();
    
    _node.push_back(Box::create());
}

Canvas * Canvas::getInstance() {
    if (s_pCanvas == nullptr) {
        s_pCanvas = new Canvas(SCREEN_WIDTH , SCREEN_HEIGHT);
    }
    return s_pCanvas;
}

void Canvas::clear() {
    memset(getPixels(), 0, sizeof(uint32_t) * _width * _height);
    std::fill(_depthBuffer, _depthBuffer + _bufferSize, MAXFLOAT);
}

void Canvas::update() {
    
    lock();
    
    clear();
    
    render();
    
    unlock();
    
}

void Canvas::render() {
    
    for (int i = 0 ; i < _node.size() ; ++ i) {
        auto node = _node.at(i);
        node->draw(0);
    }
    
//    Ldouble z = -1;
//    auto d =2;
//    Vec3 p1 = Vec3(-1 ,0 ,z);
//    Vec3 p2 = Vec3(1 , 2 ,z - d);
//    Vec3 p3 = Vec3(1 , 0, z);
//
//    Vec3 p4 = Vec3(-1 , 2 , z- d);
//
//    auto camera = Camera::getInstance();
//
//    auto p = camera->getProjectionMat();
//
//    _shader->setProjectionMat(p);
//    _shader->setViewMat(camera->getViewMat());
//
//    Vertex v1(p1 , Color(1 , 0 , 0 , 0));
//    Vertex v2(p2 , Color(0 , 1 , 0 , 0));
//    Vertex v3(p3 , Color(0 , 1 , 0 , 0));
//    Vertex v4(p4 , Color(1 , 0 , 0 , 0));
//
//
//    drawTriangle(v1 , v2 , v3);
//    drawTriangle(v1 , v2 , v4);
}

void Canvas::lock() {
    SDL_LockSurface(_surface);
}

void Canvas::unlock() {
    SDL_UnlockSurface(_surface);
}

void Canvas::drawTriangle(const Vertex &v1, const Vertex &v2, const Vertex &v3) {
    VertexOut vOut1 = handleVertex(v1);
    VertexOut vOut2 = handleVertex(v2);
    VertexOut vOut3 = handleVertex(v3);
    
    if (_drawMode == Fill) {
        _triangleRasterize(vOut1, vOut2, vOut3);
    } else if (_drawMode == Frame) {
        lineBresenham(vOut1, vOut2);
        lineBresenham(vOut1, vOut3);
        lineBresenham(vOut3, vOut2);
    }
}

void Canvas::_triangleRasterize(const VertexOut &v1, const VertexOut &v2, const VertexOut &v3) {
    VertexOut const * pVert1 = &v1;
    VertexOut const * pVert2 = &v2;
    VertexOut const * pVert3 = &v3;
    
    vector<VertexOut const *> vector = {pVert1 , pVert2 , pVert3};
    
    sort(vector.begin(), vector.end() , [](const VertexOut * p1 , const VertexOut * p2)->bool {
        return p1->pos.y >= p2->pos.y;
    });
    
    pVert1 = vector.at(0);
    pVert2 = vector.at(1);
    pVert3 = vector.at(2);
    
    if (MathUtil::equal(pVert1->pos.y , pVert2->pos.y)) {
        // 平顶三角形
        _triangleBottomRasterize(*pVert1, *pVert2, *pVert3);
    } else if (MathUtil::equal(pVert2->pos.y , pVert3->pos.y)) {
        // 平底三角形
        _triangleTopRasterize(*pVert1, *pVert2, *pVert3);
    } else {
        Ldouble ty = pVert2->pos.y;
        Ldouble factor = (ty - pVert1->pos.y) / (pVert3->pos.y - pVert1->pos.y);
        VertexOut tVert = pVert1->interpolate(*pVert3 , factor);
        _triangleTopRasterize(*pVert1, tVert , *pVert2);
        _triangleBottomRasterize(*pVert2, tVert , *pVert3);
    }
}

void Canvas::_triangleTopRasterize(const VertexOut &v1, const VertexOut &v2, const VertexOut &v3) {
    const VertexOut * pVert1 = &v1;
    const VertexOut * pVert2 = &v2;
    const VertexOut * pVert3 = &v3;
    vector<const VertexOut *> vector = {pVert1 , pVert2 , pVert3};
    // 根据纵坐标排序
    sort(vector.begin(), vector.end() , [](const VertexOut * p1 , const VertexOut * p2)->bool {
        return p1->pos.y >= p2->pos.y;
    });
    // 上面的顶点
    pVert1 = vector.at(0);
    pVert2 = vector.at(1);
    pVert3 = vector.at(2);
    
    int startPY = pVert1->pos.y;
    int endPY = pVert3->pos.y;
    
    int sign = endPY > startPY ? 1 : -1;
    
    for (int py = startPY ; py * sign <= sign * endPY ; py = py + sign) {
        Ldouble ld = 1.0f;
        Ldouble factor = (py - startPY) * ld / (endPY - startPY);
        VertexOut vertStart = pVert1->interpolate(*pVert2, factor);
        VertexOut vertEnd = pVert1->interpolate(*pVert3, factor);
        scanLineFill(vertStart , vertEnd , py);
    }
}

void Canvas::_triangleBottomRasterize(const VertexOut &v1, const VertexOut &v2, const VertexOut &v3) {
    const VertexOut * pVert1 = &v1;
    const VertexOut * pVert2 = &v2;
    const VertexOut * pVert3 = &v3;
    vector<const VertexOut *> vector = {pVert1 , pVert2 , pVert3};
    // 根据纵坐标排序
    sort(vector.begin(), vector.end() , [](const VertexOut * p1 , const VertexOut * p2)->bool {
        return p1->pos.y >= p2->pos.y;
    });
    // 上面的顶点
    pVert1 = vector.at(0);
    pVert2 = vector.at(1);
    pVert3 = vector.at(2);
    
    int startPY = pVert3->pos.y;
    int endPY = pVert1->pos.y;
    
    int sign = endPY > startPY ? 1 : -1;
    
    for (int py = startPY ; py * sign < sign * endPY ; py = py + sign) {
        Ldouble ld = 1.0f;
        Ldouble factor = (py - startPY) * ld / (endPY - startPY);
        VertexOut vertStart = pVert3->interpolate(*pVert2, factor);
        VertexOut vertEnd = pVert3->interpolate(*pVert1, factor);
        scanLineFill(vertStart, vertEnd , py);
    };
}

void Canvas::scanLineFill(const VertexOut &v1, const VertexOut &v2 , int yIndex) {
    
    const VertexOut * pVert1 = &v1;
    const VertexOut * pVert2 = &v2;
    
    pVert1 = v1.pos.x > v2.pos.x ? &v2 : &v1;
    pVert2 = v1.pos.x < v2.pos.x ? &v2 : &v1;
    
    int startX = pVert1->pos.x;
    int endX = pVert2->pos.x;
    
    if (startX == endX) {
        drawPixel(startX , yIndex, pVert1->getZ(), pVert1->color);
    }
    for (int x = startX ; x <= endX ; ++ x) {
        Ldouble factor = (x - startX) * 1.0f / (endX - startX);
        VertexOut v = pVert1->interpolate(*pVert2, factor);
        Ldouble z = v.getZ();
        drawPixel(x , yIndex , z , v.color);
    }
}

VertexOut Canvas::handleVertex(const Vertex &vert) const {
    VertexOut vertOut = _shader->vs(vert);
    transformToScrn(vertOut);
    return vertOut;
}

void Canvas::transformToScrn(VertexOut &vert) const {
    // 透视除法将cvv坐标转化成Ndc坐标
    vert.oneDivZ = 1 / vert.pos.w;
    vert.pos = vert.pos.get3DNormal();
    vert.pos.w = 1;
    vert.pos.x = _getPX(vert.pos.x);
    vert.pos.y = _getPY(vert.pos.y);
}

void Canvas::putPixel(int px , int py , const Color &color) {
    bool outX = px < 0 || px > _width;
    bool outY = py < 0 || py > _height;
    if (outX || outY) {
        return;
    }
    unsigned index = getIndex(px, py);
    auto pixels = getPixels();
    pixels[index] = color.uint32();
}

void Canvas::lineBresenham(const VertexOut &vert1, const VertexOut &vert2) {
    const VertexOut * pVert1 = &vert1;
    const VertexOut * pVert2 = &vert2;
    
    int px1 = pVert1->pos.x;
    int py1 = pVert1->pos.y;
    
    int px2 = pVert2->pos.x;
    int py2 = pVert2->pos.y;
    
    int dx = abs(px2 - px1);
    int dy = abs(py2 - py1);
    
    if (dx >= dy) {
        if (px1 > px2) {
            swap(pVert1, pVert2);
        }
        int px1 = pVert1->pos.x;
        int py1 = pVert1->pos.y;
        
        int px2 = pVert2->pos.x;
        int py2 = pVert2->pos.y;
        
        Color color1 = pVert1->color;
        Color color2 = pVert2->color;
        
        int sign = py2 >= py1 ? 1 : -1;  //斜率[-1,1]
        int k = sign * dy * 2;
        int e = -dx * sign;
        
        Ldouble z1 = pVert1->getZ();
        Ldouble z2 = pVert2->getZ();
        
        for (int x = px1 , y = py1;x <= px2; ++x) {
            Ldouble factor = static_cast<Ldouble>((x - px1) * 1.0 / (px2 - px1));
            Ldouble z;
            if (!MathUtil::equal(z1, z2)) {
                z = 1 / MathUtil::interpolate(1 / z1 , 1 / z2, factor);
                factor = (z - z1) / (z2 - z1);
            } else {
                z = z1;
            }
            Color color = color1.interpolate(color2, factor);
            drawPixel(x , y , z, color);
            e += k;
            if (sign * e > 0) {
                y += sign;
                e -= 2 * dx * sign;
            }
        }
    } else {
        if (py1 > py2) {
            swap(pVert1, pVert2);
        }
        int px1 = pVert1->pos.x;
        int py1 = pVert1->pos.y;
        
        int px2 = pVert2->pos.x;
        int py2 = pVert2->pos.y;
        
        Color color1 = pVert1->color;
        Color color2 = pVert2->color;
        
        Ldouble z1 = pVert1->getZ();
        Ldouble z2 = pVert2->getZ();
        
        int sign = px2 > px1 ? 1 : -1;  //斜率[-1,1]
        int k = sign * dx * 2;
        int e = -dy * sign;
        
        for (int x = px1 , y = py1; y <= py2 ; ++y) {
            Ldouble factor = static_cast<Ldouble>((x - px1) * 1.0 / (px2 - px1));
            Ldouble z;
            if (!MathUtil::equal(z1, z2)) {
                z = 1 / MathUtil::interpolate(1 / z1 , 1 / z2, factor);
                factor = (z - z1) / (z2 - z1);
            } else {
                z = z1;
            }
            Color color = color1.interpolate(color2, factor);
            drawPixel(x , y , z, color);
            e += k;
            if (sign * e > 0) {
                x += sign;
                e -= 2 * dy * sign;
            }
        }
    }
}














