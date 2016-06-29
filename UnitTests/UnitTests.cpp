// UnitTests.cpp : Defines the entry point for the console application.
//

#include "Source.cpp"

static void assert(bool b)
{
   if (!b) throw;
}

static void testComputeDistance()
{
   Entity e1(0, 0, 0, 0, 0, 0);
   Entity e2(0, 4, 3, 0, 0, 0);
   int distance = computeDistance(e1, e2);
   assert(distance == 5);
}

static void testComputeDistanceFromBase()
{
   Entity e1(0, 4, 3, 0, 0, 0);
   g_myBaseCoord = make_pair(0,0);
   int distance = computeDistanceFromBase(e1);
   assert(distance == 5);
}

static void testCanRelease()
{
   g_myBaseCoord = make_pair(0, 0);

   Entity e1(0, 4, 3, 0, 0, 0);
   assert(canRelease(e1) == true);

   Entity e2(0, 5000, 5000, 0, 0, 0);
   assert(canRelease(e2) == false);

   Entity e3(0, 10000, 8000, 0, 0, 0);
   assert(canRelease(e3) == false);
}

static void testSelectClosestGhost()
{
   Entity b1(0, 0, 0, 0, 0, 0);
 
   Entity g1(1, 4, 3, 0, 0, 0);
   Entity g2(2, 6, 9, 0, 0, 0);
   Entity g3(3, 10, 5, 0, 0, 0);
   Entity g4(4, 20, 5, 0, 0, 0);

   g_ghosts.push_back(g1);
   g_ghosts.push_back(g2);
   g_ghosts.push_back(g3);
   g_ghosts.push_back(g4);

   const Entity& closestGhost = selectClosestGhost(b1);
   assert(closestGhost.m_id == 1);
}

int main()
{
   testComputeDistance();
   testSelectClosestGhost();
   testComputeDistanceFromBase();
   testCanRelease();
	return 0;
}

