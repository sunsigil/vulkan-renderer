#include "geom_lib_2d.h"

#include "PGA/pga.h"

#define TOS_min(a, b) ((a) < (b) ? (a) : (b))

//Displace a point p on the direction d
//The result is a point
Point2D move(Point2D p, Dir2D d){
	return p+d;
}

//Compute the displacement vector between points p1 and p2
//The result is a direction 
Dir2D displacement(Point2D p1, Point2D p2){
	return p2-p1;
}

//Compute the distance between points p1 and p2
//The result is a scalar 
float dist(Point2D p1, Point2D p2){
	return displacement(p1, p2).magnitude();
}

//Compute the perpendicular distance from the point p the the line l
//The result is a scalar 
float dist(Line2D l, Point2D p){
	return abs(l.x*p.x + l.y*p.y + l.w) / l.magnitude();
}

//Compute the perpendicular distance from the point p the the line l
//The result is a scalar 
float dist(Point2D p, Line2D l){
  return dist(l, p);
}

//Compute the intersection point between lines l1 and l2
//You may assume the lines are not parallel
//The results is a a point that lies on both lines
Point2D intersect(Line2D l1, Line2D l2){
	return MultiVector(l1).wedge(MultiVector(l2));
}

//Compute the line that goes through the points p1 and p2
//The result is a line 
Line2D join(Point2D p1, Point2D p2){
	return MultiVector(p1).vee(MultiVector(p2));
}

//Compute the projection of the point p onto line l
//The result is the closest point to p that lies on line l
Point2D project(Point2D p, Line2D l){
	MultiVector pmv = MultiVector(p);
	MultiVector lmv = MultiVector(l);
	return pmv.dot(lmv) * lmv.normalized();
}

//Compute the projection of the line l onto point p
//The result is a line that lies on point p in the same direction of l
Line2D project(Line2D l, Point2D p){
	MultiVector pmv = MultiVector(p);
	MultiVector lmv = MultiVector(l);
	return lmv.dot(pmv) * pmv.normalized();
}

//Compute the angle point between lines l1 and l2 in radians
//You may assume the lines are not parallel
//The results is a scalar
float angle(Line2D l1, Line2D l2){
	MultiVector l1mv = MultiVector(l1);
	MultiVector l2mv = MultiVector(l2);
	return acos(l1mv.dot(l2mv).magnitude() / (l1mv.magnitude() * l2mv.magnitude()));
}

//Compute if the line segment p1->p2 intersects the line segment a->b
//The result is a boolean
bool segmentSegmentIntersect(Point2D p1, Point2D p2, Point2D a, Point2D b)
{
	Line2D l1 = join(p1, p2);
	Line2D l2 = join(a, b);
	if(sign(vee(l1, a)) == sign(vee(l1, b)))
		return false;
	if(sign(vee(l2, p1)) == sign(vee(l2, p2)))
		return false;
	return true;
}

//Compute if the point p lies inside the triangle t1,t2,t3
//Your code should work for both clockwise and counterclockwise windings
//The result is a bool
bool pointInTriangle(Point2D p, Point2D t1, Point2D t2, Point2D t3)
{
	Line2D a = join(t1, t2);
	Line2D b = join(t2, t3);
	Line2D c = join(t3, t1);
	int as = sign(vee(a, p));
	int bs = sign(vee(b, p));
	if(as != bs)
		return false;
	int cs = sign(vee(c, p));
	if(bs != cs)
		return false;
	return true;
}

//Compute the area of the triangle t1,t2,t3
//The result is a scalar
float areaTriangle(Point2D t1, Point2D t2, Point2D t3)
{
	Line2D ab = join(t1, t2);
	Line2D ac = join(t1, t3);
	return wedge(ab, ac).magnitude() * 0.5f;
}

//Compute the distance from the point p to the triangle t1,t2,t3 as defined 
//by it's distance from the edge closest to p.
//The result is a scalar
//NOTE: There are some tricky cases to consider here that do not show up in the test cases!
float pointTriangleEdgeDist(Point2D p, Point2D t1, Point2D t2, Point2D t3)
{
	Line2D a = join(t1, t2);
	Line2D b = join(t2, t3);
	Line2D c = join(t3, t1);
	float da = dist(a, p);
	if(da == 0)
		return TOS_min(dist(t1, p), dist(t2, p));
	float db = dist(b, p);
	if(db == 0)
		return TOS_min(dist(t2, p), dist(t3, p));
	float dc = dist(c, p);
	if(dc == 0)
		return TOS_min(dist(t3, p), dist(t1, p));
	return TOS_min(TOS_min(da, db), dc);
}

//Compute the distance from the point p to the closest of three corners of
// the triangle t1,t2,t3
//The result is a scalar
float pointTriangleCornerDist(Point2D p, Point2D t1, Point2D t2, Point2D t3)
{
	float d1 = dist(t1, p);
	float d2 = dist(t2, p);
	float d3 = dist(t3, p);
	return TOS_min(TOS_min(d1, d2), d3);
}

//Compute if the quad (p1,p2,p3,p4) is convex.
//Your code should work for both clockwise and counterclockwise windings
//The result is a boolean
// A---B
// |   |
// D---C
bool isConvex_Quad(Point2D p1, Point2D p2, Point2D p3, Point2D p4)
{
	int dba = sign(vee(join(p4, p2), p1));
	int acb = sign(vee(join(p1, p3), p2));
	if(dba != acb)
		return false;
	int bdc = sign(vee(join(p2, p4), p3));
	if(acb != bdc)
		return false;
	int cad = sign(vee(join(p3, p1), p4));
	if(bdc != cad)
		return false;
	return true;
}

MultiVector sandwich(MultiVector u, MultiVector a)
{
	u = u.normalized();
	return u.reverse().times(a).times(u);
}

//Compute the reflection of the point p about the line l
//The result is a point
Point2D reflect(Point2D p, Line2D l)
{
	return sandwich(MultiVector(l), MultiVector(p));
}

//Compute the reflection of the line d about the line l
//The result is a line
Line2D reflect(Line2D d, Line2D l)
{
	return sandwich(MultiVector(l), MultiVector(d));
}