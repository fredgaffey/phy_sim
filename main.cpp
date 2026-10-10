// https://www.geometrictools.com/Documentation/TriangulationByEarClipping.pdf
// https://en.wikipedia.org/wiki/Centroid
#include "stdio.h"
#include <_string.h>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include "raylib/src/raylib.h"

#define MAX_EDGE_CNT 1000

// this is not used for now 
float scrMinX = 0;
float scrMaxX = 4000;
float scrMinY = 0;
float scrMaxY = 4000;
int canvasMaxX = 1000;
int canvasMaxY = 1000;


class poly;

class f_vec{
  // this idear I got from the ardupilo.org progect
  public:
    float x;
    float y;
    double mag(){return pow((pow(x,2)+pow(y,2)),0.5);}

    double dot(f_vec _v2){
      return x*_v2.x + y*_v2.y;
    }

    f_vec(float _x1, float _y1, float _x2, float _y2){
      x = _x1 - _x2;
      y = _y1 - _y2;
    }

    f_vec(float _x, float _y){
      x=_x;
      y=_y;}

    f_vec(float _v[1]){
      x=_v[0];
      y=_v[1];}

    f_vec(){
      x=0;
      y=0;}
};

struct Line{
  double m;
  double c;

  f_vec get_intecetion(Line l2){
    // solve simltaiosly
    double x = (l2.c + c)/(l2.m + m);
    double y = m*x + c;
    return f_vec(x,y);
  }

};

Line get_nomal(f_vec _a, double dist){
    double _m;
    _m = _a.y / _a.x; // rise/run
    _m = -1.0 * (1/_m); // negitive reciprocal (perpindcular)
                            //
    double scaler = _a.mag()/dist;

    _a.x *= scaler;
    _a.y *= scaler;

    // PG reaaranged 
    double _c = (-1.0 * _a.x) + _a.y;
    return Line{_m, _c};
  }

Line get_nomal_scaler(f_vec _a, double scaler){
  return get_nomal(_a, _a.mag()*scaler);
  }

class ear {
  public:
    int points[2*3]; // x,y, x,y x,y
                     // ^^^ this is the origin for this tri
    double area;
    f_vec center;
    poly* derived_poly;


    ear(poly* _derived_poly, int* _points){
      // area = (1/2).a.b.sin(C) 
      f_vec a = f_vec(_points[0],_points[1],_points[2],_points[3]);
      f_vec b = f_vec(_points[0],_points[1],_points[4],_points[5]);

      for (int i = 0; i < 6; i++){
        points[i] = _points[i];
      }

      double c = acos(a.dot(b)/(a.mag()*b.mag()));
      area = 0.5 * a.mag() * b.mag() * sin(c);
    
      Line na = get_nomal_scaler(a,0.5);
      Line nb = get_nomal_scaler(a,0.5);

      center = na.get_intecetion(nb);
      derived_poly = _derived_poly;
    }
};

struct vtx {
  vtx* next;
  vtx* prev;
  int x;
  int y;
  int point_cnt;

  vtx(int _x, int _y, vtx* _prev = nullptr, vtx* _next = nullptr, int _point_cnt = 0){
    if(_prev != nullptr){
        assert(_next != _prev);
    }

    x = _x;
    y = _y;
    next = _next;
    prev = _prev;
    point_cnt = _point_cnt;
  };
};

class poly {
  public:
    vtx* origin;
    bool cw;
    double area = 0;

  poly(int* point, int point_cnt, bool print = true){
    vtx* _origin = (vtx*) malloc(sizeof(vtx)*point_cnt);
    int x;
    int y;
    int i = 0; 

    vtx* next = _origin+1;
    vtx* prev = _origin+point_cnt;

    // printf("x= %d\n",x);
    // printf("y= %d\n", y);
    // printf("next= %p\n", next);
    // printf("prev= %p\n", prev);
    // printf("self %p\n",_origin+i);
    // printf("i= %d\n", i);
    // printf("---------------\n");

    // *_origin = vtx(x,y,next,prev,i);

    for(; i < point_cnt; i++){
      x = *(point+(i*2));
      y = *(point+(i*2)+1);

      next = _origin+(i+1);
      prev = _origin+(i-1);

      *(_origin+i) = vtx(x,y,prev,next,i);
    }

    _origin[i].prev = _origin+point_cnt-1;
    _origin[point_cnt-1].next = _origin;

    origin = _origin;
    validate(_origin,true);
  }

  poly(ear &e) : poly(e.points, 3) {}

  int validate(vtx* _origin,bool print = false){
    vtx* next = _origin;
    if (print) printf("START validate\n");
    int i = 0;
    do{
      if (print == true) {
        printf("%d,%d\n",next->x,next->y);
      }
      next = next->next;
      i++;

    }while(i<MAX_EDGE_CNT && next != _origin);
    assert(i >= 3);
    if (print) printf("END validate\n");
    return i;
  };
  int validate(bool print = false){
    return validate(origin,print);
  };
};

bool drawVtx(vtx* vertex, float rad = 5)
{ 
  // printf("%d,%d\n", ((vertex->x)), (vertex->y));
  // DrawCircle(((1.0*(vertex->x))-scrMinX)/scrMaxX * canvasMaxX, (1.0*(vertex->y) - scrMinY)/scrMaxY * canvasMaxY, rad, RED);
  DrawCircle((vertex->x), (vertex->y), rad, RED);
  return true;
}

bool drawPoly(poly* polygon, float ran = 5, float thick = 2){
  vtx* c_vtx = polygon->origin;
  // DrawLine(1000,1000, 1000,1000,BLUE);
  do{
    drawVtx(c_vtx);
    DrawLine(c_vtx->x,c_vtx->y,c_vtx->next->x,c_vtx->next->y,BLUE);
    c_vtx = c_vtx->next;
  } while (c_vtx->next != polygon->origin);

  DrawLine(c_vtx->x,c_vtx->y,c_vtx->next->x,c_vtx->next->y,BLUE);
  drawVtx(c_vtx);

  return true;
}

bool drawEar(ear* _ear, float rad = 1000, float thick = 2){
  double x = _ear->center.x;
  double y = _ear->center.y;
  DrawCircle(x,y, rad, GREEN);
  printf("ear center x%d\n",x);
  printf("ear center y%d\n",y);

  poly polygon(*_ear);
  assert(polygon.validate(true) != -1);
  drawPoly(&polygon);
  return true;
}

int main(){
  srand(1000); // random seed

  int i = 0;
  int const e = 3;
  int x, y;

  int poly_arr[e*2];

  while (i < e) {
    // this will make random polys 
    x = rand() % 300;
    y = rand() % 300;

    poly_arr[i*2] = x;
    poly_arr[(i*2)+1] = y;
    i++;
  }

  InitWindow(canvasMaxX, canvasMaxY, "raylib example - basic window");

  // int _p[8]= {1000,0, 1000,1000, 1000,0};
  poly pol = poly(&poly_arr[0],e);

  ear era = ear(&pol,poly_arr);
  drawEar(&era);

  pol.area = 0;
  
  // drawPoly(&pol);

  //poly has_a_cracker = pol
    // void DrawPixel(int posX, int posY, Color color); // Draw a pixel using geometry [Can be slow, use with care]

    // void DrawCircle(int centerX, int centerY, float radius, Color color);
    //     void DrawLine(int startPosX, int startPosY, int endPosX, int endPosY, Color color);
    while (!WindowShouldClose())
    {
        BeginDrawing();
            ClearBackground(RAYWHITE);

            drawPoly(&pol);

        EndDrawing();
    }

    CloseWindow();
  return 0;
}
